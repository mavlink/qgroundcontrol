#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Ashtech/AshtechFamily.h"
#include "Ashtech/AshtechPlan.h"
#include "GPSEllipsoidPosition.h"
#include "GPSProtocolRuntime.h"
#include "NMEASatelliteEpoch.h"
#include "NMEAUtils.h"
#include "Support/AshtechReceiverModel.h"
#include "Support/GPSProtocolLogCapture.h"
#include "Support/GPSRuntimeTestIO.h"
#include "Support/ProtocolTestPackets.h"
#include "UnitTest.h"

using namespace std::chrono_literals;

#define CHECK(condition)                                                                                 \
    do {                                                                                                 \
        if (!(condition)) {                                                                              \
            throw std::runtime_error(std::string("line ") + std::to_string(__LINE__) + ": " #condition); \
        }                                                                                                \
    } while (0)

namespace {

using AshtechReceiver = GPSTest::AshtechReceiverModel;
using GPSTest::ASHTECH_SURVEY_FAILED;
using GPSTest::ASHTECH_SURVEY_FINISHED;

GPSRuntimeIO noDevice(GPSTestClock& clock)
{
    auto io = makeGPSRuntimeTestIO(clock);
    io.read = [](std::span<uint8_t>, GPSDeadline) -> GPSReadResult { throw std::runtime_error("decoder read device"); };
    io.write = [](std::span<const uint8_t>, GPSDeadline) -> GPSWriteResult {
        throw std::runtime_error("decoder wrote device");
    };
    io.setBaudrate = [](unsigned) -> GPSBaudStatus { throw std::runtime_error("decoder changed baudrate"); };
    return io;
}

/// The Ashtech decoder on the runtime, without a device; decoded events are kept in order.
struct Decoding
{
    explicit Decoding(GPSTestClock& clock, bool satelliteInfoEnabled = true)
        : runtime(Ashtech::FAMILY, noDevice(clock), observer(), {.satelliteInfoEnabled = satelliteInfoEnabled})
    {}

    GPSRuntimeObserver observer()
    {
        GPSRuntimeObserver result;
        result.decoded = [this](const GPSEventBatch& batch) {
            events.insert(events.end(), batch.events.begin(), batch.events.end());
        };
        return result;
    }

    GPSReceiveUpdates consume(std::span<const uint8_t> bytes) { return runtime.consume(bytes); }

    /// The decoder's working position, which also holds metadata no event has published yet.
    const GPSDecodedPosition& position()
    {
        return static_cast<Ashtech::Protocol&>(runtime.protocol()).decoder().nmea().position();
    }

    std::vector<GPSProtocolEvent> events;
    GPSProtocolRuntime runtime;
};

bool hasPosition(GPSReceiveUpdates updates)
{
    return updates.testFlag(GPSReceiveUpdate::Position);
}

bool hasSatellites(GPSReceiveUpdates updates)
{
    return updates.testFlag(GPSReceiveUpdate::Satellites);
}

int surveyFlags(const GPSDecodedSurvey& report)
{
    return (report.survey.valid ? 1 : 0) | (report.survey.active ? 2 : 0);
}

uint32_t surveyDuration(const GPSDecodedSurvey& report)
{
    return static_cast<uint32_t>(report.survey.duration.count());
}

void ashtechCommandEvidence(GPSTestClock& clock)
{
    for (const std::string command : {"$PASHS,POP,20\r\n", "$PASHS,NME,ALL,A,OFF\r\n"}) {
        for (const auto outcome :
             {GPSCommandOutcome::Acknowledged, GPSCommandOutcome::Rejected, GPSCommandOutcome::TimedOut}) {
            clock.reset(1000000);
            AshtechReceiver receiver(clock);
            if (outcome != GPSCommandOutcome::Acknowledged) {
                receiver.failedCommand = command;
                receiver.silentFailure = outcome == GPSCommandOutcome::TimedOut;
            }
            receiver.configure();
            CHECK(receiver.results.size() == receiver.commands.size());
            for (size_t index = 0; index < receiver.results.size(); ++index) {
                CHECK(receiver.results[index].evidence.command == receiver.commands[index]);
            }
            const auto result = std::find_if(receiver.results.begin(), receiver.results.end(),
                                             [&](const auto& item) { return item.evidence.command == command; });
            CHECK(result != receiver.results.end());
            CHECK(result->evidence.outcome == outcome);
            CHECK(result->evidence.acceptedBytes == int(command.size()));
            CHECK(result->evidence.writtenBytes == int(command.size()));
            CHECK(result->evidence.uncertainBytes == 0);
        }
    }
}

void ashtechSurveyReceipts(GPSTestClock& clock)
{
    clock.reset(1000000);
    AshtechReceiver receiver(clock);
    receiver.configure();
    receiver.startSurvey();
    CHECK(!receiver.surveys.empty() && surveyFlags(receiver.surveys.back()) == 2);
    receiver.surveys.clear();

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
        const auto commands = receiver.commands.size();
        (void) receiver.driver.consume(nmeaPacket(body));
        CHECK(receiver.surveys.empty());
        (void) receiver.driver.receive(1ms);
        CHECK(receiver.commands.size() == commands);
        CHECK(receiver.surveys.empty());
    }

    // A checksum-valid unrelated long frame poisons the reused buffer beyond the next short receipt.
    (void) receiver.driver.consume(nmeaPacket("XXXXX," + finished));
    (void) receiver.driver.consume(nmeaPacket("PASHR,RECEIPT,"));
    CHECK(receiver.surveys.empty());
    const auto commands = receiver.commands.size();
    (void) receiver.driver.receive(1ms);
    CHECK(receiver.commands.size() == commands);

    const auto completeFrame = nmeaPacket(finished);
    for (size_t length : {size_t{1}, size_t{15}, size_t{64}, completeFrame.size() - 4, completeFrame.size() - 3}) {
        (void) receiver.driver.consume(std::span(completeFrame).first(length));
        CHECK(receiver.surveys.empty());
        (void) receiver.driver.consume(nmeaPacket("PASHR,RECEIPT,"));
        CHECK(receiver.surveys.empty());
    }
    (void) receiver.driver.consume(completeFrame);
    CHECK(receiver.surveys.size() == 1 && surveyFlags(receiver.surveys.back()) == 1);
    CHECK(!receiver.surveys.back().survey.meanAccuracyMeters.has_value());
    CHECK(std::abs(receiver.surveys.back().survey.position.latitudeDegrees - (55 + 42.5178481 / 60)) < 1e-8);
    (void) receiver.driver.receive(1ms);
    receiver.surveys.clear();
    const auto completedCommands = receiver.commands.size();
    (void) receiver.driver.consume(completeFrame);
    (void) receiver.driver.receive(1ms);
    CHECK(receiver.surveys.empty() && receiver.commands.size() == completedCommands);

    receiver.configure();
    receiver.startSurvey();
    receiver.surveys.clear();
    (void) receiver.driver.consume(nmeaPacket("PASHR,RECEIPT,POS,AVG,100,FINISHED,114642.81,28.12.2011,"));
    clock.advanceBy(2000000);
    (void) receiver.driver.consume(nmeaPacket("GPHDT,121.2,T"));
    CHECK(receiver.surveys.size() == 1 && surveyFlags(receiver.surveys.back()) == 2);
    CHECK(surveyDuration(receiver.surveys.back()) >= 2);
    receiver.surveys.clear();
    (void) receiver.driver.consume(nmeaPacket(ASHTECH_SURVEY_FAILED));
    CHECK(receiver.surveys.size() == 1 && surveyFlags(receiver.surveys.back()) == 0);
    const auto afterFailure = receiver.commands.size();
    (void) receiver.driver.receive(1ms);
    CHECK(receiver.commands.size() == afterFailure);
    receiver.surveys.clear();
    (void) receiver.driver.consume(nmeaPacket(finished));
    (void) receiver.driver.consume(nmeaPacket(ASHTECH_SURVEY_FAILED));
    (void) receiver.driver.receive(1ms);
    CHECK(receiver.commands.size() == afterFailure);
    CHECK(receiver.surveys.empty());

    for (const auto body :
         {std::string_view{"PASHR,RECEIPT,"},
          std::string_view{"PASHR,RECEIPT,POS,AVG,STARTED,INTERVAL,,114502.56,28.12.2011"},
          std::string_view{"PASHR,RECEIPT,POS,AVG,STARTED,INTERVAL,100,,28.12.2011"}, ASHTECH_SURVEY_FAILED}) {
        receiver.configure();
        receiver.surveyReply = body;
        CHECK(!receiver.startSurvey());
        CHECK(receiver.results.back().evidence.command == "$PASHS,POS,AVG,100\r\n");
        CHECK(receiver.results.back().evidence.outcome == GPSCommandOutcome::TimedOut);
        CHECK(receiver.driver.errorDetail() == QStringLiteral("No matching Ashtech survey-start receipt"));
    }
    receiver.configure();
    receiver.surveyReply = ASHTECH_SURVEY_FINISHED;
    CHECK(!receiver.startSurvey());
    CHECK(receiver.surveys.empty());
    receiver.configure(true);
    (void) receiver.driver.consume(nmeaPacket(finished));
    CHECK(receiver.surveys.empty());
    const auto count = receiver.commands.size();
    (void) receiver.driver.receive(1ms);
    CHECK(receiver.commands.size() == count);

    const GPSProtocolLogCapture log;
    GPSConfig invalid{};
    unsigned baudrate = 115200;
    const auto calls = receiver.transportCalls;
    CHECK(!receiver.driver.configure(invalid, baudrate));
    CHECK(!receiver.driver.receiverReady());
    CHECK(receiver.transportCalls == calls);
    CHECK(log.warnings().size() == 1 &&
          log.warnings().front().startsWith(QStringLiteral("Invalid receiver physical configuration")));
    CHECK(log.categories() == QStringList{QStringLiteral("GPS.Driver.Protocols.Ashtech")});
}

void ashtechMetadata(GPSTestClock& clock)
{
    Decoding driver(clock);
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
    const auto& position = driver.position();
    const auto gga = nmeaPacket("GPGGA,123519,4700.0,N,00800.0,E,1,08,0.9,500.0,M,0,M,,");
    CHECK(hasPosition(driver.consume(gga)));
    CHECK(position.navigation.latitudeDegrees == 47.0);
    const auto received = position.navigation.timestampUs;
    CHECK(!hasPosition(driver.consume(nmeaPacket("GPGGA,123519,,N,,E,1,08,0.9,,M,,M,,"))));
    CHECK(position.navigation.timestampUs == received);
    CHECK(!hasPosition(
        driver.consume(nmeaPacket("PASHR,POS,bad,12,172814.0,3723.4,N,12202.2,W,18.9,0,90,10,0,1,1,1,1"))));
    CHECK(hasPosition(driver.consume(gga)));
    CHECK(
        hasPosition(driver.consume(nmeaPacket("PASHR,POS,2,12,172814.0,3723.4,N,12202.2,W,18.9,0,90,10,0,1,1,1,1,"))));
    CHECK(position.navigation.altitudeEllipsoidMeters == 18.9);
    CHECK(std::isnan(position.navigation.altitudeMslMeters));
    for (const std::string_view missingCoordinate : {
             "PASHR,POS,2,12,172814.0,,N,12202.2,W,18.9,0,90,10,0,1,1,1,1,",
             "PASHR,POS,2,12,172814.0,3723.4,N,,W,18.9,0,90,10,0,1,1,1,1,",
             "PASHR,POS,2,12,172814.0,3723.4,N,12202.2,W,,0,90,10,0,1,1,1,1,",
         }) {
        CHECK(hasPosition(driver.consume(nmeaPacket(missingCoordinate))));
        CHECK(position.navigation.fixType == GPSPositionReport::FixType::NoFix);
    }
    CHECK(!hasSatellites(driver.consume(nmeaPacket("GPGSV,1,1,01,01,,,"))));
    clock.advanceBy(NMEA::SatelliteAssembler::IDLE_TIMEOUT_US);
    CHECK(hasSatellites(driver.consume({})));
    CHECK(gpsSatellites().constellations[0].inView == 1);
    CHECK(!hasSatellites(driver.consume(nmeaPacket("GPGSV,1,1,01,01,0,0,0"))));
    clock.advanceBy(NMEA::SatelliteAssembler::IDLE_TIMEOUT_US);
    CHECK(hasSatellites(driver.consume({})));
    CHECK(gpsSatellites().constellations[0].inView == 1);
    CHECK(!hasPosition(driver.consume(nmeaPacket("GPZDA,172809.456,12,07,2026,00,00"))));
    CHECK(position.navigation.utcTimeUs % 1000000 >= 455999 && position.navigation.utcTimeUs % 1000000 <= 456001);

    (void) driver.consume(nmeaPacket("GPGST,172810.0,0,0,0,0,0.3,0.4,0.6"));
    (void) driver.consume(nmeaPacket("GPHDT,121.2,T"));
    const auto positionPacket = nmeaPacket("PASHR,POS,2,12,172810.0,3723.4,N,12202.2,W,18.9,0,90,10,0,1,1,1,1,");
    (void) driver.consume(positionPacket);
    CHECK(position.navigation.horizontalAccuracyMeters == 0.5f && std::isfinite(position.navigation.headingRadians));
    CHECK(position.navigation.utcTimeUs % 60000000 == 10000000);
    clock.advanceBy(6000000);
    (void) driver.consume(positionPacket);
    CHECK(std::isnan(position.navigation.horizontalAccuracyMeters) &&
          std::isnan(position.navigation.verticalAccuracyMeters) && std::isnan(position.navigation.headingRadians));
    CHECK(position.navigation.utcTimeUs == 0);
    (void) driver.consume(nmeaPacket("GPGST,172809.0,0,0,0,0,0.3,0.4,0.6"));
    (void) driver.consume(positionPacket);
    CHECK(std::isnan(position.navigation.horizontalAccuracyMeters));
    const auto positionReceipt = position.navigation.timestampUs;
    clock.advanceBy(1);
    CHECK(hasPosition(driver.consume(nmeaPacket("GPGST,172810.0,0,0,0,0,0.3,0.4,0.6"))));
    CHECK(position.navigation.horizontalAccuracyMeters == 0.5f && position.navigation.timestampUs == positionReceipt);
}

void ashtechFraming(GPSTestClock& clock)
{
    const std::string sentence = nmeaSentence("PASHR,POS,2,12,172814.0,3723.4,N,12202.2,W,18.9,0,90,10,0,1,1,1,1,");
    const auto packet = [](std::string_view text) { return std::vector<uint8_t>(text.begin(), text.end()); };
    {
        Decoding driver(clock);
        GPSReceiveUpdates updates;
        for (const uint8_t byte : packet(sentence)) {
            updates |= driver.consume({&byte, 1});
        }
        CHECK(hasPosition(updates));
        CHECK(driver.position().navigation.altitudeEllipsoidMeters == 18.9);
    }
    {
        Decoding driver(clock);
        auto corrupted = sentence;
        auto& checksum = corrupted[corrupted.find('*') + 1];
        checksum = checksum == '0' ? '1' : '0';
        CHECK(!hasPosition(driver.consume(packet(corrupted))));
        CHECK(driver.position().navigation.timestampUs == 0);
        CHECK(hasPosition(driver.consume(packet("$GPGGA,123519,47" + sentence))));
    }
    {
        Decoding driver(clock, false);
        (void) driver.consume(nmeaPacket("GPGSV,1,1,01,01,40,080,45"));
        clock.advanceBy(NMEA::SatelliteAssembler::IDLE_TIMEOUT_US);
        (void) driver.consume({});
        CHECK(std::none_of(driver.events.begin(), driver.events.end(),
                           [](const auto& event) { return std::holds_alternative<GPSDecodedSatellites>(event); }));
    }
}

void ashtechMixedFramingAndFixedCommand(GPSTestClock& clock)
{
    clock.reset(1000000);
    AshtechReceiver receiver(clock);
    receiver.configure(true);
    const auto embedded = nmeaPacket("PASHR,POS,2,12,172810.0,3723.4,N,12202.2,W,18.9,0,90,10,0,1,1,1,1,");
    const auto binary = rtcmPacket(embedded);
    (void) receiver.driver.consume(binary);
    CHECK(receiver.position.navigation.timestampUs == 0);
    CHECK(receiver.corrections == std::vector<std::vector<uint8_t>>{binary});
    verifyRTCMRecovery(receiver.driver, receiver.corrections);
    receiver.startSurvey();
    CHECK(receiver.driver.error() == GPSProtocolError::None);
    CHECK(receiver.surveys.size() == 1 && surveyFlags(receiver.surveys.back()) == 1);
    const auto fixed = std::find_if(receiver.commands.begin(), receiver.commands.end(),
                                    [](const auto& command) { return command.starts_with("$PASHS,POS,4700."); });
    CHECK(fixed != receiver.commands.end() && fixed->ends_with(",PC1\r\n"));
}

/// Coordinates are formatted as printf does: exact ties round to even, and negative zero keeps its sign.
void ashtechFixedPositionText()
{
    CHECK(Ashtech::Plan::fixedPosition(
              {.latitudeDegrees = -12.5, .longitudeDegrees = 8.25, .altitudeMeters = 100.015625f}) ==
          "$PASHS,POS,1230.00000000,S,815.00000000,E,100.01562,PC1\r\n");
    CHECK(Ashtech::Plan::fixedPosition({.latitudeDegrees = 47, .longitudeDegrees = -8, .altitudeMeters = -0.0f}) ==
          "$PASHS,POS,4700.00000000,N,800.00000000,W,-0.00000,PC1\r\n");
}

void ashtechAcknowledgementReturnsImmediately(GPSTestClock& clock)
{
    clock.reset(1000000);
    std::string reply = NMEAUtils::repairChecksum("$PASHR,PRT,A,115200").toStdString();
    std::vector<std::string> writes;
    std::vector<GPSCommandResult> completions;
    int reads = 0;
    auto io = makeGPSRuntimeTestIO(clock);
    io.write = [&](std::span<const uint8_t> bytes, GPSDeadline) -> GPSWriteResult {
        writes.emplace_back(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        clock.advanceBy(1000);
        return writes.size() == 1 ? GPSWriteResult{GPSWriteStatus::Completed, int(bytes.size()), int(bytes.size())}
                                  : GPSWriteResult{GPSWriteStatus::Cancelled};
    };
    io.read = [&](std::span<uint8_t> bytes, GPSDeadline) -> GPSReadResult {
        CHECK(!reply.empty());  // A decoded PRT response must not trigger a trailing timeout read.
        ++reads;
        const auto size = std::min<size_t>({bytes.size(), reply.size(), 4});
        std::memcpy(bytes.data(), reply.data(), size);
        reply.erase(0, size);
        clock.advanceBy(1000);
        return {GPSReadStatus::Data, static_cast<int>(size)};
    };
    GPSRuntimeObserver observer;
    observer.commandFinished = [&](const auto& result) { completions.push_back(result); };
    GPSProtocolRuntime receiver(Ashtech::FAMILY, std::move(io), std::move(observer), {.satelliteInfoEnabled = false});
    unsigned baud = 115200;
    GPSConfig config;
    std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).accuracyMeters = 1;
    std::get<GPSBaseStationConfig::SurveyIn>(config.base.mode).duration = 60s;
    CHECK(!receiver.configure(config, baud));
    CHECK(writes == (std::vector<std::string>{"$PASHQ,PRT\r\n", "$PASHQ,RID\r\n"}));
    CHECK(reads > 1);
    CHECK(completions.size() == 2);
    CHECK(completions[0].evidence.outcome == GPSCommandOutcome::Acknowledged);
    CHECK(completions[0].evidence.finishedAtUs == 1001000 + uint64_t(reads) * 1000);
    CHECK(completions[1].evidence.outcome == GPSCommandOutcome::Cancelled);
    CHECK(receiver.error() == GPSProtocolError::Cancelled);
}

}  // namespace

class GPSProtocolAshtechTest : public UnitTest
{
    Q_OBJECT

private slots:

    void _protocol();
    void _commandEvidence();
};

void GPSProtocolAshtechTest::_protocol()
{
    GPSTestClock clock;
    try {
        ashtechFixedPositionText();
        ashtechAcknowledgementReturnsImmediately(clock);
        ashtechMetadata(clock);
        ashtechFraming(clock);
        ashtechMixedFramingAndFixedCommand(clock);
        ashtechSurveyReceipts(clock);
    } catch (const std::exception& error) {
        QFAIL(error.what());
    }
}

void GPSProtocolAshtechTest::_commandEvidence()
{
    GPSTestClock clock;
    try {
        ashtechCommandEvidence(clock);
    } catch (const std::exception& error) {
        QFAIL(error.what());
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSProtocolAshtechTest, TestLabel::Unit)

#include "gps-ashtech-test.moc"
