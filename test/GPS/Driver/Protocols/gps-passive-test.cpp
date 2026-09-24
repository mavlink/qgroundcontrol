#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

#include "GPSAsciiProtocol.h"
#include "GPSProtocolTestIO.h"
#include "ProtocolTestPackets.h"
#include "UnitTest.h"

#define CHECK(condition)                          \
    do {                                          \
        if (!(condition)) {                       \
            throw std::runtime_error(#condition); \
        }                                         \
    } while (0)

namespace {
struct Receiver
{
    uint64_t clock = 1000000;
    int writes = 0;
    int reads = 0;
    unsigned baud = 0;
    GPSReadStatus readStatus = GPSReadStatus::TimedOut;
    GPSBaudStatus baudStatus = GPSBaudStatus::Configured;
    std::vector<GPSDecodedEvent> events;
    GPSNativePositionReport position;
    GPSNativeSatelliteReport satellites;

    GPSProtocolIO io()
    {
        auto io = makeGPSProtocolTestIO();
        io.nowUs = [&] { return clock; };
        io.write = [&](std::span<const uint8_t> bytes, GPSDeadline) {
            ++writes;
            return GPSWriteResult{GPSWriteStatus::Completed, int(bytes.size()), int(bytes.size())};
        };
        io.read = [&](std::span<uint8_t>, GPSDeadline deadline) {
            ++reads;
            clock = deadline.untilUs;
            return GPSReadResult{readStatus};
        };
        io.setBaudrate = [&](unsigned value) {
            baud = value;
            return baudStatus;
        };
        io.decoded = [&](const GPSDecodedBatch& batch) {
            CHECK(batch.events.size() <= GPSDecodedBatch::MAX_EVENTS);
            events.insert(events.end(), batch.events.begin(), batch.events.end());
        };
        return captureGPSReports(std::move(io), position, &satellites);
    }

    template <typename T>
    std::vector<T> reports() const
    {
        std::vector<T> result;
        for (const auto& event : events) {
            if (const auto* value = std::get_if<T>(&event)) {
                result.push_back(*value);
            }
        }
        return result;
    }
};

void feed(GPSNativePassive& driver, std::string_view text)
{
    driver.consume({reinterpret_cast<const uint8_t*>(text.data()), text.size()});
}

void configuration()
{
    Receiver receiver;
    GPSNativePassive driver(receiver.io());
    GPSProtocol::GPSConfig config;
    unsigned baud = 0;
    CHECK(driver.configure(baud, config) < 0);
    CHECK(receiver.baud == 0);
    baud = 115200;
    config.allowPersistentChanges = true;
    CHECK(driver.configure(baud, config) < 0);
    config.allowPersistentChanges = false;
    CHECK(driver.configure(baud, config) == 0);
    CHECK(driver.receiverReady());
    CHECK(receiver.baud == 115200);
    CHECK(driver.receive(10) == -1);
    CHECK(driver.ioError() == 0);
    receiver.readStatus = GPSReadStatus::Cancelled;
    CHECK(driver.receive(10) == GPSProtocol::ReadCancelled);
    const auto reads = receiver.reads;
    CHECK(driver.receive(10) == GPSProtocol::ReadCancelled);
    CHECK(receiver.reads == reads);
    CHECK(receiver.writes == 0);
    receiver.baudStatus = GPSBaudStatus::Error;
    CHECK(driver.configure(baud, config) < 0);
    CHECK(!driver.receiverReady());
    CHECK(receiver.writes == 0);
}

void navigation()
{
    Receiver receiver;
    GPSNativePassive driver(receiver.io());
    const auto gga = nmeaSentence("GNGGA,123519,4807.038,N,01131.000,E,4,00,0.9,0.0,M,,M,,");
    const auto gst = nmeaSentence("GNGST,123519,0,0,0,0,0.3,0.4,0.6");
    feed(driver, gst);
    CHECK(receiver.reports<GPSNativePositionReport>().empty());
    for (const char byte : gga) {
        feed(driver, std::string_view(&byte, 1));
    }
    const auto fixes = receiver.reports<GPSNativePositionReport>();
    CHECK(fixes.size() == 1);
    CHECK(fixes[0].navigation.fixType == GPSPositionReport::FixType::RTKFixed);
    CHECK(std::abs(fixes[0].navigation.latitudeDegrees - 48.1173) < 1e-8);
    CHECK(fixes[0].navigation.altitudeMslMeters == 0);
    CHECK(std::isnan(fixes[0].navigation.altitudeEllipsoidMeters));
    CHECK(std::abs(fixes[0].navigation.horizontalAccuracyMeters - 0.5) < 1e-6);
    CHECK(receiver.reports<GPSNativeSatelliteUsageReport>().back().usedCount == 0);
    receiver.events.clear();
    receiver.clock += 1000000;
    feed(driver, nmeaSentence("GNGGA,123520,4807.038,N,01131.000,E,1,,0.9,1.0,M,2.0,M,,"));
    CHECK(std::isnan(receiver.reports<GPSNativePositionReport>().back().navigation.horizontalAccuracyMeters));
    CHECK(!receiver.reports<GPSNativeSatelliteUsageReport>().back().usedCount);
    const auto positionTime = receiver.position.navigation.timestampUs;
    receiver.clock += 1000;
    feed(driver, nmeaSentence("GNGST,123520,0,0,0,0,0.6,0.8,1.0"));
    CHECK(receiver.reports<GPSNativePositionReport>().size() == 2);
    CHECK(receiver.position.navigation.timestampUs == positionTime);
    CHECK(std::abs(receiver.position.navigation.horizontalAccuracyMeters - 1.0) < 1e-6);
    receiver.events.clear();
    auto corrupt = gga;
    corrupt[10] ^= 1;
    feed(driver, corrupt);
    feed(driver, gga.substr(0, 8) + "\r" + gga.substr(8));
    CHECK(receiver.reports<GPSNativePositionReport>().empty());
    feed(driver, nmeaSentence("GNGGA,123521,,,,,0,00,0.9,,M,,M,,"));
    CHECK(receiver.reports<GPSNativePositionReport>().size() == 1);
    CHECK(receiver.position.navigation.fixType == GPSPositionReport::FixType::NoFix);
    CHECK(std::isnan(receiver.position.navigation.latitudeDegrees) &&
          std::isnan(receiver.position.navigation.longitudeDegrees));
    CHECK(receiver.reports<GPSNativeSatelliteUsageReport>().back().usedCount == 0);
    receiver.events.clear();
    feed(driver, std::string(10000, 'A') + "\n" + gga);
    CHECK(receiver.reports<GPSNativePositionReport>().size() == 1);
    CHECK(receiver.reports<GPSNativeSurveyReport>().empty());
    CHECK(receiver.writes == 0 && receiver.reads == 0 && receiver.baud == 0);

    for (const auto* body :
         {"GNRMC,123522,V,,,,,,,090926,,,N", "GNGLL,,,,,123522,V", "GPGSA,A,1,,,,,,,,,,,,,1.0,0.8,0.6"}) {
        feed(driver, gga);
        receiver.events.clear();
        feed(driver, nmeaSentence(body));
        const auto invalid = receiver.reports<GPSNativePositionReport>();
        CHECK(invalid.size() == 1 && invalid.front().navigation.fixType == GPSPositionReport::FixType::NoFix);
        CHECK(std::isnan(invalid.front().navigation.latitudeDegrees) &&
              std::isnan(invalid.front().navigation.horizontalAccuracyMeters));
    }
}

void corrections()
{
    const auto text = nmeaSentence("GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,");
    std::vector<uint8_t> payload{0x3e, 0xd0};
    payload.insert(payload.end(), text.begin(), text.end());
    const auto binary = rtcmPacket(payload);
    for (size_t split = 0; split <= binary.size(); ++split) {
        Receiver receiver;
        GPSNativePassive driver(receiver.io(), false);
        driver.consume(std::span(binary).first(split));
        driver.consume(std::span(binary).subspan(split));
        CHECK(receiver.reports<GPSNativePositionReport>().empty());
        CHECK(receiver.reports<GPSRTCMReport>().size() == 1);
        const auto correction = receiver.reports<GPSRTCMReport>().front();
        CHECK(correction.size == binary.size());
        CHECK(std::equal(binary.begin(), binary.end(), correction.bytes.begin()));
        CHECK(receiver.writes == 0 && receiver.reads == 0);
    }
    Receiver receiver;
    GPSNativePassive driver(receiver.io(), false);
    auto corrupt = binary;
    corrupt.back() ^= 1;
    driver.consume(corrupt);
    CHECK(receiver.reports<GPSRTCMReport>().empty());
    feed(driver, text);
    CHECK(receiver.reports<GPSNativePositionReport>().size() == 1);

    const auto inner = rtcmPacket(std::array<uint8_t, 2>{0x3e, 0xd0});
    payload.clear();
    for (int index = 0; index < 30; ++index) {
        payload.insert(payload.end(), inner.begin(), inner.end());
    }
    auto outer = rtcmPacket(payload);
    outer.back() ^= 1;
    receiver.events.clear();
    driver.consume(outer);
    for (int index = 0; index < 30; ++index) {
        driver.consume({});
    }
    CHECK(receiver.reports<GPSRTCMReport>().size() == 30);
}

void satellites()
{
    Receiver receiver;
    GPSNativePassive driver(receiver.io());
    feed(driver, nmeaSentence("GPGSV,2,1,05,01,10,20,30,02,20,30,40,03,30,40,50,04,40,50,60"));
    CHECK(receiver.reports<GPSNativeSatelliteReport>().empty());
    feed(driver, nmeaSentence("GPGSV,2,2,05,05,50,60,70"));
    CHECK(receiver.reports<GPSNativeSatelliteReport>().empty());
    receiver.clock += NMEA::SatelliteAssembler::IDLE_TIMEOUT_US;
    driver.consume({});
    const auto reports = receiver.reports<GPSNativeSatelliteReport>();
    CHECK(reports.size() == 2);
    CHECK(reports[0].constellations[0].constellation == GPSConstellation::GPS);
    CHECK(reports[0].constellations[0].inView == 5);
    CHECK(receiver.reports<GPSNativePositionReport>().empty());
    CHECK(receiver.writes == 0);
}

void satelliteEpochBoundaries()
{
    Receiver receiver;
    GPSNativePassive driver(receiver.io());
    const auto send = [&](const char* body) { feed(driver, nmeaSentence(body)); };
    send("GPGSV,2,1,05,01,10,20,30,02,20,30,40,03,30,40,50,04,40,50,60,1");
    send("GLGSV,1,1,01,65,10,20,30,1");
    send("GPGSV,2,2,05,05,50,60,70,1");
    send("GPGSV,1,1,01,07,10,20,45,7");
    CHECK(receiver.reports<GPSNativeSatelliteReport>().empty());
    send("GNRMC,120001.00,V,,,,,,,090926,,,N");
    auto reports = receiver.reports<GPSNativeSatelliteReport>();
    CHECK(reports.size() == 3);
    CHECK(reports[0].constellations[0].constellation == GPSConstellation::GPS &&
          reports[0].constellations[0].inView == 6);
    CHECK(reports[1].constellations[0].constellation == GPSConstellation::GLONASS &&
          reports[1].constellations[0].inView == 1);
    CHECK(reports[0].constellations[0].inViewTimestampUs == 1000000);
    receiver.events.clear();
    feed(driver, nmeaSentence("GPGSA,A,3,01,,,,,,,,,,,,1.0,0.8,0.6"));
    receiver.clock += NMEA::SatelliteAssembler::IDLE_TIMEOUT_US;
    driver.consume({});
    reports = receiver.reports<GPSNativeSatelliteReport>();
    CHECK(reports.size() == 2);
    CHECK(reports[0].constellations[0].inViewTimestampUs == 0);
    CHECK(reports[0].constellations[0].inUse == 1);
    CHECK(reports[1].constellations[0].inUse == 0);

    receiver.events.clear();
    send("GPGSV,2,1,05,01,10,20,30,02,20,30,40,03,30,40,50,04,40,50,60");
    send("GLGSV,1,1,01,66,10,20,30");
    receiver.clock += NMEA::SatelliteAssembler::IDLE_TIMEOUT_US;
    driver.consume({});
    reports = receiver.reports<GPSNativeSatelliteReport>();
    CHECK(reports.size() == 1 && reports[0].constellations[0].constellation == GPSConstellation::GLONASS);
    receiver.events.clear();
    send("GPGSV,2,2,05,05,50,60,70");
    receiver.clock += NMEA::SatelliteAssembler::IDLE_TIMEOUT_US;
    driver.consume({});
    CHECK(receiver.events.empty());
    send("GPGSV,1,1,00");
    receiver.clock += NMEA::SatelliteAssembler::IDLE_TIMEOUT_US;
    driver.consume({});
    reports = receiver.reports<GPSNativeSatelliteReport>();
    CHECK(reports.size() == 2 && reports[0].constellations[0].inView == 0);

    receiver.events.clear();
    // More systems than one batch can drain: no complete epoch may overrun MAX_EVENTS.
    for (int epoch = 0; epoch < 3; ++epoch) {
        for (const char* body : {"GPGSV,1,1,01,01,10,20,30", "GLGSV,1,1,01,65,10,20,30", "GAGSV,1,1,01,01,10,20,30",
                                 "GBGSV,1,1,01,01,10,20,30", "GQGSV,1,1,01,01,10,20,30", "GIGSV,1,1,01,01,10,20,30"}) {
            send(body);
        }
        receiver.clock += NMEA::SatelliteAssembler::IDLE_TIMEOUT_US;
        driver.consume({});
        driver.consume({});
    }
    CHECK(receiver.reports<GPSNativeSatelliteReport>().size() == 21);
}

void satelliteBatchDeadline()
{
    Receiver receiver;
    GPSNativePassive driver(receiver.io());
    feed(driver, nmeaSentence("GLGSV,1,1,01,65,10,20,30"));
    for (int page = 1; page <= 10; ++page) {
        feed(driver,
             nmeaSentence("GPGSV,64," + std::to_string(page) + ",256,01,10,20,30,02,20,30,40,03,30,40,50,04,40,50,60"));
        receiver.clock += 100000;
    }
    CHECK(receiver.reports<GPSNativeSatelliteReport>().empty());
    driver.consume({});
    const auto reports = receiver.reports<GPSNativeSatelliteReport>();
    CHECK(reports.size() == 1);
    CHECK(reports[0].constellations[0].constellation == GPSConstellation::GLONASS);
    CHECK(reports[0].constellations[0].inViewTimestampUs == 1000000);
}
}  // namespace

class GPSProtocolPassiveTest : public UnitTest
{
    Q_OBJECT

private slots:

    void _protocol();
};

void GPSProtocolPassiveTest::_protocol()
{
    gps_test_time = 0;
    gps_test_warnings.clear();
    try {
        configuration();
        navigation();
        corrections();
        satellites();
        satelliteEpochBoundaries();
        satelliteBatchDeadline();
    } catch (const std::exception& error) {
        QFAIL(error.what());
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSProtocolPassiveTest, TestLabel::Unit)

#include "gps-passive-test.moc"
