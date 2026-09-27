#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

#include "Ashtech/AshtechProtocol.h"
#include "Femto/FemtoProtocol.h"
#include "GPSNMEAReport.h"
#include "GPSProtocolTestIO.h"
#include "NMEAFields.h"
#include "NMEASentence.h"
#include "ProtocolTestPackets.h"
#include "SBF/SBFProtocol.h"
#include "Support/AshtechReceiverModel.h"
#include "UnitTest.h"

#define CHECK(condition)                                                                                 \
    do {                                                                                                 \
        if (!(condition)) {                                                                              \
            throw std::runtime_error(std::string("line ") + std::to_string(__LINE__) + ": " #condition); \
        }                                                                                                \
    } while (0)

namespace {
GPSProtocolIO noDevice(GPSTestClock& clock)
{
    auto io = makeGPSProtocolTestIO(clock);
    io.read = [](std::span<uint8_t>, GPSDeadline) -> GPSReadResult { throw std::runtime_error("decoder read device"); };
    io.write = [](std::span<const uint8_t>, GPSDeadline) -> GPSWriteResult {
        throw std::runtime_error("decoder wrote device");
    };
    io.setBaudrate = [](unsigned) -> GPSBaudStatus { throw std::runtime_error("decoder changed baudrate"); };
    return io;
}

void nmeaFixQualities()
{
    struct Case
    {
        unsigned quality;
        GPSPositionReport::FixType expected;
    };

    constexpr std::array cases{
        Case{NMEA::GgaQuality::INVALID, GPSPositionReport::FixType::NoFix},
        Case{NMEA::GgaQuality::GPS, GPSPositionReport::FixType::Fix3D},
        Case{NMEA::GgaQuality::DIFFERENTIAL, GPSPositionReport::FixType::Differential},
        Case{NMEA::GgaQuality::RTK_FIXED, GPSPositionReport::FixType::RTKFixed},
        Case{NMEA::GgaQuality::RTK_FLOAT, GPSPositionReport::FixType::RTKFloat},
        Case{NMEA::GgaQuality::ESTIMATED, GPSPositionReport::FixType::Extrapolated},
        Case{99, GPSPositionReport::FixType::Unknown},
    };
    GPSDecodedPosition report;
    CHECK(report.navigation.fixType == GPSPositionReport::FixType::Unknown);
    for (const auto& test : cases) {
        NMEA::GGA fix;
        fix.quality = test.quality;
        applyNMEAGGA(report, fix, 123);
        CHECK(report.navigation.fixType == test.expected);
        CHECK(report.navigation.timestampUs == 123);
    }
}

void malformedMessages(GPSTestClock& clock)
{
    GPSDecodedPosition position{};
    GPSDecodedSatellites satellites{};
    const std::array<uint8_t, 2> shortPayload{};
    SBFProtocol sbf(captureGPSReports(noDevice(clock), position, &satellites));
    sbf_payload_pvt_geodetic_t fix{};
    fix.mode_type = 1;
    fix.latitude = 0.5;
    fix.longitude = 1.0;
    fix.nr_sv = 12;
    const auto good = sbfBlock(SBF_ID_PVTGeodetic, bytes(fix));
    sbf.consume(good);
    clock.advanceBy(200000);
    CHECK(sbf.consume({}) & 1);
    CHECK(std::abs(position.navigation.latitudeDegrees - 0.5 * GPS_RAD_TO_DEG) < 0.00001);
    const auto received = position.navigation.timestampUs;
    CHECK(sbf.consume(sbfBlock(SBF_ID_PVTGeodetic, shortPayload, 1000)) == 0);
    CHECK(position.navigation.timestampUs == received);
    CHECK(!(sbf.consume(good) & 1));  // Duplicate receiver epoch does not republish.

    fix.cog = -2.0e10f;
    sbf.consume(sbfBlock(SBF_ID_PVTGeodetic, bytes(fix), 1000));
    clock.advanceBy(200000);
    CHECK(sbf.consume({}) & 1);
    CHECK(std::isnan(position.navigation.courseRadians));
    fix.cog = 90.0f;
    sbf.consume(sbfBlock(SBF_ID_PVTGeodetic, bytes(fix), 2000));
    clock.advanceBy(200000);
    CHECK(sbf.consume({}) & 1);
    CHECK(std::abs(position.navigation.courseRadians - GPS_PI / 2) < 0.00001);
}

class ReadProbe : public GPSProtocol
{
public:
    using GPSProtocol::GPSProtocol;
    using GPSProtocol::read;

    bool configure(unsigned&, const GPSConfig&) override { return true; }

    int receive(unsigned) override { return 0; }

    int consume(std::span<const uint8_t>) override { return 0; }
};

void tinyReads(GPSTestClock& clock)
{
    auto io = makeGPSProtocolTestIO(clock);
    io.read = [&clock](std::span<uint8_t> bytes, GPSDeadline deadline) -> GPSReadResult {
        CHECK(deadline.remainingMilliseconds(clock.nowUs()) == 1000);
        CHECK(!bytes.empty() && bytes.size() <= 3);
        std::fill(bytes.begin(), bytes.end(), 0x12);
        return {GPSReadStatus::Data, int(bytes.size())};
    };
    ReadProbe probe(io);
    for (int capacity = 0; capacity <= 3; ++capacity) {
        std::array<uint8_t, 5> guarded{0xab, 0xab, 0xab, 0xab, 0xab};
        CHECK(probe.read(guarded.data() + 1, capacity, 1000) == capacity);
        CHECK(guarded.front() == 0xab);
        CHECK(guarded[capacity + 1] == 0xab);
    }
}

class DeadlineProbe : public ReadProbe
{
public:
    using ReadProbe::ReadProbe;

    int transaction()
    {
        const Operation outer(*this, 10);
        const Operation inner(*this, 1000);
        uint8_t byte{};
        if (read(&byte, 1, 1000) != 1) {
            return -1;
        }
        return read(&byte, 1, 1000);
    }
};

void absoluteDeadline()
{
    uint64_t now = 1000000;
    GPSProtocolIO io;
    io.nowUs = [&] { return now; };
    io.read = [&](auto buffer, GPSDeadline deadline) {
        CHECK(deadline.untilUs == 1010000);
        if (now == 1000000) {
            CHECK(deadline.remainingMilliseconds(now) == 10);
            now += 9000;
            buffer[0] = 0;
            return GPSReadResult{GPSReadStatus::Data, 1};
        }
        CHECK(deadline.remainingMilliseconds(now) == 1);
        now = deadline.untilUs;
        return GPSReadResult{GPSReadStatus::TimedOut};
    };
    DeadlineProbe probe(io);
    CHECK(probe.transaction() == 0);
    CHECK(now == 1010000);
}

using AshtechReceiver = GPSTest::AshtechReceiverModel;
using GPSTest::ASHTECH_SURVEY_FAILED;
using GPSTest::ASHTECH_SURVEY_FINISHED;

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
        receiver.driver.consume(nmeaPacket(body));
        CHECK(receiver.surveys.empty());
        receiver.driver.receive(1);
        CHECK(receiver.commands.size() == commands);
        CHECK(receiver.surveys.empty());
    }

    // A checksum-valid unrelated long frame poisons the reused buffer beyond the next short receipt.
    receiver.driver.consume(nmeaPacket("XXXXX," + finished));
    receiver.driver.consume(nmeaPacket("PASHR,RECEIPT,"));
    CHECK(receiver.surveys.empty());
    const auto commands = receiver.commands.size();
    receiver.driver.receive(1);
    CHECK(receiver.commands.size() == commands);

    const auto completeFrame = nmeaPacket(finished);
    for (size_t length : {size_t{1}, size_t{15}, size_t{64}, completeFrame.size() - 4, completeFrame.size() - 3}) {
        receiver.driver.consume(std::span(completeFrame).first(length));
        CHECK(receiver.surveys.empty());
        receiver.driver.consume(nmeaPacket("PASHR,RECEIPT,"));
        CHECK(receiver.surveys.empty());
    }
    receiver.driver.consume(completeFrame);
    CHECK(receiver.surveys.size() == 1 && surveyFlags(receiver.surveys.back()) == 1);
    CHECK(!receiver.surveys.back().survey.meanAccuracyMeters.has_value());
    CHECK(std::abs(receiver.surveys.back().survey.position.latitudeDegrees - (55 + 42.5178481 / 60)) < 1e-8);
    receiver.driver.receive(1);
    receiver.surveys.clear();
    const auto completedCommands = receiver.commands.size();
    receiver.driver.consume(completeFrame);
    receiver.driver.receive(1);
    CHECK(receiver.surveys.empty() && receiver.commands.size() == completedCommands);

    receiver.configure();
    receiver.startSurvey();
    receiver.surveys.clear();
    receiver.driver.consume(nmeaPacket("PASHR,RECEIPT,POS,AVG,100,FINISHED,114642.81,28.12.2011,"));
    clock.advanceBy(2000000);
    receiver.driver.consume(nmeaPacket("GPHDT,121.2,T"));
    CHECK(receiver.surveys.size() == 1 && surveyFlags(receiver.surveys.back()) == 2);
    CHECK(surveyDuration(receiver.surveys.back()) >= 2);
    receiver.surveys.clear();
    receiver.driver.consume(nmeaPacket(ASHTECH_SURVEY_FAILED));
    CHECK(receiver.surveys.size() == 1 && surveyFlags(receiver.surveys.back()) == 0);
    const auto afterFailure = receiver.commands.size();
    receiver.driver.receive(1);
    CHECK(receiver.commands.size() == afterFailure);
    receiver.surveys.clear();
    receiver.driver.consume(nmeaPacket(finished));
    receiver.driver.consume(nmeaPacket(ASHTECH_SURVEY_FAILED));
    receiver.driver.receive(1);
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
    }
    receiver.configure();
    receiver.surveyReply = ASHTECH_SURVEY_FINISHED;
    CHECK(!receiver.startSurvey());
    CHECK(receiver.surveys.empty());
    receiver.configure(true);
    receiver.driver.consume(nmeaPacket(finished));
    CHECK(receiver.surveys.empty());
    const auto count = receiver.commands.size();
    receiver.driver.receive(1);
    CHECK(receiver.commands.size() == count);

    GPSProtocol::GPSConfig invalid{};
    unsigned baudrate = 115200;
    const auto calls = receiver.transportCalls;
    CHECK(!receiver.driver.configure(baudrate, invalid));
    CHECK(!receiver.driver.receiverReady());
    CHECK(receiver.transportCalls == calls);
}

void ashtechMetadata(GPSTestClock& clock)
{
    auto io = noDevice(clock);
    GPSDecodedSatellites gpsSatellites;
    io.decoded = [&](const GPSDecodedBatch& batch) {
        for (const auto& event : batch.events) {
            if (const auto* report = std::get_if<GPSDecodedSatellites>(&event);
                report && report->count && report->constellations[0].constellation == GPSConstellation::GPS) {
                gpsSatellites = *report;
            }
        }
    };
    GPSProtocolTestProbe<AshtechProtocol> driver(std::move(io));
    const auto& position = driver.workingPosition();
    const auto gga = nmeaPacket("GPGGA,123519,4700.0,N,00800.0,E,1,08,0.9,500.0,M,0,M,,");
    CHECK(driver.consume(gga) & 1);
    CHECK(position.navigation.latitudeDegrees == 47.0);
    const auto received = position.navigation.timestampUs;
    CHECK(!(driver.consume(nmeaPacket("GPGGA,123519,,N,,E,1,08,0.9,,M,,M,,")) & GPSDecodedBatch::POSITION_UPDATE));
    CHECK(position.navigation.timestampUs == received);
    CHECK(!(driver.consume(nmeaPacket("PASHR,POS,bad,12,172814.0,3723.4,N,12202.2,W,18.9,0,90,10,0,1,1,1,1")) &
            GPSDecodedBatch::POSITION_UPDATE));
    CHECK(driver.consume(gga) & 1);
    CHECK(driver.consume(nmeaPacket("PASHR,POS,2,12,172814.0,3723.4,N,12202.2,W,18.9,0,90,10,0,1,1,1,1,")) & 1);
    CHECK(position.navigation.altitudeEllipsoidMeters == 18.9);
    CHECK(std::isnan(position.navigation.altitudeMslMeters));
    for (const std::string_view missingCoordinate : {
             "PASHR,POS,2,12,172814.0,,N,12202.2,W,18.9,0,90,10,0,1,1,1,1,",
             "PASHR,POS,2,12,172814.0,3723.4,N,,W,18.9,0,90,10,0,1,1,1,1,",
             "PASHR,POS,2,12,172814.0,3723.4,N,12202.2,W,,0,90,10,0,1,1,1,1,",
         }) {
        CHECK(driver.consume(nmeaPacket(missingCoordinate)) & 1);
        CHECK(position.navigation.fixType == GPSPositionReport::FixType::NoFix);
    }
    CHECK(!(driver.consume(nmeaPacket("GPGSV,1,1,01,01,,,")) & GPSDecodedBatch::SATELLITES_UPDATE));
    clock.advanceBy(NMEA::SatelliteAssembler::IDLE_TIMEOUT_US);
    CHECK(driver.consume({}) & GPSDecodedBatch::SATELLITES_UPDATE);
    CHECK(gpsSatellites.constellations[0].inView == 1);
    CHECK(!(driver.consume(nmeaPacket("GPGSV,1,1,01,01,0,0,0")) & GPSDecodedBatch::SATELLITES_UPDATE));
    clock.advanceBy(NMEA::SatelliteAssembler::IDLE_TIMEOUT_US);
    CHECK(driver.consume({}) & GPSDecodedBatch::SATELLITES_UPDATE);
    CHECK(gpsSatellites.constellations[0].inView == 1);
    CHECK(!(driver.consume(nmeaPacket("GPZDA,172809.456,12,07,2026,00,00")) & 1));
    CHECK(position.navigation.utcTimeUs % 1000000 >= 455999 && position.navigation.utcTimeUs % 1000000 <= 456001);

    driver.consume(nmeaPacket("GPGST,172810.0,0,0,0,0,0.3,0.4,0.6"));
    driver.consume(nmeaPacket("GPHDT,121.2,T"));
    const auto positionPacket = nmeaPacket("PASHR,POS,2,12,172810.0,3723.4,N,12202.2,W,18.9,0,90,10,0,1,1,1,1,");
    driver.consume(positionPacket);
    CHECK(position.navigation.horizontalAccuracyMeters == 0.5f && std::isfinite(position.navigation.headingRadians));
    CHECK(position.navigation.utcTimeUs % 60000000 == 10000000);
    clock.advanceBy(6000000);
    driver.consume(positionPacket);
    CHECK(std::isnan(position.navigation.horizontalAccuracyMeters) &&
          std::isnan(position.navigation.verticalAccuracyMeters) && std::isnan(position.navigation.headingRadians));
    CHECK(position.navigation.utcTimeUs == 0);
    driver.consume(nmeaPacket("GPGST,172809.0,0,0,0,0,0.3,0.4,0.6"));
    driver.consume(positionPacket);
    CHECK(std::isnan(position.navigation.horizontalAccuracyMeters));
    const auto positionReceipt = position.navigation.timestampUs;
    clock.advanceBy(1);
    CHECK(driver.consume(nmeaPacket("GPGST,172810.0,0,0,0,0,0.3,0.4,0.6")) & 1);
    CHECK(position.navigation.horizontalAccuracyMeters == 0.5f && position.navigation.timestampUs == positionReceipt);
}

void ashtechFraming(GPSTestClock& clock)
{
    const std::string sentence = nmeaSentence("PASHR,POS,2,12,172814.0,3723.4,N,12202.2,W,18.9,0,90,10,0,1,1,1,1,");
    const auto packet = [](std::string_view text) { return std::vector<uint8_t>(text.begin(), text.end()); };
    {
        GPSProtocolTestProbe<AshtechProtocol> driver(noDevice(clock));
        int updates = 0;
        for (const uint8_t byte : packet(sentence)) {
            updates |= driver.consume({&byte, 1});
        }
        CHECK(updates & GPSDecodedBatch::POSITION_UPDATE);
        CHECK(driver.workingPosition().navigation.altitudeEllipsoidMeters == 18.9);
    }
    {
        GPSProtocolTestProbe<AshtechProtocol> driver(noDevice(clock));
        auto corrupted = sentence;
        auto& checksum = corrupted[corrupted.find('*') + 1];
        checksum = checksum == '0' ? '1' : '0';
        CHECK(!(driver.consume(packet(corrupted)) & GPSDecodedBatch::POSITION_UPDATE));
        CHECK(driver.workingPosition().navigation.timestampUs == 0);
        CHECK(driver.consume(packet("$GPGGA,123519,47" + sentence)) & GPSDecodedBatch::POSITION_UPDATE);
    }
    {
        size_t satelliteReports = 0;
        auto io = noDevice(clock);
        io.decoded = [&](const GPSDecodedBatch& batch) {
            for (const auto& event : batch.events) {
                satelliteReports += std::holds_alternative<GPSDecodedSatellites>(event);
            }
        };
        AshtechProtocol driver(std::move(io), false);
        driver.consume(nmeaPacket("GPGSV,1,1,01,01,40,080,45"));
        clock.advanceBy(NMEA::SatelliteAssembler::IDLE_TIMEOUT_US);
        driver.consume({});
        CHECK(satelliteReports == 0);
    }
}

void ashtechMixedFramingAndFixedCommand(GPSTestClock& clock)
{
    clock.reset(1000000);
    AshtechReceiver receiver(clock);
    receiver.configure(true);
    const auto embedded = nmeaPacket("PASHR,POS,2,12,172810.0,3723.4,N,12202.2,W,18.9,0,90,10,0,1,1,1,1,");
    const auto binary = rtcmPacket(embedded);
    receiver.driver.consume(binary);
    CHECK(receiver.position.navigation.timestampUs == 0);
    CHECK(receiver.corrections == std::vector<std::vector<uint8_t>>{binary});
    verifyRTCMRecovery(receiver.driver, receiver.corrections);
    receiver.startSurvey();
    CHECK(!receiver.driver.hasIOError());
    CHECK(receiver.surveys.size() == 1 && surveyFlags(receiver.surveys.back()) == 1);
    const auto fixed = std::find_if(receiver.commands.begin(), receiver.commands.end(),
                                    [](const auto& command) { return command.starts_with("$PASHS,POS,4700."); });
    CHECK(fixed != receiver.commands.end() && fixed->ends_with(",PC1\r\n"));
}

void sbfEpochMetadata(GPSTestClock& clock)
{
    GPSDecodedPosition position;
    GPSDecodedSatellites satellites;
    std::vector<GPSDecodedPosition> fixes;
    std::vector<GPSDecodedSatelliteUsage> usage;
    auto io = makeGPSProtocolTestIO(clock);
    io.decoded = [&](const GPSDecodedBatch& batch) {
        for (const auto& event : batch.events) {
            if (const auto* fix = std::get_if<GPSDecodedPosition>(&event)) {
                fixes.push_back(*fix);
            }
            if (const auto* count = std::get_if<GPSDecodedSatelliteUsage>(&event)) {
                usage.push_back(*count);
            }
        }
    };
    SBFProtocol driver(captureGPSReports(io, position, &satellites));
    sbf_payload_pvt_geodetic_t fix{};
    fix.mode_type = 1;
    fix.mode_2d = 1;
    fix.latitude = 0.5;
    fix.longitude = 1;
    fix.height = 10;
    fix.nr_sv = UINT8_MAX;
    fix.h_accuracy = UINT16_MAX;
    driver.consume(sbfBlock(SBF_ID_PVTGeodetic, bytes(fix), 1000));
    CHECK(fixes.empty());
    clock.advanceBy(200000);
    driver.consume({});
    CHECK(fixes.size() == 1);
    CHECK(fixes[0].navigation.fixType == GPSPositionReport::FixType::Fix2D);
    CHECK(fixes[0].navigation.satellitesUsed == UINT8_MAX);
    CHECK(usage.size() == 1 && !usage[0].usedCount);
    CHECK(std::isnan(fixes[0].navigation.horizontalDop));
    CHECK(std::isnan(fixes[0].navigation.horizontalAccuracyMeters));
    CHECK(fixes[0].navigation.utcTimeUs == 0);  // GNSS time cannot be labeled UTC without a receiver UTC offset.
    fix.mode_2d = 0;
    fix.nr_sv = 0;
    driver.consume(sbfBlock(SBF_ID_PVTGeodetic, bytes(fix), 3000));
    clock.advanceBy(200000);
    driver.consume({});
    CHECK(fixes.size() == 2);
    CHECK(fixes.back().navigation.satellitesUsed == 0);
    CHECK(fixes.back().navigation.fixType == GPSPositionReport::FixType::Fix3D);
    CHECK(driver.consume(sbfBlock(SBF_ID_PVTGeodetic, bytes(fix), UINT32_MAX)) == 0);
    CHECK(driver.consume(sbfBlock(SBF_ID_PVTGeodetic, bytes(fix), 4000, UINT16_MAX)) == 0);
    clock.advanceBy(200000);
    driver.consume({});
    CHECK(fixes.size() == 2);
}

void sbfInvalidCoordinates(GPSTestClock& clock)
{
    GPSDecodedPosition position;
    GPSDecodedSatellites satellites;
    std::vector<GPSDecodedPosition> reports;
    auto io = noDevice(clock);
    io.decoded = [&](const GPSDecodedBatch& batch) {
        for (const auto& event : batch.events) {
            if (const auto* report = std::get_if<GPSDecodedPosition>(&event)) {
                reports.push_back(*report);
            }
        }
    };
    SBFProtocol driver(captureGPSReports(io, position, &satellites));
    sbf_payload_pvt_geodetic_t fix{};
    fix.mode_type = 1;
    fix.latitude = 0.5;
    fix.longitude = 1;
    fix.height = 10;
    uint32_t tow = 1000;
    auto check = [&](const auto& payload) {
        const auto previous = reports.size();
        driver.consume(sbfBlock(SBF_ID_PVTGeodetic, payload, tow));
        tow += 1000;
        clock.advanceBy(200000);
        driver.consume({});
        CHECK(reports.size() == previous + 1);
        CHECK(reports.back().navigation.fixType == GPSPositionReport::FixType::NoFix);
    };

    struct InvalidCoordinate
    {
        size_t offset;
        double value;
    };

    for (const auto& invalid : std::array{
             InvalidCoordinate{2, NAN},
             InvalidCoordinate{2, 4},
             InvalidCoordinate{10, NAN},
             InvalidCoordinate{10, 4},
             InvalidCoordinate{18, NAN},
             InvalidCoordinate{18, 1e11},
         }) {
        auto payload = bytes(fix);
        CHECK(LittleEndian::write(payload, invalid.offset, invalid.value));
        check(payload);
    }
    for (const float invalid : {NAN, 1e11f}) {
        auto payload = bytes(fix);
        CHECK(LittleEndian::write(payload, 26, invalid));
        check(payload);
    }
}

}  // namespace

class GPSProtocolDecodeTest : public UnitTest
{
    Q_OBJECT

private slots:

    void _protocol();
    void _commandEvidence();
};

void GPSProtocolDecodeTest::_protocol()
{
    GPSTestClock clock;
    try {
        tinyReads(clock);
        absoluteDeadline();
        nmeaFixQualities();
        malformedMessages(clock);
        sbfEpochMetadata(clock);
        sbfInvalidCoordinates(clock);
        ashtechMetadata(clock);
        ashtechFraming(clock);
        ashtechMixedFramingAndFixedCommand(clock);
        ashtechSurveyReceipts(clock);
    } catch (const std::exception& error) {
        QFAIL(error.what());
    }
}

void GPSProtocolDecodeTest::_commandEvidence()
{
    GPSTestClock clock;
    try {
        ashtechCommandEvidence(clock);
    } catch (const std::exception& error) {
        QFAIL(error.what());
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSProtocolDecodeTest, TestLabel::Unit)

#include "gps-decode-test.moc"
