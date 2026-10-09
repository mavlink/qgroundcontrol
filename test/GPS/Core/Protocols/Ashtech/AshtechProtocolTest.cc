#include "AshtechProtocolTest.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "Ashtech/AshtechPlan.h"
#include "GPSProtocolRuntime.h"
#include "GPSReceiverConfig.h"
#include "GPSReceiverFamilies.h"
#include "NMEASatellites.h"
#include "NMEASentence.h"
#include "Protocols/Support/AshtechReceiverModel.h"
#include "Protocols/Support/GPSProtocolTestBase.h"
#include "Protocols/Support/ReceiverBench.h"

using namespace std::chrono_literals;
using namespace GPSTest;

namespace {

/// An MB-Two on the runtime, configured as a survey-in or fixed base.
struct Base
{
    explicit Base(GPSTestClock& clock)
        : peer(clock)
        , driver(Ashtech::FAMILY, peer.io(), peer.observer())
    {}

    /// Configures the receiver and settles the replies; false when it does not become ready.
    [[nodiscard]] bool configure(bool fixed = false)
    {
        GPSConfig config{};
        config.base = {
            .mode = fixed ? GPSBaseStationConfig::Mode{GPSBaseStationConfig::Fixed{
                                .position = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500}}}
                          : GPSBaseStationConfig::Mode{GPSBaseStationConfig::SurveyIn{
                                .accuracyMeters = 1, .duration = std::chrono::seconds{100}}}};
        unsigned baudrate = 115200;
        if (!driver.configure(config, baudrate) || !driver.receiverReady()) {
            return false;
        }
        (void) driver.consume({});
        clearSurveys();
        return true;
    }

    bool startSurvey()
    {
        (void) driver.consume(nmeaPacket("PASHR,POS,2,12,172814.0,3723.4,N,12202.2,W,18.9,0,90,10,0,1,1,1,1,"));
        (void) driver.receive(std::chrono::milliseconds{1});
        return driver.error() == GPSProtocolError::None;
    }

    std::vector<GPSSurveyReport> surveys() const { return peer.log.reports<GPSSurveyReport>(); }

    void clearSurveys() { peer.log.clear<GPSSurveyReport>(); }

    ModelReceiver<AshtechReceiverModel> peer;
    GPSProtocolRuntime driver;
};

bool hasPosition(GPSReceiveUpdates updates)
{
    return updates.testFlag(GPSReceiveUpdate::Position);
}

bool hasSatellites(GPSReceiveUpdates updates)
{
    return updates.testFlag(GPSReceiveUpdate::Satellites);
}

void ashtechSurveyReceipts(GPSTestClock& clock)
{
    clock.reset(GPSTestClock::START_US);
    Base receiver(clock);
    QVERIFY(receiver.configure());
    QVERIFY(receiver.startSurvey());
    QVERIFY(!receiver.surveys().empty() && surveyFlags(receiver.surveys().back()) == 2);
    receiver.clearSurveys();

    std::vector<std::string> malformed{"PASHR,RECEIPT,", "PASHR,RECEIPT,POS,AVG,100,FINISHED",
                                       "PASHR,RECEIPT,POS,AVG,100,FINISHED,114642.81,28.12.2011,"};
    const std::string finished{ASHTECH_SURVEY_FINISHED};
    for (size_t end = finished.find(','); end != std::string::npos; end = finished.find(',', end + 1)) {
        malformed.push_back(finished.substr(0, end));
    }
    for (const auto& [field, replacement] :
         std::vector<std::pair<std::string, std::string>>{{"5542.5178481", ""},
                                                          {"5542.5178481", "5560.0"},
                                                          {"5542.5178481", "9100.0"},
                                                          {"03739.2954994", "18100.0"},
                                                          {"03739.2954994", "nan"},
                                                          {",N,", ",Q,"},
                                                          {",E,", ",N,"},
                                                          {"176.334", ""},
                                                          {"176.334", "inf"},
                                                          {"114642.81", ""},
                                                          {"114642.81", "246000.00"},
                                                          {"114642.81", "114402.00"},
                                                          {"28.12.2011", "31.02.2011"},
                                                          {"28.12.2011", "27.12.2011"},
                                                          {"100,FINISHED", "99,FINISHED"},
                                                          {",OK,", ",ERR,"},
                                                          {"100.20", ""}}) {
        std::string body = finished;
        body.replace(body.find(field), field.size(), replacement);
        malformed.push_back(std::move(body));
    }
    for (const auto& body : malformed) {
        const auto commands = receiver.peer.commands().size();
        (void) receiver.driver.consume(nmeaPacket(body));
        QVERIFY(receiver.surveys().empty());
        (void) receiver.driver.receive(1ms);
        QCOMPARE(receiver.peer.commands().size(), commands);
        QVERIFY(receiver.surveys().empty());
    }

    // A checksum-valid unrelated long frame poisons the reused buffer beyond the next short receipt.
    (void) receiver.driver.consume(nmeaPacket("XXXXX," + finished));
    (void) receiver.driver.consume(nmeaPacket("PASHR,RECEIPT,"));
    QVERIFY(receiver.surveys().empty());
    const auto commands = receiver.peer.commands().size();
    (void) receiver.driver.receive(1ms);
    QCOMPARE(receiver.peer.commands().size(), commands);

    const auto completeFrame = nmeaPacket(finished);
    for (size_t length : {size_t{1}, size_t{15}, size_t{64}, completeFrame.size() - 4, completeFrame.size() - 3}) {
        (void) receiver.driver.consume(std::span(completeFrame).first(length));
        QVERIFY(receiver.surveys().empty());
        (void) receiver.driver.consume(nmeaPacket("PASHR,RECEIPT,"));
        QVERIFY(receiver.surveys().empty());
    }
    (void) receiver.driver.consume(completeFrame);
    QVERIFY(receiver.surveys().size() == 1 && surveyFlags(receiver.surveys().back()) == 1);
    QVERIFY(!receiver.surveys().back().meanAccuracyMeters.has_value());
    QVERIFY(std::abs(receiver.surveys().back().position.latitudeDegrees - (55 + 42.5178481 / 60)) < 1e-8);
    (void) receiver.driver.receive(1ms);
    receiver.clearSurveys();
    const auto completedCommands = receiver.peer.commands().size();
    (void) receiver.driver.consume(completeFrame);
    (void) receiver.driver.receive(1ms);
    QVERIFY(receiver.surveys().empty() && receiver.peer.commands().size() == completedCommands);

    QVERIFY(receiver.configure());
    QVERIFY(receiver.startSurvey());
    receiver.clearSurveys();
    (void) receiver.driver.consume(nmeaPacket("PASHR,RECEIPT,POS,AVG,100,FINISHED,114642.81,28.12.2011,"));
    clock.advanceBy(2000000);
    (void) receiver.driver.consume(nmeaPacket("GPHDT,121.2,T"));
    QVERIFY(receiver.surveys().size() == 1 && surveyFlags(receiver.surveys().back()) == 2);
    QCOMPARE_GE(surveyDuration(receiver.surveys().back()), 2);
    receiver.clearSurveys();
    (void) receiver.driver.consume(nmeaPacket(ASHTECH_SURVEY_FAILED));
    QVERIFY(receiver.surveys().size() == 1 && surveyFlags(receiver.surveys().back()) == 0);
    const auto afterFailure = receiver.peer.commands().size();
    (void) receiver.driver.receive(1ms);
    QCOMPARE(receiver.peer.commands().size(), afterFailure);
    receiver.clearSurveys();
    (void) receiver.driver.consume(nmeaPacket(finished));
    (void) receiver.driver.consume(nmeaPacket(ASHTECH_SURVEY_FAILED));
    (void) receiver.driver.receive(1ms);
    QCOMPARE(receiver.peer.commands().size(), afterFailure);
    QVERIFY(receiver.surveys().empty());

    for (const auto body :
         {std::string_view{"PASHR,RECEIPT,"},
          std::string_view{"PASHR,RECEIPT,POS,AVG,STARTED,INTERVAL,,114502.56,28.12.2011"},
          std::string_view{"PASHR,RECEIPT,POS,AVG,STARTED,INTERVAL,100,,28.12.2011"}, ASHTECH_SURVEY_FAILED}) {
        QVERIFY(receiver.configure());
        receiver.peer.model.surveyReply = body;
        QVERIFY(!receiver.startSurvey());
        QCOMPARE(receiver.peer.log.commands.back().command, QByteArray("$PASHS,POS,AVG,100"));
        QCOMPARE(receiver.peer.log.commands.back().outcome, GPSCommandOutcome::TimedOut);
        QCOMPARE(receiver.driver.errorDetail(), QStringLiteral("No matching Ashtech survey-start receipt"));
    }
    QVERIFY(receiver.configure());
    receiver.peer.model.surveyReply = ASHTECH_SURVEY_FINISHED;
    QVERIFY(!receiver.startSurvey());
    QVERIFY(receiver.surveys().empty());
    QVERIFY(receiver.configure(true));
    (void) receiver.driver.consume(nmeaPacket(finished));
    QVERIFY(receiver.surveys().empty());
    const auto count = receiver.peer.commands().size();
    (void) receiver.driver.receive(1ms);
    QCOMPARE(receiver.peer.commands().size(), count);
}

void ashtechMetadata(GPSTestClock& clock)
{
    LoggedRuntime driver(Ashtech::FAMILY, makeDecoderOnlyIO(clock));
    const auto gpsSatellites = [&driver] {
        GPSDecodedSatellites latest;
        for (const auto& event : driver.events) {
            if (const auto* report = std::get_if<GPSDecodedSatellites>(&event);
                report && report->count && report->constellations[0].constellation == GPSConstellation::GPS) {
                latest = *report;
            }
        }
        return latest;
    };
    const auto gga = nmeaPacket("GPGGA,123519,4700.0,N,00800.0,E,1,08,0.9,500.0,M,0,M,,");
    QVERIFY(hasPosition(driver->consume(gga)));
    QCOMPARE_EQ(driver.position.navigation.latitudeDegrees, 47.0);
    const auto received = driver.position.navigation.timestampUs;
    QVERIFY(!hasPosition(driver->consume(nmeaPacket("GPGGA,123519,,N,,E,1,08,0.9,,M,,M,,"))));
    QCOMPARE(driver.position.navigation.timestampUs, received);
    QVERIFY(!hasPosition(
        driver->consume(nmeaPacket("PASHR,POS,bad,12,172814.0,3723.4,N,12202.2,W,18.9,0,90,10,0,1,1,1,1"))));
    QVERIFY(hasPosition(driver->consume(gga)));
    QVERIFY(
        hasPosition(driver->consume(nmeaPacket("PASHR,POS,2,12,172814.0,3723.4,N,12202.2,W,18.9,0,90,10,0,1,1,1,1,"))));
    QCOMPARE_EQ(driver.position.navigation.altitudeEllipsoidMeters, 18.9);
    QVERIFY(std::isnan(driver.position.navigation.altitudeMslMeters));
    for (const std::string_view missingCoordinate : {
             "PASHR,POS,2,12,172814.0,,N,12202.2,W,18.9,0,90,10,0,1,1,1,1,",
             "PASHR,POS,2,12,172814.0,3723.4,N,,W,18.9,0,90,10,0,1,1,1,1,",
             "PASHR,POS,2,12,172814.0,3723.4,N,12202.2,W,,0,90,10,0,1,1,1,1,",
         }) {
        QVERIFY(hasPosition(driver->consume(nmeaPacket(missingCoordinate))));
        QCOMPARE(driver.position.navigation.fixType, GPSPositionReport::FixType::NoFix);
    }
    QVERIFY(!hasSatellites(driver->consume(nmeaPacket("GPGSV,1,1,01,01,,,"))));
    clock.advanceBy(NMEA::SatelliteAssembler::IDLE_TIMEOUT);
    QVERIFY(hasSatellites(driver->consume({})));
    QCOMPARE(gpsSatellites().constellations[0].inView, 1);
    QVERIFY(!hasSatellites(driver->consume(nmeaPacket("GPGSV,1,1,01,01,0,0,0"))));
    clock.advanceBy(NMEA::SatelliteAssembler::IDLE_TIMEOUT);
    QVERIFY(hasSatellites(driver->consume({})));
    QCOMPARE(gpsSatellites().constellations[0].inView, 1);
    // ZDA dates the positions that follow.
    QVERIFY(!hasPosition(driver->consume(nmeaPacket("GPZDA,172809.456,12,07,2026,00,00"))));

    (void) driver->consume(nmeaPacket("GPGST,172810.0,0,0,0,0,0.3,0.4,0.6"));
    const auto positionPacket = nmeaPacket("PASHR,POS,2,12,172810.0,3723.4,N,12202.2,W,18.9,0,90,10,0,1,1,1,1,");
    (void) driver->consume(positionPacket);
    QCOMPARE_EQ(driver.position.navigation.horizontalAccuracyMeters, 0.5f);
    QCOMPARE(driver.position.navigation.utcTimeUs % 60000000, 10000000);
    clock.advanceBy(6000000);
    (void) driver->consume(positionPacket);
    QVERIFY(std::isnan(driver.position.navigation.horizontalAccuracyMeters) &&
            std::isnan(driver.position.navigation.verticalAccuracyMeters));
    QCOMPARE(driver.position.navigation.utcTimeUs, 0);
    (void) driver->consume(nmeaPacket("GPGST,172809.0,0,0,0,0,0.3,0.4,0.6"));
    (void) driver->consume(positionPacket);
    QVERIFY(std::isnan(driver.position.navigation.horizontalAccuracyMeters));
    const auto positionReceipt = driver.position.navigation.timestampUs;
    clock.advanceBy(1);
    QVERIFY(hasPosition(driver->consume(nmeaPacket("GPGST,172810.0,0,0,0,0,0.3,0.4,0.6"))));
    QVERIFY(driver.position.navigation.horizontalAccuracyMeters == 0.5f &&
            driver.position.navigation.timestampUs == positionReceipt);
}

void ashtechFraming(GPSTestClock& clock)
{
    const std::string sentence = nmeaSentence("PASHR,POS,2,12,172814.0,3723.4,N,12202.2,W,18.9,0,90,10,0,1,1,1,1,");
    const auto packet = [](std::string_view text) { return std::vector<uint8_t>(text.begin(), text.end()); };
    {
        LoggedRuntime driver(Ashtech::FAMILY, makeDecoderOnlyIO(clock));
        GPSReceiveUpdates updates;
        for (const uint8_t byte : packet(sentence)) {
            updates |= driver->consume({&byte, 1});
        }
        QVERIFY(hasPosition(updates));
        QCOMPARE_EQ(driver.position.navigation.altitudeEllipsoidMeters, 18.9);
    }
    {
        LoggedRuntime driver(Ashtech::FAMILY, makeDecoderOnlyIO(clock));
        auto corrupted = sentence;
        auto& checksum = corrupted[corrupted.find('*') + 1];
        checksum = checksum == '0' ? '1' : '0';
        QVERIFY(!hasPosition(driver->consume(packet(corrupted))));
        QCOMPARE(driver.position.navigation.timestampUs, 0);
        QVERIFY(hasPosition(driver->consume(packet("$GPGGA,123519,47" + sentence))));
    }
}

/// Coordinates are formatted as printf does: exact ties round to even, and negative zero keeps its sign.
void ashtechFixedPositionText(GPSTestClock&)
{
    QCOMPARE(Ashtech::Plan::fixedPosition(
                 {.latitudeDegrees = -12.5, .longitudeDegrees = 8.25, .altitudeMeters = 100.015625f}),
             "$PASHS,POS,1230.00000000,S,815.00000000,E,100.01562,PC1\r\n");
    QCOMPARE(Ashtech::Plan::fixedPosition({.latitudeDegrees = 47, .longitudeDegrees = -8, .altitudeMeters = -0.0f}),
             "$PASHS,POS,4700.00000000,N,800.00000000,W,-0.00000,PC1\r\n");
}

void ashtechAcknowledgementReturnsImmediately(GPSTestClock& clock)
{
    clock.reset(GPSTestClock::START_US);
    std::string reply = NMEAUtils::repairChecksum("$PASHR,PRT,A,115200").toStdString();
    std::vector<std::string> writes;
    std::vector<GPSConfigurationEvidence> completions;
    int reads = 0;
    auto io = makeGPSRuntimeTestIO(clock);
    io.write = [&](std::span<const uint8_t> bytes, GPSDeadline) -> GPSWriteResult {
        writes.emplace_back(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        clock.advanceBy(1000);
        return writes.size() == 1 ? GPSWriteResult{GPSWriteStatus::Completed, int(bytes.size()), int(bytes.size())}
                                  : GPSWriteResult{GPSWriteStatus::Cancelled};
    };
    bool readAfterReply = false;
    io.read = [&](std::span<uint8_t> bytes, GPSDeadline) -> GPSReadResult {
        readAfterReply |= reply.empty();
        ++reads;
        const auto size = std::min<size_t>({bytes.size(), reply.size(), 4});
        std::memcpy(bytes.data(), reply.data(), size);
        reply.erase(0, size);
        clock.advanceBy(1000);
        return {GPSReadStatus::Data, static_cast<int>(size)};
    };
    GPSRuntimeObserver observer;
    observer.commandFinished = [&](const auto& result) { completions.push_back(result); };
    GPSProtocolRuntime receiver(Ashtech::FAMILY, std::move(io), std::move(observer));
    unsigned baud = 115200;
    GPSConfig config;
    std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).accuracyMeters = 1;
    std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).duration = 60s;
    QVERIFY(!receiver.configure(config, baud));
    // A decoded PRT response must not trigger a trailing timeout read.
    QVERIFY(!readAfterReply);
    QCOMPARE(writes, (std::vector<std::string>{"$PASHQ,PRT\r\n", "$PASHQ,RID\r\n"}));
    QVERIFY(reads > 1);
    QCOMPARE(completions.size(), 2);
    QCOMPARE(completions[0].outcome, GPSCommandOutcome::Acknowledged);
    QCOMPARE(completions[1].outcome, GPSCommandOutcome::Cancelled);
    QCOMPARE(receiver.error(), GPSProtocolError::Cancelled);
}

void ashtechOtherBoardIsNotABase(GPSTestClock& clock)
{
    Base receiver(clock);
    receiver.peer.model.board = "BD982";
    QVERIFY(!receiver.configure());
    QCOMPARE(receiver.driver.errorDetail(),
             QStringLiteral("Trimble BD982 cannot run as an RTK base station; only the MB-Two can"));
    QVERIFY(!receiver.driver.receiverReady());
    // Identified without changing its output.
    QCOMPARE(receiver.peer.commands().back(), "$PASHQ,RID\r\n");
}

constexpr GPSTest::ProtocolScenario SCENARIOS[] = {
    {"fixed-position-text", ashtechFixedPositionText},
    {"acknowledgement-returns-immediately", ashtechAcknowledgementReturnsImmediately},
    {"metadata", ashtechMetadata},
    {"framing", ashtechFraming},
    {"survey-receipts", ashtechSurveyReceipts},
    {"other-board-is-not-a-base", ashtechOtherBoardIsNotABase},
};
}  // namespace

void AshtechProtocolTest::_scenario_data()
{
    addScenarioRows(SCENARIOS);
}

void AshtechProtocolTest::_scenario()
{
    runScenario(SCENARIOS);
}

UT_REGISTER_TEST_LIGHTWEIGHT(AshtechProtocolTest, TestLabel::Unit)
