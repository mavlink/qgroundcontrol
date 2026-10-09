#include "GPSProtocolRuntimeTest.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <memory>
#include <span>
#include <vector>

#include <QtCore/QRegularExpression>
#include <QtCore/QStringList>

#include "AllocationTracker.h"
#include "GPSFamilyProtocol.h"
#include "GPSProtocolRuntime.h"
#include "GPSReceiverFamilies.h"
#include "LittleEndian.h"
#include "Protocols/Support/GPSProtocolTestBase.h"
#include "Protocols/Support/ProtocolTestPackets.h"
#include "Protocols/Support/ReceiverBench.h"
#include "Protocols/Support/UnicoreReceiverModel.h"
#include "UBX/UBXFrame.h"

using namespace std::chrono_literals;
using namespace GPSTest;

namespace {

/// Publishes one position per UBX frame and forwards RTCM3, as a navigation decoder does. Its readiness ignores the
/// sticky failure, and its streaming service does nothing.
class PositionPublisher final : public GPSFamilyProtocol
{
public:
    bool configure(GPSCommandChannel&, GPSConfig, unsigned&) override { return true; }

    void serviceStreaming(GPSCommandChannel&) override {}

    GPSReceiveUpdates onFrame(const GPSFrame& frame, GPSDecodeContext& context) override
    {
        if (frame.kind == GPSFrameKind::RTCM3) {
            context.publishRTCM(frame.bytes);
            return {};
        }
        GPSDecodedPosition position;
        position.navigation.timestampUs = context.nowUs();
        position.navigation.satellitesUsed = static_cast<uint8_t>(frame.payload.size());
        context.publishPosition(position);
        return GPSReceiveUpdate::Activity;
    }
};

/// Publishes a buffered survey status as received, then a new one, on each flush.
class SurveyPublisher final : public GPSFamilyProtocol
{
public:
    bool configure(GPSCommandChannel&, GPSConfig, unsigned&) override { return true; }

    GPSReceiveUpdates onFrame(const GPSFrame&, GPSDecodeContext&) override { return {}; }

    void flush(GPSDecodeContext& context) override
    {
        GPSSurveyReport buffered{};
        buffered.timestampUs = 42;
        buffered.active = true;
        context.publishAsReceived(buffered);
        GPSSurveyReport stamped{};
        context.publishSurvey(stamped);
    }
};

constexpr GPSReceiverFamily UBX_STREAM_FAMILY{
    .type = GPSType::ublox,
    .stream = {.framers = GPSFrameKind::UBX | GPSFrameKind::RTCM3, .enabled = GPSFrameKind::UBX | GPSFrameKind::RTCM3}};

}  // namespace

void GPSProtocolRuntimeTest::_readinessSeesSessionFailure()
{
    GPSTestClock unicoreClock(GPSTestClock::START_US);
    ModelReceiver<UnicoreReceiverModel> unicore(unicoreClock);
    GPSProtocolRuntime ending(Unicore::FAMILY, unicore.io());
    unsigned baud = 115200;
    QVERIFY(ending.configure({.base = {.mode = GPSBaseStationConfig::ReceiverAveraging{}}}, baud));
    QVERIFY(ending.receiverReady());
    unicore.model.readError = true;
    expectLogMessage("GPS.Protocols.Unicore", QtWarningMsg, QRegularExpression(QStringLiteral("Receiver read failed")));
    (void) ending.receive(50ms);
    verifyExpectedLogMessage();
    QCOMPARE(ending.error(), GPSProtocolError::Transport);
    QVERIFY(!ending.receiverReady());

    GPSTestClock clock(GPSTestClock::START_US);
    bool failRead = false;
    GPSRuntimeIO io = makeGPSRuntimeTestIO(clock);
    io.read = [&clock, &failRead](std::span<uint8_t>, GPSDeadline deadline) {
        if (failRead) {
            return GPSReadResult{GPSReadStatus::Error, 0, QStringLiteral("link lost")};
        }
        clock.advanceTo(deadline.untilUs);
        return GPSReadResult{GPSReadStatus::TimedOut};
    };
    GPSProtocolRuntime publisher(UBX_STREAM_FAMILY, std::make_unique<PositionPublisher>(), std::move(io));
    QVERIFY(publisher.configure({}, baud));
    // The empty streaming service completes like any other.
    QCOMPARE(publisher.receive(10ms), GPSReceiveUpdates{});
    QCOMPARE(clock.nowUs(), GPSTestClock::START_US + 10000);
    failRead = true;
    expectLogMessage("GPS.Protocols.Runtime", QtWarningMsg, QRegularExpression(QStringLiteral("Receiver read failed")));
    (void) publisher.receive(10ms);
    verifyExpectedLogMessage();
    QCOMPARE(publisher.error(), GPSProtocolError::Transport);
    // A session failure ends readiness whatever the family reports.
    QVERIFY(!publisher.receiverReady());
}

void GPSProtocolRuntimeTest::_publishAsReceivedKeepsReceipt()
{
    GPSTestClock clock(99);
    GPSProtocolRuntime runtime(UBX_STREAM_FAMILY, std::make_unique<SurveyPublisher>(), makeGPSRuntimeTestIO(clock));
    const auto batch = runtime.decode({});
    QCOMPARE(batch.updates, GPSReceiveUpdates{});
    QCOMPARE(batch.events.size(), size_t{2});
    QCOMPARE(std::get<GPSSurveyReport>(batch.events[0]).timestampUs, uint64_t{42});
    QVERIFY(std::get<GPSSurveyReport>(batch.events[0]).active);
    QCOMPARE(std::get<GPSSurveyReport>(batch.events[1]).timestampUs, uint64_t{99});
}

void GPSProtocolRuntimeTest::_deadlineRemaining_data()
{
    QTest::addColumn<quint64>("untilUs");
    QTest::addColumn<quint64>("nowUs");
    QTest::addColumn<qint64>("expectedMs");
    QTest::newRow("unbounded") << quint64(GPSDeadline{}.untilUs) << quint64(0) << qint64(INT32_MAX);
    QTest::newRow("due") << quint64(0) << quint64(0) << qint64(0);
    QTest::newRow("one-microsecond-rounds-up") << quint64(1) << quint64(0) << qint64(1);
    QTest::newRow("whole-millisecond") << quint64(1000) << quint64(0) << qint64(1);
    QTest::newRow("partial-millisecond-rounds-up") << quint64(1001) << quint64(0) << qint64(2);
    QTest::newRow("past") << quint64(1000) << quint64(1001) << qint64(0);
    QTest::newRow("near-clock-limit") << quint64(UINT64_MAX) << quint64(UINT64_MAX - 1001) << qint64(2);
    QTest::newRow("at-clock-limit") << quint64(UINT64_MAX) << quint64(UINT64_MAX) << qint64(0);
}

void GPSProtocolRuntimeTest::_deadlineRemaining()
{
    QFETCH(quint64, untilUs);
    QFETCH(quint64, nowUs);
    QFETCH(qint64, expectedMs);
    QCOMPARE(GPSDeadline{untilUs}.remaining(nowUs), std::chrono::milliseconds(expectedMs));
}

void GPSProtocolRuntimeTest::_deadlineAfter()
{
    QCOMPARE(GPSDeadline::after(1000, std::chrono::milliseconds(2)).untilUs, 3000u);
    // A negative timeout is already due.
    QCOMPARE(GPSDeadline::after(1000, std::chrono::milliseconds(-1)).untilUs, 1000u);
}

void GPSProtocolRuntimeTest::_linkBaud_data()
{
    QTest::addColumn<unsigned>("requested");
    QTest::addColumn<unsigned>("detected");
    QTest::addColumn<unsigned>("expected");
    QTest::newRow("fallback") << 0u << 0u << 115200u;
    QTest::newRow("requested") << 9600u << 0u << 9600u;
    QTest::newRow("detected") << 0u << 230400u << 230400u;
    QTest::newRow("requested-over-detected") << 9600u << 230400u << 9600u;
}

void GPSProtocolRuntimeTest::_linkBaud()
{
    QFETCH(unsigned, requested);
    QFETCH(unsigned, detected);
    QFETCH(unsigned, expected);
    QCOMPARE(GPSConfig{.detectedBaud = detected}.linkBaud(requested, 115200), expected);
}

void GPSProtocolRuntimeTest::_steadyStateDecodeDoesNotAllocate()
{
    const auto recorded = GPSTest::fixtureBytes(GPSTest::NAV_PVT);
    QVERIFY2(recorded && recorded->size() == 100, "Cannot read the independent NAV-PVT fixture");
    const std::vector<uint8_t>& fixture = *recorded;
    // The fixture's NAV-PVT at a newer time of week, the NAV-EOE that publishes its epoch, and an RTCM3 frame.
    uint32_t tow = 0;
    const auto correction = rtcmPacket(std::vector<uint8_t>(92, 0x5a));
    const auto nextEpoch = [&fixture, &tow, &correction] {
        auto epoch = ubxNavigationEpoch({fixture.begin() + 6, fixture.end() - 2}, tow += 200);
        epoch.insert(epoch.end(), correction.begin(), correction.end());
        return epoch;
    };
    uint64_t now = GPSTestClock::START_US;
    std::size_t positions = 0;
    std::size_t ownedFrames = 0;
    GPSProtocolRuntime* receiver = nullptr;
    std::vector<uint8_t> nested;
    bool injectNested = false;
    bool nestedSnapshotValid = true;
    GPSRuntimeIO io;
    io.clock.nowUs = [&] { return now; };
    GPSRuntimeObserver observer;
    observer.decoded = [&](const GPSEventBatch& batch) {
        for (const auto& event : batch.events) {
            positions += std::holds_alternative<GPSDecodedPosition>(event);
            // Each RTCM3 frame owns its bytes: one QByteArray block, which the consumer can keep without copying.
            if (const auto* frame = std::get_if<GPSRTCMFrame>(&event); frame && frame->bytes.isDetached()) {
                ++ownedFrames;
            }
            if (const auto* report = std::get_if<GPSDecodedPosition>(&event); report && injectNested) {
                injectNested = false;
                const auto timestamp = report->navigation.timestampUs;
                now += 200000;
                receiver->consume(nested);
                nestedSnapshotValid = report->navigation.timestampUs == timestamp;
            }
        }
    };
    GPSProtocolRuntime driver(UBX::FAMILY, std::move(io), std::move(observer));
    receiver = &driver;
    driver.armNavigationDecode({.corrections = true});
    driver.consume(nextEpoch());
    positions = 0;
    ownedFrames = 0;
    constexpr std::size_t ITERATIONS = 1000;
    std::vector<std::vector<uint8_t>> epochs(ITERATIONS);
    std::ranges::generate(epochs, nextEpoch);
    QGCTest::AllocationTracker::Counts allocations;
    {
        QGCTest::AllocationTracker tracker;
        for (const auto& epoch : epochs) {
            now += 200000;
            driver.consume(epoch);
        }
        allocations = tracker.counts();
    }
    QCOMPARE(positions, ITERATIONS);
    // The tracker cannot see QByteArray's malloc, so the one block per RTCM3 frame is counted through ownership.
    QCOMPARE(allocations.calls, size_t{0});
    QCOMPARE(ownedFrames, ITERATIONS);
    const auto owned = driver.decode(nextEpoch());
    const auto timestamp = now;
    now += 200000;
    driver.consume(nextEpoch());
    QCOMPARE(owned.events.size(), size_t{2});
    QVERIFY(std::holds_alternative<GPSDecodedPosition>(owned.events.front()));
    QCOMPARE(std::get<GPSDecodedPosition>(owned.events.front()).navigation.timestampUs, timestamp);
    const auto beforeNested = positions;
    const auto outer = nextEpoch();
    nested = nextEpoch();
    injectNested = true;
    driver.consume(outer);
    QVERIFY(nestedSnapshotValid);
    QCOMPARE(positions, beforeNested + 2);
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSProtocolRuntimeTest, TestLabel::Unit)
