#include <array>
#include <chrono>
#include <memory>

#include <QtCore/QRegularExpression>
#include <QtCore/QStringList>

#include "../Support/GPSRuntimeTestIO.h"
#include "../Support/ProtocolTestPackets.h"
#include "AllocationTracker.h"
#include "GPSProtocolMath.h"
#include "GPSProtocolRuntime.h"
#include "GPSRuntimeTestSupport.h"
#include "QGCLoggingCategory.h"
#include "ToyFamily.h"
#include "UnitTest.h"

QGC_LOGGING_CATEGORY(ToyFamilyLog, "GPS.Driver.Protocols.Toy")

using namespace std::chrono_literals;

namespace {

using GPSRuntimeTest::LatencyModel;

constexpr uint64_t START_US = 1000000;

QByteArray line(const char* text)
{
    return QByteArray(text) + "\r\n";
}

QByteArray rtcm()
{
    const auto frame = rtcmPacket(std::array<uint8_t, 6>{0x3e, 0xd0, 1, 2, 3, 4});
    return QByteArray(reinterpret_cast<const char*>(frame.data()), static_cast<qsizetype>(frame.size()));
}

/// A toy receiver that answers only at 115200 baud and rejects the optional satellite command.
void scriptToyReceiver(LatencyModel& model)
{
    model.answeringRates = {115200};
    model.script("TOY?\r\n", {{line("$TOYID,TOY-1,2.3")}});
    model.script("TOYRATE,1\r\n", {{line("$TOYACK,TOYRATE,1")}});
    model.script("TOYSAT,ON\r\n", {{line("$TOYNAK,TOYSAT,ON")}});
    model.script("TOYSVIN,60\r\n", {{line("$TOYACK,TOYSVIN,60")}});
    // The RTCM switch answers without a line ending, so only the raw matcher can see it.
    model.script("TOYRTCM,ON\r\n", {{"xRTCM ON"}});
}

struct ToyLink
{
    ToyLink()
        : model(clock)
        , receiver(std::stop_token{}, &model)
    {
        scriptToyReceiver(model);
    }

    GPSRuntimeIO io() { return receiver.makeIO(makeGPSRuntimeTestIO(clock)); }

    QStringList writes() const
    {
        QStringList result;
        for (const auto& command : receiver.commands()) {
            result.append(QString::fromLatin1(command));
        }
        return result;
    }

    GPSTestClock clock{START_US};
    LatencyModel model;
    ScriptedReceiver receiver;
};

/// Publishes one position per UBX frame and forwards RTCM3, as a navigation decoder does. Its readiness ignores the
/// sticky failure, and its streaming service is an empty task returned without a coroutine.
class PositionPublisher final : public GPSFamilyProtocol
{
public:
    GPSTask<bool> configure(GPSCommandChannel&, GPSConfig, unsigned&) override { co_return true; }

    GPSTask<void> serviceStreaming(GPSCommandChannel&) override { return GPSTask<void>::ready(); }

    GPSReceiveUpdates onFrame(const GPSFrame& frame, GPSDecodeContext& context) override
    {
        if (frame.kind == GPSFrameKind::RTCM3) {
            context.sink().publishRTCM(frame.bytes);
            return {};
        }
        GPSDecodedPosition position;
        position.navigation.timestampUs = context.nowUs();
        position.navigation.satellitesUsed = static_cast<uint8_t>(frame.payload.size());
        context.sink().publishPosition(position);
        return GPSReceiveUpdate::Activity;
    }
};

constexpr GPSReceiverFamily UBX_STREAM_FAMILY{
    .type = GPSType::ublox,
    .name = QLatin1StringView("ubx-stream"),
    .stream = {.framers = GPSFrameKind::UBX | GPSFrameKind::RTCM3, .enabled = GPSFrameKind::UBX | GPSFrameKind::RTCM3}};

GPSConfig surveyIn()
{
    return {
        .base = {.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = 2.0, .duration = std::chrono::seconds(60)}}};
}

}  // namespace

class GPSProtocolRuntimeTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _toyFamilyConfiguresAndStreams();
    void _decodeIsPure();
    void _rejectsReentrantReceive();
    void _rejectsUnsupportedConfiguration();
    void _steadyStateDecodeDoesNotAllocate_data();
    void _steadyStateDecodeDoesNotAllocate();
    void _readinessSeesSessionFailure();
    void _publishAsReceivedKeepsReceipt();
    void _utcMicroseconds_data();
    void _utcMicroseconds();
};

void GPSProtocolRuntimeTest::_toyFamilyConfiguresAndStreams()
{
    ToyLink link;
    QStringList events;
    GPSRuntimeObserver observer;
    observer.decoded = [&events](const GPSEventBatch& batch) {
        for (const auto& event : batch.events) {
            events.append(std::visit([](const auto& report) { return GPSRuntimeTest::summary(report); }, event)
                              .section(QLatin1Char(' '), 0, 0));
        }
    };
    GPSProtocolRuntime runtime(Toy::FAMILY, link.io(), std::move(observer));
    QVERIFY(!runtime.receiverReady());

    unsigned baud = 0;
    QVERIFY(runtime.configure(surveyIn(), baud));
    // The survey start published by the configurator was flushed without another read.
    QCOMPARE(events, QStringList{"survey"});
    QCOMPARE(baud, 115200U);
    QVERIFY(runtime.receiverReady());
    QCOMPARE(runtime.identity(), QStringLiteral("TOY-1 2.3"));
    QCOMPARE(link.model.bauds, (QList<unsigned>{9600, 115200}));
    QCOMPARE(link.writes(), (QStringList{"TOY?\r\n", "TOY?\r\n", "TOYRATE,1\r\n", "TOYSAT,ON\r\n", "TOYSVIN,60\r\n",
                                         "TOYRTCM,ON\r\n"}));
    QStringList outcomes;
    for (const auto& evidence : runtime.configurationEvidence()) {
        outcomes.append(
            QStringLiteral("%1=%2").arg(QString::fromStdString(evidence.command)).arg(int(evidence.outcome)));
    }
    QCOMPARE(outcomes, (QStringList{QStringLiteral("TOY?=%1").arg(int(GPSCommandOutcome::TimedOut)),
                                    QStringLiteral("TOY?=%1").arg(int(GPSCommandOutcome::ReadbackVerified)),
                                    QStringLiteral("TOYRATE,1=%1").arg(int(GPSCommandOutcome::Acknowledged)),
                                    QStringLiteral("TOYSAT,ON=%1").arg(int(GPSCommandOutcome::Rejected)),
                                    QStringLiteral("TOYSVIN,60=%1").arg(int(GPSCommandOutcome::Acknowledged)),
                                    QStringLiteral("TOYRTCM,ON=%1").arg(int(GPSCommandOutcome::Acknowledged))}));
    // The identity probe at 9600 baud timed out after its full command timeout.
    QCOMPARE(runtime.configurationEvidence().front().finishedAtUs - runtime.configurationEvidence().front().startedAtUs,
             uint64_t(Toy::COMMAND_TIMEOUT.count() * 1000));

    const uint64_t streaming = link.clock.nowUs();
    link.model.emitAt(streaming + 10000, line("$TOYPOS,3,47.1,8.5,12") + rtcm());
    link.model.emitAt(streaming + 60000, line("$TOYSVIN,1,0,5") + line("$TOYWARN"));
    GPSReceiveUpdates updates;
    for (int cycle = 0; cycle < 4; ++cycle) {
        updates |= runtime.receive(50ms);
    }
    QVERIFY(updates.testFlag(GPSReceiveUpdate::Position));
    QVERIFY(updates.testFlag(GPSReceiveUpdate::Satellites));
    QVERIFY(updates.testFlag(GPSReceiveUpdate::Activity));
    QCOMPARE(events, (QStringList{"survey", "usage", "position", "rtcm", "survey"}));
    // The warning scheduled a diagnostics poll, written by the streaming service after that receive.
    QCOMPARE(link.writes().last(), QStringLiteral("TOYDIAG\r\n"));
    QCOMPARE(runtime.error(), GPSProtocolError::None);
}

void GPSProtocolRuntimeTest::_decodeIsPure()
{
    ToyLink link;
    int delivered = 0;
    GPSRuntimeObserver observer;
    observer.decoded = [&delivered](const GPSEventBatch&) { ++delivered; };
    GPSProtocolRuntime runtime(Toy::FAMILY, link.io(), std::move(observer));
    const QByteArray bytes = line("$TOYPOS,3,47.1,8.5,12") + line("$TOYSVIN,0,1,60");
    const auto chunk =
        runtime.decode({reinterpret_cast<const uint8_t*>(bytes.constData()), static_cast<size_t>(bytes.size())});
    QCOMPARE(chunk.bytesConsumed, static_cast<size_t>(bytes.size()));
    QCOMPARE(chunk.batch.events.size(), size_t{3});
    QVERIFY(std::holds_alternative<GPSDecodedSurvey>(chunk.batch.events.back()));
    QVERIFY(chunk.batch.updates.testFlag(GPSReceiveUpdate::Position));
    QCOMPARE(delivered, 0);
    QVERIFY(link.receiver.commands().isEmpty());
    QCOMPARE(link.model.reads, 0);
}

void GPSProtocolRuntimeTest::_rejectsReentrantReceive()
{
    ToyLink link;
    GPSProtocolRuntime* active = nullptr;
    GPSReceiveUpdates nested{GPSReceiveUpdate::Position};
    GPSRuntimeObserver observer;
    observer.decoded = [&active, &nested](const GPSEventBatch& batch) {
        if (active && !batch.events.empty()) {
            nested = active->receive(10ms);
            active = nullptr;
        }
    };
    GPSProtocolRuntime runtime(Toy::FAMILY, link.io(), std::move(observer));
    link.model.emitAt(START_US + 1000, line("$TOYPOS,3,47.1,8.5,12"));
    active = &runtime;
    expectLogMessage("GPS.Driver.Protocols.Toy", QtWarningMsg,
                     QRegularExpression(QStringLiteral("already in progress; receive rejected")));
    QVERIFY(runtime.receive(50ms).testFlag(GPSReceiveUpdate::Position));
    verifyExpectedLogMessage();
    QCOMPARE(nested, GPSReceiveUpdates{});
}

void GPSProtocolRuntimeTest::_rejectsUnsupportedConfiguration()
{
    ToyLink link;
    GPSProtocolRuntime runtime(Toy::FAMILY, link.io());
    GPSConfig config = surveyIn();
    config.allowPersistentChanges = true;
    unsigned baud = 0;
    expectLogMessage("GPS.Driver.Protocols.Toy", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Invalid receiver physical configuration")));
    QVERIFY(!runtime.configure(config, baud));
    verifyExpectedLogMessage();
    QVERIFY(link.receiver.commands().isEmpty());
    QVERIFY(runtime.configurationEvidence().empty());
}

void GPSProtocolRuntimeTest::_steadyStateDecodeDoesNotAllocate_data()
{
    QTest::addColumn<bool>("rtcm");
    QTest::newRow("ubx") << false;
    QTest::newRow("rtcm") << true;
}

void GPSProtocolRuntimeTest::_steadyStateDecodeDoesNotAllocate()
{
    QFETCH(bool, rtcm);
    const auto frame =
        rtcm ? rtcmPacket(std::vector<uint8_t>(92, 0x5a)) : ubxFrame(0x0701, std::vector<uint8_t>(92, 0x5a));
    uint64_t now = START_US;
    size_t events = 0;
    size_t ownedFrames = 0;
    GPSRuntimeObserver observer;
    observer.decoded = [&events, &ownedFrames](const GPSEventBatch& batch) {
        events += batch.events.size();
        for (const auto& event : batch.events) {
            // Each RTCM3 frame owns its bytes: one QByteArray block, which the consumer can keep without copying.
            if (const auto* correction = std::get_if<GPSRTCMFrame>(&event);
                correction && correction->bytes.isDetached()) {
                ++ownedFrames;
            }
        }
    };
    GPSProtocolRuntime runtime(UBX_STREAM_FAMILY, std::make_unique<PositionPublisher>(),
                               {.nowUs = [&now] { return now; }}, std::move(observer));
    (void) runtime.consume(frame);
    events = 0;
    ownedFrames = 0;
    constexpr size_t ITERATIONS = 1000;
    QGCTest::AllocationTracker::Counts allocations;
    {
        QGCTest::AllocationTracker tracker;
        for (size_t index = 0; index < ITERATIONS; ++index) {
            now += 200000;
            (void) runtime.consume(frame);
        }
        allocations = tracker.counts();
    }
    QCOMPARE(events, ITERATIONS);
    // No C++ allocation either way. The tracker cannot see QByteArray's malloc, so the one block per RTCM3 frame is
    // counted through ownership instead.
    QCOMPARE(allocations.calls, size_t{0});
    QCOMPARE(ownedFrames, rtcm ? ITERATIONS : size_t{0});
}

void GPSProtocolRuntimeTest::_readinessSeesSessionFailure()
{
    ToyLink link;
    GPSProtocolRuntime toy(Toy::FAMILY, link.io());
    unsigned baud = 0;
    QVERIFY(toy.configure(surveyIn(), baud));
    QVERIFY(toy.receiverReady());
    link.receiver.failNextRead({GPSReadStatus::Error, 0, QStringLiteral("link lost")});
    expectLogMessage("GPS.Driver.Protocols.Toy", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Receiver read failed")));
    (void) toy.receive(50ms);
    verifyExpectedLogMessage();
    QCOMPARE(toy.error(), GPSProtocolError::Transport);
    // The toy ends readiness on a session failure, as Unicore and Quectel do.
    QVERIFY(!toy.receiverReady());

    GPSTestClock clock(START_US);
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
    QCOMPARE(clock.nowUs(), START_US + 10000);
    failRead = true;
    expectLogMessage("GPS.Driver.Protocols.Runtime", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Receiver read failed")));
    (void) publisher.receive(10ms);
    verifyExpectedLogMessage();
    QCOMPARE(publisher.error(), GPSProtocolError::Transport);
    // A family that keeps readiness after a failure, as UBX, Ashtech and Femto do, still reports ready.
    QVERIFY(publisher.receiverReady());
}

void GPSProtocolRuntimeTest::_publishAsReceivedKeepsReceipt()
{
    GPSEventSink sink([] { return uint64_t{99}; });
    GPSDecodedSurvey buffered{};
    buffered.timestamp = 42;
    buffered.survey.active = true;
    sink.publishAsReceived(buffered);
    GPSDecodedSurvey stamped{};
    sink.publishSurvey(stamped);
    const auto batch = sink.takeBatch();
    QCOMPARE(batch.updates, GPSReceiveUpdates{});
    QCOMPARE(batch.events.size(), size_t{2});
    QCOMPARE(std::get<GPSDecodedSurvey>(batch.events[0]).timestamp, uint64_t{42});
    QVERIFY(std::get<GPSDecodedSurvey>(batch.events[0]).survey.active);
    QCOMPARE(std::get<GPSDecodedSurvey>(batch.events[1]).timestamp, uint64_t{99});
}

void GPSProtocolRuntimeTest::_utcMicroseconds_data()
{
    QTest::addColumn<int>("year");
    QTest::addColumn<int>("nanoseconds");
    QTest::addColumn<quint64>("expected");
    // 2026-07-12T23:00:00Z is 1783897200 s after the epoch.
    QTest::newRow("sub-second") << 126 << 250000000 << quint64(1783897200250000);
    QTest::newRow("negative-sub-second") << 126 << -1000 << quint64(1783897199999999);
    QTest::newRow("before-plausibility-floor") << 100 << 0 << quint64(0);
}

void GPSProtocolRuntimeTest::_utcMicroseconds()
{
    QFETCH(int, year);
    QFETCH(int, nanoseconds);
    QFETCH(quint64, expected);
    tm utc{};
    utc.tm_year = year;
    utc.tm_mon = 6;
    utc.tm_mday = 12;
    utc.tm_hour = 23;
    QCOMPARE(GPSProtocolMath::utcMicroseconds(utc, nanoseconds), expected);
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSProtocolRuntimeTest, TestLabel::Unit)

#include "GPSProtocolRuntimeTest.moc"
