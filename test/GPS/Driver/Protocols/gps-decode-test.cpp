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

#include "Ashtech/GPSDriverAshtech.h"
#include "CRC32.h"
#include "Femto/GPSDriverFemto.h"
#include "GPSProtocolFeatures.h"
#include "GPSProtocolTestIO.h"
#include "NMEA/GPSNMEAReport.h"
#include "NMEAFields.h"
#include "NMEASentence.h"
#include "ProtocolTestPackets.h"
#include "SBF/GPSDriverSBF.h"
#include "UnitTest.h"

#define CHECK(condition)                                                                                 \
    do {                                                                                                 \
        if (!(condition)) {                                                                              \
            throw std::runtime_error(std::string("line ") + std::to_string(__LINE__) + ": " #condition); \
        }                                                                                                \
    } while (0)

namespace {
GPSProtocolIO noDevice()
{
    auto io = makeGPSProtocolTestIO();
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
    GPSNativePositionReport report;
    CHECK(report.fix_type == GPSPositionReport::FixType::Unknown);
    for (const auto& test : cases) {
        NMEA::GGA fix;
        fix.quality = test.quality;
        applyNMEAGGA(report, fix, 123);
        CHECK(report.fix_type == test.expected);
        CHECK(report.timestamp == 123);
    }
}

#if QGC_GPS_ENABLE_SBF
std::vector<uint8_t> sbfPacket(uint16_t id, std::span<const uint8_t> payload, uint32_t tow = 0, uint16_t week = 2435)
{
    std::vector<uint8_t> packet(14 + payload.size());
    (void) LittleEndian::write<uint16_t>(packet, 0, 0x4024);
    (void) LittleEndian::write<uint16_t>(packet, 4, id);
    (void) LittleEndian::write<uint16_t>(packet, 6, uint16_t(packet.size()));
    (void) LittleEndian::write<uint32_t>(packet, 8, tow);
    (void) LittleEndian::write<uint16_t>(packet, 12, week);
    std::copy(payload.begin(), payload.end(), packet.begin() + 14);
    (void) LittleEndian::write<uint16_t>(packet, 2, crc16(packet.data() + 4, packet.size() - 4));
    return packet;
}
#endif

std::vector<uint8_t> femtoPacket(uint16_t id, std::span<const uint8_t> payload)
{
    std::vector<uint8_t> packet(28 + payload.size() + 4);
    packet[0] = 0xaa;
    packet[1] = 0x44;
    packet[2] = 0x12;
    packet[3] = 28;
    (void) LittleEndian::write<uint16_t>(packet, 4, id);
    (void) LittleEndian::write<uint16_t>(packet, 8, uint16_t(payload.size()));
    std::copy(payload.begin(), payload.end(), packet.begin() + 28);
    (void) LittleEndian::write<uint32_t>(packet, packet.size() - 4,
                                         QGC::crc32Update({packet.data(), packet.size() - 4}));
    return packet;
}

#if QGC_GPS_ENABLE_SBF || QGC_GPS_ENABLE_FEMTO
void malformedMessages()
{
    GPSNativePositionReport position{};
    GPSNativeSatelliteReport satellites{};
    const std::array<uint8_t, 2> shortPayload{};
#if QGC_GPS_ENABLE_SBF
    GPSNativeSBF sbf(noDevice(), &position, &satellites);
    sbf_payload_pvt_geodetic_t fix{};
    fix.mode_type = 1;
    fix.latitude = 0.5;
    fix.longitude = 1.0;
    fix.nr_sv = 12;
    sbf_payload_dop_t dop{};
    sbf_payload_vel_cov_geodetic_t covariance{};
    sbf.consume(sbfPacket(SBF_ID_DOP, bytes(dop)));
    sbf.consume(sbfPacket(SBF_ID_VelCovGeodetic, bytes(covariance)));
    const auto good = sbfPacket(SBF_ID_PVTGeodetic, bytes(fix));
    sbf.consume(good);
    gps_test_time += 200000;
    CHECK(sbf.consume({}) & 1);
    CHECK(std::abs(position.latitude_deg - 0.5 * GPS_RAD_TO_DEG) < 0.00001);
    const auto received = position.timestamp;
    CHECK(sbf.consume(sbfPacket(SBF_ID_PVTGeodetic, shortPayload, 1000)) == 0);
    CHECK(position.timestamp == received);
    CHECK(!(sbf.consume(good) & 1));  // Duplicate receiver epoch does not republish.

    fix.cog = -2.0e10f;
    sbf.consume(sbfPacket(SBF_ID_PVTGeodetic, bytes(fix), 1000));
    gps_test_time += 200000;
    CHECK(sbf.consume({}) & 1);
    CHECK(std::isnan(position.cog_rad));
    fix.cog = 90.0f;
    sbf.consume(sbfPacket(SBF_ID_PVTGeodetic, bytes(fix), 2000));
    gps_test_time += 200000;
    CHECK(sbf.consume({}) & 1);
    CHECK(std::abs(position.cog_rad - GPS_PI / 2) < 0.00001);

#endif
#if QGC_GPS_ENABLE_FEMTO
    GPSNativeFemto femto(noDevice(), &position, &satellites);
    femto_uav_gps_t gps{};
    gps.lat = 470000000;
    gps.lon = 80000000;
    gps.fix_type = 3;
    gps.satellites_used = 14;
    const auto valid = femtoPacket(FEMTO_MSG_ID_UAVGPS, bytes(gps));
    CHECK(femto.consume(valid) & 1);
    CHECK(position.latitude_deg == 47.0);
    gps_test_time += 1000;
    const auto femtoReceived = position.timestamp;
    CHECK(femto.consume(femtoPacket(FEMTO_MSG_ID_UAVGPS, shortPayload)) == 0);
    CHECK(position.timestamp == femtoReceived);
    CHECK(femto.consume(valid) & 1);
    for (uint8_t length : {uint8_t{0}, uint8_t{3}, uint8_t{255}}) {
        auto invalid = valid;
        invalid[3] = length;
        CHECK(femto.consume(invalid) == 0);
        CHECK(femto.consume(valid) & 1);
    }
    CHECK(femto.consume(femtoPacket(FEMTO_MSG_ID_UAVSTATUS, shortPayload)) == 0);
#endif
}
#endif

class ReadProbe : public GPSProtocol
{
public:
    using GPSProtocol::GPSProtocol;
    using GPSProtocol::read;

    int configure(unsigned&, const GPSConfig&) override { return 0; }

    int receive(unsigned) override { return 0; }

    int consume(std::span<const uint8_t>) override { return 0; }
};

void tinyReads()
{
    auto io = makeGPSProtocolTestIO();
    io.read = [](std::span<uint8_t> bytes, GPSDeadline deadline) -> GPSReadResult {
        CHECK(deadline.remainingMilliseconds(gps_test_time) == 1000);
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

std::vector<uint8_t> nmeaPacket(std::string_view body)
{
    uint8_t checksum = 0;
    std::vector<uint8_t> result{'$'};
    for (const auto byte : body) {
        result.push_back(byte);
        checksum ^= byte;
    }
    result.push_back('*');
    result.push_back(NMEAFields::hexDigit(checksum >> 4));
    result.push_back(NMEAFields::hexDigit(checksum));
    result.push_back('\r');
    result.push_back('\n');
    return result;
}

#if QGC_GPS_ENABLE_ASHTECH
constexpr std::string_view ASHTECH_SURVEY_STARTED = "PASHR,RECEIPT,POS,AVG,STARTED,INTERVAL,100,114502.56,28.12.2011";
constexpr std::string_view ASHTECH_SURVEY_FINISHED =
    "PASHR,RECEIPT,POS,AVG,100,FINISHED,114642.81,28.12.2011,5542.5178481,N,03739.2954994,E,176.334,OK,CONTINUOUS,100."
    "20";
constexpr std::string_view ASHTECH_SURVEY_FAILED = "PASHR,RECEIPT,POS,AVG,100,FINISHED,124628.01,28.12.2011,ERR";

class AshtechReceiver
{
public:
    std::vector<std::string> commands;
    std::vector<GPSCommandResult> results;
    std::vector<GPSNativeSurveyReport> surveys;
    std::string surveyReply{ASHTECH_SURVEY_STARTED};
    std::string failedCommand;
    bool silentFailure = false;
    std::vector<uint8_t> reply;
    GPSNativePositionReport position;
    size_t transportCalls = 0;
    GPSNativeAshtech driver;

    AshtechReceiver()
        : driver(io(), &position, nullptr)
    {}

    GPSProtocolIO io()
    {
        auto io = makeGPSProtocolTestIO();
        io.read = [this](std::span<uint8_t> bytes, GPSDeadline deadline) {
            ++transportCalls;
            if (reply.empty()) {
                gps_test_time += uint64_t(deadline.remainingMilliseconds(gps_test_time)) * 1000 + 1;
                return GPSReadResult{GPSReadStatus::TimedOut};
            }
            const size_t count = std::min(bytes.size(), reply.size());
            std::copy_n(reply.begin(), count, bytes.begin());
            reply.erase(reply.begin(), reply.begin() + count);
            ++gps_test_time;
            return GPSReadResult{GPSReadStatus::Data, int(count)};
        };
        io.write = [this](std::span<const uint8_t> bytes, GPSDeadline) {
            ++transportCalls;
            const std::string command(reinterpret_cast<const char*>(bytes.data()), bytes.size());
            commands.push_back(command);
            if (command == failedCommand) {
                reply = silentFailure ? std::vector<uint8_t>{} : nmeaPacket("PASHR,NAK");
            } else if (command.starts_with("$PASHQ,PRT")) {
                reply = nmeaPacket("PASHR,PRT,A,115200");
            } else if (command.starts_with("$PASHQ,RID")) {
                reply = nmeaPacket("PASHR,RID,MB2");
            } else if (command.starts_with("$PASHS,POS,AVG")) {
                reply = nmeaPacket(surveyReply);
            } else {
                reply = nmeaPacket("PASHR,ACK");
            }
            return GPSWriteResult{GPSWriteStatus::Completed, int(bytes.size()), int(bytes.size())};
        };
        io.setBaudrate = [this](unsigned) {
            ++transportCalls;
            return GPSBaudStatus::Configured;
        };
        io.commandFinished = [this](const GPSCommandResult& result) { results.push_back(result); };
        io.decoded = [this](const GPSDecodedBatch& batch) {
            for (const auto& event : batch.events) {
                if (const auto* survey = std::get_if<GPSNativeSurveyReport>(&event)) {
                    surveys.push_back(*survey);
                }
            }
        };
        return io;
    }

    void configure(GPSProtocol::OutputMode mode = GPSProtocol::OutputMode::RTCM, bool fixed = false)
    {
        GPSProtocol::GPSConfig config{};
        config.output_mode = mode;
        config.base = {.useFixedBase = fixed,
                       .surveyInAccMeters = 1,
                       .surveyInDurationSecs = 100,
                       .fixedPosition = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500}};
        unsigned baudrate = 115200;
        CHECK(driver.configure(baudrate, config) == 0);
        CHECK(driver.receiverReady());
        driver.consume({});
        surveys.clear();
    }

    int startSurvey()
    {
        driver.consume(nmeaPacket("PASHR,POS,2,12,172814.0,3723.4,N,12202.2,W,18.9,0,90,10,0,1,1,1,1,"));
        return driver.receive(1);
    }
};

void ashtechCommandEvidence()
{
    for (const std::string command : {"$PASHS,POP,20\r\n", "$PASHS,NME,ALL,A,OFF\r\n"}) {
        for (const auto outcome :
             {GPSCommandOutcome::Acknowledged, GPSCommandOutcome::Rejected, GPSCommandOutcome::TimedOut}) {
            gps_test_time = 1000000;
            AshtechReceiver receiver;
            if (outcome != GPSCommandOutcome::Acknowledged) {
                receiver.failedCommand = command;
                receiver.silentFailure = outcome == GPSCommandOutcome::TimedOut;
            }
            receiver.configure(GPSProtocol::OutputMode::GPS);
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

void ashtechSurveyReceipts()
{
    gps_test_time = 1000000;
    AshtechReceiver receiver;
    receiver.configure();
    receiver.startSurvey();
    CHECK(!receiver.surveys.empty() && receiver.surveys.back().flags == 2);
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
                                                          {"28.12.2011", "31.02.2011"},
                                                          {",OK,", ",ERR,"},
                                                          {"100.20", ""}}) {
        std::string body = finished;
        body.replace(body.find(field), field.size(), replacement);
        malformed.push_back(std::move(body));
    }
    for (const auto& body : malformed) {
        receiver.driver.consume(nmeaPacket(finished));
        CHECK(receiver.surveys.size() == 1);
        CHECK(receiver.surveys.back().flags == 1);
        CHECK(!receiver.surveys.back().accuracyKnown);
        CHECK(std::abs(receiver.surveys.back().latitude - (55 + 42.5178481 / 60)) < 1e-8);
        receiver.driver.receive(1);
        receiver.surveys.clear();
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
        receiver.driver.consume(completeFrame);
        CHECK(receiver.surveys.size() == 1 && receiver.surveys.back().flags == 1);
        receiver.driver.receive(1);
        receiver.surveys.clear();
    }

    receiver.configure();
    receiver.startSurvey();
    receiver.surveys.clear();
    receiver.driver.consume(nmeaPacket("PASHR,RECEIPT,POS,AVG,100,FINISHED,114642.81,28.12.2011,"));
    gps_test_time += 2000000;
    receiver.driver.consume(nmeaPacket("GPHDT,121.2,T"));
    CHECK(receiver.surveys.size() == 1 && receiver.surveys.back().flags == 2);
    CHECK(receiver.surveys.back().duration >= 2);
    receiver.surveys.clear();
    receiver.driver.consume(nmeaPacket(ASHTECH_SURVEY_FAILED));
    CHECK(receiver.surveys.size() == 1 && receiver.surveys.back().flags == 0);
    const auto afterFailure = receiver.commands.size();
    receiver.driver.receive(1);
    CHECK(receiver.commands.size() == afterFailure);
    receiver.surveys.clear();
    receiver.driver.consume(nmeaPacket(finished));
    receiver.driver.consume(nmeaPacket(ASHTECH_SURVEY_FAILED));
    receiver.driver.receive(1);
    CHECK(receiver.commands.size() == afterFailure);
    CHECK(receiver.surveys.size() == 2 && receiver.surveys.back().flags == 0);

    for (const auto body :
         {std::string_view{"PASHR,RECEIPT,"},
          std::string_view{"PASHR,RECEIPT,POS,AVG,STARTED,INTERVAL,,114502.56,28.12.2011"},
          std::string_view{"PASHR,RECEIPT,POS,AVG,STARTED,INTERVAL,100,,28.12.2011"}, ASHTECH_SURVEY_FAILED}) {
        receiver.configure();
        receiver.surveyReply = body;
        CHECK(receiver.startSurvey() < 0);
        CHECK(receiver.results.back().evidence.command == "$PASHS,POS,AVG,100\r\n");
        CHECK(receiver.results.back().evidence.outcome ==
              (body == ASHTECH_SURVEY_FAILED ? GPSCommandOutcome::Rejected : GPSCommandOutcome::TimedOut));
    }
    receiver.configure();
    receiver.surveyReply = ASHTECH_SURVEY_FINISHED;
    receiver.startSurvey();
    CHECK(receiver.surveys.size() == 1 && receiver.surveys.back().flags == 1);
    for (auto mode : {GPSProtocol::OutputMode::GPS, GPSProtocol::OutputMode::RTCM}) {
        receiver.configure(mode, true);
        receiver.driver.consume(nmeaPacket(finished));
        CHECK(receiver.surveys.empty());
        const auto count = receiver.commands.size();
        receiver.driver.receive(1);
        CHECK(receiver.commands.size() == count);
    }

    GPSProtocol::GPSConfig invalid{};
    invalid.output_mode = GPSProtocol::OutputMode::RTCM;
    unsigned baudrate = 115200;
    const auto calls = receiver.transportCalls;
    CHECK(receiver.driver.configure(baudrate, invalid) < 0);
    CHECK(!receiver.driver.receiverReady());
    CHECK(receiver.transportCalls == calls);
}

void ashtechMetadata()
{
    GPSNativePositionReport position{};
    GPSNativeSatelliteReport satellites{};
    auto io = noDevice();
    GPSNativeSatelliteReport gpsSatellites;
    io.decoded = [&](const GPSDecodedBatch& batch) {
        for (const auto& event : batch.events) {
            if (const auto* report = std::get_if<GPSNativeSatelliteReport>(&event);
                report && report->constellation == GPSConstellation::GPS) {
                gpsSatellites = *report;
            }
        }
    };
    GPSNativeAshtech driver(std::move(io), &position, &satellites);
    const auto gga = nmeaPacket("GPGGA,123519,4700.0,N,00800.0,E,1,08,0.9,500.0,M,0,M,,");
    CHECK(driver.consume(gga) & 1);
    CHECK(position.latitude_deg == 47.0);
    const auto received = position.timestamp;
    CHECK(driver.consume(nmeaPacket("GPGGA,123519,,N,,E,1,08,0.9,,M,,M,,")) == 0);
    CHECK(position.timestamp == received);
    CHECK(driver.consume(nmeaPacket("PASHR,POS,bad,12,172814.0,3723.4,N,12202.2,W,18.9,0,90,10,0,1,1,1,1")) == 0);
    CHECK(driver.consume(gga) & 1);
    CHECK(driver.consume(nmeaPacket("PASHR,POS,2,12,172814.0,3723.4,N,12202.2,W,18.9,0,90,10,0,1,1,1,1,")) & 1);
    CHECK(position.altitude_ellipsoid_m == 18.9);
    CHECK(std::isnan(position.altitude_msl_m));
    for (const std::string_view missingCoordinate : {
             "PASHR,POS,2,12,172814.0,,N,12202.2,W,18.9,0,90,10,0,1,1,1,1,",
             "PASHR,POS,2,12,172814.0,3723.4,N,,W,18.9,0,90,10,0,1,1,1,1,",
             "PASHR,POS,2,12,172814.0,3723.4,N,12202.2,W,,0,90,10,0,1,1,1,1,",
         }) {
        CHECK(driver.consume(nmeaPacket(missingCoordinate)) & 1);
        CHECK(position.fix_type == GPSPositionReport::FixType::NoFix);
    }
    CHECK(!(driver.consume(nmeaPacket("GPGSV,1,1,01,01,,,")) & 2));
    gps_test_time += NMEA::SatelliteAssembler::IDLE_TIMEOUT_US;
    CHECK(driver.consume({}) & 2);
    CHECK(gpsSatellites.count == 1);
    CHECK(!gpsSatellites.entries[0].signal);
    CHECK(!gpsSatellites.entries[0].azimuth);
    CHECK(!gpsSatellites.entries[0].elevation);
    CHECK(!gpsSatellites.entries[0].used);
    CHECK(!(driver.consume(nmeaPacket("GPGSV,1,1,01,01,0,0,0")) & 2));
    gps_test_time += NMEA::SatelliteAssembler::IDLE_TIMEOUT_US;
    CHECK(driver.consume({}) & 2);
    CHECK(gpsSatellites.entries[0].signal == 0);
    CHECK(gpsSatellites.entries[0].azimuth == 0);
    CHECK(gpsSatellites.entries[0].elevation == 0);
    driver.consume(nmeaPacket("GPZDA,172809.456,12,07,2026,00,00"));
    CHECK(position.time_utc_usec % 1000000 >= 455999 && position.time_utc_usec % 1000000 <= 456001);
}
#endif

void invalidFamilyConfiguration()
{
    using Config = GPSProtocol::GPSConfig;
    Config valid{};
    valid.output_mode = GPSProtocol::OutputMode::RTCM;
    valid.base = {.useFixedBase = true,
                  .surveyInAccMeters = 1,
                  .surveyInDurationSecs = 60,
                  .fixedPosition = {.latitudeDegrees = 47, .longitudeDegrees = 8, .altitudeMeters = 500},
                  .fixedBaseAccuracyMeters = 1};
    std::vector<Config> invalid;
    auto add = [&](auto member, auto value) {
        Config config = valid;
        config.base.*member = value;
        invalid.push_back(config);
    };
    const auto addPosition = [&](auto member, auto value) {
        Config config = valid;
        config.base.fixedPosition.*member = value;
        invalid.push_back(config);
    };
    for (auto member : {&GPSEllipsoidPosition::latitudeDegrees, &GPSEllipsoidPosition::longitudeDegrees}) {
        for (double value : std::array<double, 5>{NAN, INFINITY, -INFINITY, 181.0, -181.0}) {
            addPosition(member, value);
        }
    }
    addPosition(&GPSEllipsoidPosition::latitudeDegrees, 90.01);
    for (float value : {NAN, INFINITY, -INFINITY, std::numeric_limits<float>::max()}) {
        addPosition(&GPSEllipsoidPosition::altitudeMeters, value);
        add(&GPSBaseStationConfig::fixedBaseAccuracyMeters, value);
    }
    add(&GPSBaseStationConfig::fixedBaseAccuracyMeters, -1.0f);
    valid.base = {.useFixedBase = true};
    invalid.push_back(valid);
    valid.base = {.surveyInAccMeters = 1, .surveyInDurationSecs = 60};
    for (double value : std::array<double, 5>{NAN, INFINITY, -1.0, 0.0, std::numeric_limits<double>::max()}) {
        add(&GPSBaseStationConfig::surveyInAccMeters, value);
    }
    for (int64_t value : std::array<int64_t, 3>{-1, 0, int64_t(UINT32_MAX) + 1}) {
        add(&GPSBaseStationConfig::surveyInDurationSecs, value);
    }
    valid.base = {};
    invalid.push_back(valid);
    valid.output_mode = static_cast<GPSProtocol::OutputMode>(255);
    invalid.push_back(valid);

    for (const auto& config : invalid) {
        GPSNativePositionReport position{};
        std::vector<std::unique_ptr<GPSProtocol>> drivers;
#if QGC_GPS_ENABLE_ASHTECH
        drivers.push_back(std::make_unique<GPSNativeAshtech>(noDevice(), &position, nullptr));
#endif
#if QGC_GPS_ENABLE_SBF
        drivers.push_back(std::make_unique<GPSNativeSBF>(noDevice(), &position));
#endif
#if QGC_GPS_ENABLE_FEMTO
        drivers.push_back(std::make_unique<GPSNativeFemto>(noDevice(), &position));
#endif
        for (const auto& driver : drivers) {
            unsigned baudrate = 9600;
            CHECK(driver->configure(baudrate, config) < 0);
            CHECK(!driver->receiverReady());
            CHECK(baudrate == 9600);
        }
    }
}

#if QGC_GPS_ENABLE_SBF
void sbfEpochMetadata()
{
    GPSNativePositionReport position;
    GPSNativeSatelliteReport satellites;
    std::vector<GPSNativePositionReport> fixes;
    std::vector<GPSSatelliteUsageReport> usage;
    auto io = makeGPSProtocolTestIO();
    io.decoded = [&](const GPSDecodedBatch& batch) {
        for (const auto& event : batch.events) {
            if (const auto* fix = std::get_if<GPSNativePositionReport>(&event)) {
                fixes.push_back(*fix);
            }
            if (const auto* count = std::get_if<GPSSatelliteUsageReport>(&event)) {
                usage.push_back(*count);
            }
        }
    };
    GPSNativeSBF driver(io, &position, &satellites);
    sbf_payload_pvt_geodetic_t fix{};
    fix.mode_type = 1;
    fix.mode_2d = 1;
    fix.latitude = 0.5;
    fix.longitude = 1;
    fix.height = 10;
    fix.nr_sv = UINT8_MAX;
    fix.h_accuracy = UINT16_MAX;
    sbf_payload_att_euler heading{};
    heading.heading = 90;
    heading.mode = 2;
    sbf_payload_att_cov_euler accuracy{};
    accuracy.cov_headhead = 4;
    sbf_payload_vel_cov_geodetic_t speed{};
    speed.cov_vn_vn = 9;
    sbf_payload_dop_t dop{};
    dop.hDOP = 125;
    driver.consume(sbfPacket(SBF_ID_AttCovEuler, bytes(accuracy), 1000));
    driver.consume(sbfPacket(SBF_ID_PVTGeodetic, bytes(fix), 1000));
    driver.consume(sbfPacket(SBF_ID_DOP, bytes(dop), 2000));  // Adjacent receiver epochs must remain independent.
    driver.consume(sbfPacket(SBF_ID_AttEuler, bytes(heading), 1000));
    driver.consume(sbfPacket(SBF_ID_VelCovGeodetic, bytes(speed), 1000));
    CHECK(fixes.empty());
    gps_test_time += 200000;
    driver.consume({});
    CHECK(fixes.size() == 1);
    CHECK(fixes[0].fix_type == GPSPositionReport::FixType::Fix2D);
    CHECK(fixes[0].satellites_used == UINT8_MAX);
    CHECK(usage.size() == 1 && !usage[0].usedCount);
    CHECK(std::isnan(fixes[0].hdop));
    CHECK(std::isnan(fixes[0].eph));
    CHECK(std::abs(fixes[0].heading_accuracy * GPS_RAD_TO_DEG - 2) < 1e-5);
    CHECK(std::abs(fixes[0].heading * GPS_RAD_TO_DEG - 90) < 1e-5);
    CHECK(fixes[0].speedAccuracyMetersPerSecond == 3);
    CHECK(fixes[0].time_utc_usec == 0);  // GNSS time cannot be labeled UTC without a receiver UTC offset.
    fix.mode_2d = 0;
    fix.nr_sv = 0;
    driver.consume(sbfPacket(SBF_ID_PVTGeodetic, bytes(fix), 3000));
    gps_test_time += 200000;
    driver.consume({});
    CHECK(fixes.size() == 2);
    CHECK(fixes.back().satellites_used == 0);
    CHECK(fixes.back().fix_type == GPSPositionReport::FixType::Fix3D);
    CHECK(std::isnan(fixes.back().heading));
    CHECK(std::isnan(fixes.back().heading_accuracy));
    CHECK(std::isnan(fixes.back().speedAccuracyMetersPerSecond));
    CHECK(driver.consume(sbfPacket(SBF_ID_PVTGeodetic, bytes(fix), UINT32_MAX)) == 0);
    CHECK(driver.consume(sbfPacket(SBF_ID_PVTGeodetic, bytes(fix), 4000, UINT16_MAX)) == 0);
    gps_test_time += 200000;
    driver.consume({});
    CHECK(fixes.size() == 2);
}

void sbfInvalidCoordinates()
{
    GPSNativePositionReport position;
    GPSNativeSatelliteReport satellites;
    std::vector<GPSNativePositionReport> reports;
    auto io = noDevice();
    io.decoded = [&](const GPSDecodedBatch& batch) {
        for (const auto& event : batch.events) {
            if (const auto* report = std::get_if<GPSNativePositionReport>(&event)) {
                reports.push_back(*report);
            }
        }
    };
    GPSNativeSBF driver(io, &position, &satellites);
    sbf_payload_pvt_geodetic_t fix{};
    fix.mode_type = 1;
    fix.latitude = 0.5;
    fix.longitude = 1;
    fix.height = 10;
    uint32_t tow = 1000;
    auto check = [&](const auto& payload) {
        const auto previous = reports.size();
        driver.consume(sbfPacket(SBF_ID_PVTGeodetic, payload, tow));
        tow += 1000;
        gps_test_time += 200000;
        driver.consume({});
        CHECK(reports.size() == previous + 1);
        CHECK(reports.back().fix_type == GPSPositionReport::FixType::NoFix);
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
#endif

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
    gps_test_time = 0;
    gps_test_warnings.clear();
    try {
        tinyReads();
        absoluteDeadline();
        invalidFamilyConfiguration();
        nmeaFixQualities();
#if QGC_GPS_ENABLE_SBF || QGC_GPS_ENABLE_FEMTO
        malformedMessages();
#endif
#if QGC_GPS_ENABLE_SBF
        sbfEpochMetadata();
        sbfInvalidCoordinates();
#endif
#if QGC_GPS_ENABLE_ASHTECH
        ashtechMetadata();
        ashtechSurveyReceipts();
#endif
    } catch (const std::exception& error) {
        QFAIL(error.what());
    }
}

void GPSProtocolDecodeTest::_commandEvidence()
{
#if QGC_GPS_ENABLE_ASHTECH
    gps_test_warnings.clear();
    try {
        ashtechCommandEvidence();
    } catch (const std::exception& error) {
        QFAIL(error.what());
    }
#else
    QSKIP("Ashtech protocol is disabled");
#endif
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSProtocolDecodeTest, TestLabel::Unit)

#include "gps-decode-test.moc"
