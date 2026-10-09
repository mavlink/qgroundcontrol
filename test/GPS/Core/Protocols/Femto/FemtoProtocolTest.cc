#include "FemtoProtocolTest.h"

#include <algorithm>
#include <chrono>
#include <span>
#include <string>
#include <vector>

#include "GPSCommand.h"
#include "GPSCommandChannel.h"
#include "GPSProtocolRuntime.h"
#include "GPSReceiverConfig.h"
#include "GPSReceiverFamilies.h"
#include "Protocols/Support/FemtoReceiverModel.h"
#include "Protocols/Support/GPSProtocolTestBase.h"
#include "Protocols/Support/ReceiverBench.h"

using namespace std::chrono_literals;
using namespace GPSTest;

namespace {
/// The golden Femtomes model: "<COMMAND OK" for an accepted command, behind fault rules.
using Receiver = ModelReceiver<FemtoReceiverModel>;

void rawAcknowledgements(GPSTestClock&)
{
    for (const auto& patterns : {std::pair{"<LOG OK", "<ERROR"}, std::pair{"$R: set", "$R?"}}) {
        for (size_t chunk : {1u, 3u, 150u}) {
            GPSRawAckMatcher matcher(patterns.first, patterns.second);
            QVERIFY(matcher.valid());
            const std::string bytes = std::string(600, '\0') + patterns.first;
            for (size_t offset = 0; offset < bytes.size(); offset += chunk) {
                const size_t count = std::min(chunk, bytes.size() - offset);
                matcher.append({reinterpret_cast<const uint8_t*>(bytes.data() + offset), count});
            }
            QCOMPARE(matcher.outcome(), GPSCommandOutcome::Acknowledged);
        }
        for (bool negativeFirst : {false, true}) {
            GPSRawAckMatcher matcher(patterns.first, patterns.second);
            const std::string bytes = negativeFirst ? std::string(patterns.second) + '\0' + patterns.first
                                                    : std::string(patterns.first) + '\0' + patterns.second;
            matcher.append({reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size()});
            QCOMPARE(matcher.outcome(), GPSCommandOutcome::Rejected);
        }
    }
    GPSRawAckMatcher empty("", "");
    QVERIFY(!empty.valid());
    const std::string overlong(257, 'A');
    GPSRawAckMatcher oversized(overlong, "");
    QVERIFY(!oversized.valid());
}

void configurationDecodesInterleavedTraffic(GPSTestClock& clock)
{
    {
        Receiver peer(clock);
        // The position arrives ahead of the reply to the first UNLOGALL.
        const auto gga = toByteArray(nmeaSentence("GPGGA,123519,4807.038,N,01131.000,E,7,08,0.9,545.4,M,46.9,M,,"));
        peer.faults.rules.push_back(latestReply("UNLOGALL", gga + QByteArray("<UNLOGALL OK\0", 13), 1, 1));
        GPSProtocolRuntime driver = peer.runtime(Femto::FAMILY);
        GPSConfig config;
        config.base = {.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = 1, .duration = 60s}};
        unsigned baud = 115200;
        QVERIFY(driver.configure(config, baud));
        QCOMPARE(peer.log.count<GPSDecodedSatelliteUsage>(), 1);
        (void) driver.receive(10ms);
        // The fixed-quality GGA predates this configuration's survey, so it must not complete it.
        QVERIFY(!peer.sent("LOG RTCM"));
        QVERIFY(
            std::ranges::none_of(peer.log.reports<GPSSurveyReport>(), [](const auto& survey) { return survey.valid; }));
    }
}

constexpr GPSTest::ProtocolScenario SCENARIOS[] = {
    {"raw-acknowledgements", rawAcknowledgements},
    {"configuration-decodes-interleaved-traffic", configurationDecodesInterleavedTraffic},
};
}  // namespace

void FemtoProtocolTest::_scenario_data()
{
    addScenarioRows(SCENARIOS);
}

void FemtoProtocolTest::_scenario()
{
    runScenario(SCENARIOS);
}

void FemtoProtocolTest::_receiverMode_data()
{
    QTest::addColumn<bool>("fixed");
    QTest::addColumn<QString>("rejectedCommand");
    QTest::addColumn<bool>("cancelRead");
    QTest::addColumn<int>("readChunk");
    QTest::addColumn<int>("noiseBytes");
    // The femto goldens pin the other rejections and the survey-in and fixed transcripts.
    QTest::newRow("LOG GPGGA-rejected") << false << QStringLiteral("LOG GPGGA") << false << 7 << 0;
    for (const auto& [fixed, command] : {std::pair{false, "POSAVE ON"}, std::pair{false, "LOG GPGGA"},
                                         std::pair{true, "FIX POSITION"}, std::pair{true, "LOG RTCM"}}) {
        QTest::addRow("%s-cancelled-read", command) << fixed << QString::fromLatin1(command) << true << 7 << 0;
    }
    QTest::newRow("fixed-byte-reads") << true << QString() << false << 1 << 0;
    QTest::newRow("fixed-noise-across-reads") << true << QString() << false << int(GPSCommandChannel::READ_CHUNK_SIZE)
                                              << int(2 * GPSCommandChannel::READ_CHUNK_SIZE - 3);
}

void FemtoProtocolTest::_receiverMode()
{
    QFETCH(bool, fixed);
    QFETCH(QString, rejectedCommand);
    QFETCH(bool, cancelRead);
    QFETCH(int, readChunk);
    QFETCH(int, noiseBytes);
    GPSTestClock clock;
    const GPSProtocolLogCapture log;
    Receiver receiver(clock);
    receiver.model.readChunk = readChunk;
    receiver.model.noiseBytes = noiseBytes;
    const QByteArray rejected = rejectedCommand.toLatin1();
    if (cancelRead) {
        // A stop requested as the command is written: its reply is never read.
        receiver.faults.rules.push_back(afterCommand(rejected, [](ScriptedReceiver& link) { link.cancel(); }));
    } else if (!rejected.isEmpty()) {
        receiver.faults.rules.push_back(latestReply(rejected, "<ERROR\r\n"));
    }
    int cancelledReads = 0;
    auto io = receiver.io();
    io.read = [&cancelledReads, read = io.read](std::span<uint8_t> bytes, GPSDeadline deadline) {
        const auto result = read(bytes, deadline);
        cancelledReads += result.status == GPSReadStatus::Cancelled;
        return result;
    };
    GPSProtocolRuntime driver(Femto::FAMILY, std::move(io));
    GPSConfig config{};
    config.base = {
        .mode = fixed ? GPSBaseStationConfig::Mode{GPSBaseStationConfig::Fixed{
                            .position = {.latitudeDegrees = 47.0, .longitudeDegrees = 8.0, .altitudeMeters = 500.0f},
                            .accuracyMeters = 1.0f}}
                      : GPSBaseStationConfig::Mode{
                            GPSBaseStationConfig::SurveyIn{.accuracyMeters = 1.25, .duration = 60s}}};
    unsigned baudrate = 115200;
    const bool configured = driver.configure(config, baudrate);
    if (!rejectedCommand.isEmpty()) {
        QVERIFY(!configured);
        // A receiver whose base setup was rejected must not take injected corrections.
        QVERIFY(!driver.receiverReady());
        QVERIFY(receiver.sent(rejected));
        QVERIFY(!receiver.sent("LOG UAVGPSB"));
        if (cancelRead) {
            QVERIFY(log.warnings().empty());
            QCOMPARE(cancelledReads, 1);
        }
        return;
    }
    QVERIFY(configured);
    QVERIFY(log.warnings().empty());
    QVERIFY(!receiver.sent("POSAVE OFF"));
    QVERIFY(!receiver.sent("FIX NONE"));
    QVERIFY(!receiver.sent("LOG UAVGPSB"));
    QCOMPARE(receiver.sent("UNDULATION USER 0"), fixed);
    QCOMPARE(receiver.sent("FIX POSITION 47.00000000 8.00000000"), fixed);
    QCOMPARE(receiver.sent("POSAVE ON"), !fixed);
}

UT_REGISTER_TEST_LIGHTWEIGHT(FemtoProtocolTest, TestLabel::Unit)
