#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "GPSEventSink.h"
#include "GPSProtocolRuntime.h"
#include "NMEASatelliteEpoch.h"
#include "Passive/PassiveFamily.h"
#include "Support/GPSProtocolLogCapture.h"
#include "Support/GPSTestClock.h"
#include "Support/ProtocolTestPackets.h"
#include "UnitTest.h"

using namespace std::chrono_literals;

#define CHECK(condition)                          \
    do {                                          \
        if (!(condition)) {                       \
            throw std::runtime_error(#condition); \
        }                                         \
    } while (0)

namespace {
/// The passive family on the runtime, with a link that counts every operation and never delivers data.
struct Receiver
{
    explicit Receiver(bool satelliteInfoEnabled = true)
    {
        GPSRuntimeIO io;
        io.nowUs = [this] { return clock.nowUs(); };
        io.wait = [this](std::chrono::microseconds duration) {
            clock.advanceBy(static_cast<uint64_t>(duration.count()));
            return true;
        };
        io.write = [this](std::span<const uint8_t> bytes, GPSDeadline) {
            ++writes;
            return GPSWriteResult{GPSWriteStatus::Completed, int(bytes.size()), int(bytes.size())};
        };
        io.read = [this](std::span<uint8_t>, GPSDeadline deadline) {
            ++reads;
            clock.advanceTo(deadline.untilUs);
            return GPSReadResult{readStatus};
        };
        io.setBaudrate = [this](unsigned value) {
            baud = value;
            return baudStatus;
        };
        GPSRuntimeObserver observer;
        observer.decoded = [this](const GPSEventBatch& batch) {
            CHECK(batch.events.size() <= GPSEventSink::MAX_EVENTS);
            for (const auto& event : batch.events) {
                if (const auto* report = std::get_if<GPSDecodedPosition>(&event)) {
                    position = *report;
                }
            }
            events.insert(events.end(), batch.events.begin(), batch.events.end());
        };
        runtime = std::make_unique<GPSProtocolRuntime>(Passive::FAMILY, std::move(io), std::move(observer),
                                                       GPSFamilyOptions{.satelliteInfoEnabled = satelliteInfoEnabled});
    }

    Receiver(const Receiver&) = delete;
    Receiver& operator=(const Receiver&) = delete;

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

    void feed(std::string_view text)
    {
        (void) runtime->consume({reinterpret_cast<const uint8_t*>(text.data()), text.size()});
    }

    void consume(std::span<const uint8_t> bytes) { (void) runtime->consume(bytes); }

    GPSTestClock clock{1000000};
    int writes = 0;
    int reads = 0;
    unsigned baud = 0;
    GPSReadStatus readStatus = GPSReadStatus::TimedOut;
    GPSBaudStatus baudStatus = GPSBaudStatus::Configured;
    std::vector<GPSProtocolEvent> events;
    GPSDecodedPosition position;
    std::unique_ptr<GPSProtocolRuntime> runtime;
};

void configuration()
{
    const GPSProtocolLogCapture log;
    Receiver receiver;
    auto& runtime = *receiver.runtime;
    GPSConfig config;
    unsigned baud = 0;
    CHECK(!runtime.configure(config, baud));
    CHECK(receiver.baud == 0);
    baud = 115200;
    config.allowPersistentChanges = true;
    CHECK(!runtime.configure(config, baud));
    const QString refused =
        QStringLiteral("Passive input requires an explicit baud rate and no receiver configuration");
    CHECK(log.warnings() == (QStringList{refused, refused}));
    CHECK(log.categories() == (QStringList{QStringLiteral("GPS.Driver.Protocols.Passive"),
                                           QStringLiteral("GPS.Driver.Protocols.Passive")}));
    config.allowPersistentChanges = false;
    CHECK(runtime.configure(config, baud));
    CHECK(runtime.receiverReady());
    CHECK(receiver.baud == 115200);
    CHECK(runtime.receive(10ms) == GPSReceiveUpdates{});
    CHECK(runtime.error() == GPSProtocolError::None);
    receiver.readStatus = GPSReadStatus::Cancelled;
    CHECK(runtime.receive(10ms) == GPSReceiveUpdates{});
    CHECK(runtime.error() == GPSProtocolError::Cancelled);
    const auto reads = receiver.reads;
    CHECK(runtime.receive(10ms) == GPSReceiveUpdates{});
    CHECK(runtime.error() == GPSProtocolError::Cancelled);
    CHECK(receiver.reads == reads);
    CHECK(receiver.writes == 0);
    receiver.baudStatus = GPSBaudStatus::Error;
    CHECK(!runtime.configure(config, baud));
    CHECK(!runtime.receiverReady());
    CHECK(runtime.error() == GPSProtocolError::Transport);
    CHECK(log.warnings().last() == QStringLiteral("Could not set the passive input baud rate"));
    receiver.baudStatus = GPSBaudStatus::Cancelled;
    const auto warnings = log.warnings().size();
    CHECK(!runtime.configure(config, baud));
    CHECK(runtime.error() == GPSProtocolError::Cancelled);
    // A requested stop is not a link fault worth a warning.
    CHECK(log.warnings().size() == warnings);
    CHECK(receiver.writes == 0);
}

void navigation()
{
    Receiver receiver;
    const auto gga = nmeaSentence("GNGGA,123519,4807.038,N,01131.000,E,4,00,0.9,0.0,M,,M,,");
    const auto gst = nmeaSentence("GNGST,123519,0,0,0,0,0.3,0.4,0.6");
    receiver.feed(gst);
    CHECK(receiver.reports<GPSDecodedPosition>().empty());
    for (const char byte : gga) {
        receiver.feed(std::string_view(&byte, 1));
    }
    const auto fixes = receiver.reports<GPSDecodedPosition>();
    CHECK(fixes.size() == 1);
    CHECK(fixes[0].navigation.fixType == GPSPositionReport::FixType::RTKFixed);
    CHECK(std::abs(fixes[0].navigation.latitudeDegrees - 48.1173) < 1e-8);
    CHECK(fixes[0].navigation.altitudeMslMeters == 0);
    CHECK(std::isnan(fixes[0].navigation.altitudeEllipsoidMeters));
    CHECK(std::abs(fixes[0].navigation.horizontalAccuracyMeters - 0.5) < 1e-6);
    CHECK(receiver.reports<GPSDecodedSatelliteUsage>().back().usedCount == 0);
    receiver.events.clear();
    receiver.clock.advanceBy(1000000);
    receiver.feed(nmeaSentence("GNGGA,123520,4807.038,N,01131.000,E,1,,0.9,1.0,M,2.0,M,,"));
    CHECK(std::isnan(receiver.reports<GPSDecodedPosition>().back().navigation.horizontalAccuracyMeters));
    CHECK(!receiver.reports<GPSDecodedSatelliteUsage>().back().usedCount);
    const auto positionTime = receiver.position.navigation.timestampUs;
    receiver.clock.advanceBy(1000);
    receiver.feed(nmeaSentence("GNGST,123520,0,0,0,0,0.6,0.8,1.0"));
    CHECK(receiver.reports<GPSDecodedPosition>().size() == 2);
    CHECK(receiver.position.navigation.timestampUs == positionTime);
    CHECK(std::abs(receiver.position.navigation.horizontalAccuracyMeters - 1.0) < 1e-6);
    receiver.events.clear();
    auto corrupt = gga;
    corrupt[10] ^= 1;
    receiver.feed(corrupt);
    receiver.feed(gga.substr(0, 8) + "\r" + gga.substr(8));
    CHECK(receiver.reports<GPSDecodedPosition>().empty());
    receiver.feed(nmeaSentence("GNGGA,123521,,,,,0,00,0.9,,M,,M,,"));
    CHECK(receiver.reports<GPSDecodedPosition>().size() == 1);
    CHECK(receiver.position.navigation.fixType == GPSPositionReport::FixType::NoFix);
    CHECK(std::isnan(receiver.position.navigation.latitudeDegrees) &&
          std::isnan(receiver.position.navigation.longitudeDegrees));
    CHECK(receiver.reports<GPSDecodedSatelliteUsage>().back().usedCount == 0);
    receiver.events.clear();
    receiver.feed(std::string(10000, 'A') + "\n" + gga);
    CHECK(receiver.reports<GPSDecodedPosition>().size() == 1);
    CHECK(receiver.reports<GPSDecodedSurvey>().empty());
    CHECK(receiver.writes == 0 && receiver.reads == 0 && receiver.baud == 0);

    for (const auto* body :
         {"GNRMC,123522,V,,,,,,,090926,,,N", "GNGLL,,,,,123522,V", "GPGSA,A,1,,,,,,,,,,,,,1.0,0.8,0.6"}) {
        receiver.feed(gga);
        receiver.events.clear();
        receiver.feed(nmeaSentence(body));
        const auto invalid = receiver.reports<GPSDecodedPosition>();
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
        Receiver receiver(false);
        receiver.consume(std::span(binary).first(split));
        receiver.consume(std::span(binary).subspan(split));
        CHECK(receiver.reports<GPSDecodedPosition>().empty());
        CHECK(receiver.reports<GPSRTCMFrame>().size() == 1);
        const auto correction = receiver.reports<GPSRTCMFrame>().front();
        CHECK(correction.bytes ==
              QByteArray(reinterpret_cast<const char*>(binary.data()), static_cast<qsizetype>(binary.size())));
        CHECK(receiver.writes == 0 && receiver.reads == 0);
    }
    Receiver receiver(false);
    auto corrupt = binary;
    corrupt.back() ^= 1;
    receiver.consume(corrupt);
    CHECK(receiver.reports<GPSRTCMFrame>().empty());
    receiver.feed(text);
    CHECK(receiver.reports<GPSDecodedPosition>().size() == 1);

    const auto inner = rtcmPacket(std::array<uint8_t, 2>{0x3e, 0xd0});
    payload.clear();
    for (int index = 0; index < 30; ++index) {
        payload.insert(payload.end(), inner.begin(), inner.end());
    }
    auto outer = rtcmPacket(payload);
    outer.back() ^= 1;
    receiver.events.clear();
    receiver.consume(outer);
    for (int index = 0; index < 30; ++index) {
        receiver.consume({});
    }
    CHECK(receiver.reports<GPSRTCMFrame>().size() == 30);
}

void satellites()
{
    Receiver receiver;
    receiver.feed(nmeaSentence("GPGSV,2,1,05,01,10,20,30,02,20,30,40,03,30,40,50,04,40,50,60"));
    CHECK(receiver.reports<GPSDecodedSatellites>().empty());
    receiver.feed(nmeaSentence("GPGSV,2,2,05,05,50,60,70"));
    CHECK(receiver.reports<GPSDecodedSatellites>().empty());
    receiver.clock.advanceBy(NMEA::SatelliteAssembler::IDLE_TIMEOUT_US);
    receiver.consume({});
    const auto reports = receiver.reports<GPSDecodedSatellites>();
    CHECK(reports.size() == 2);
    CHECK(reports[0].constellations[0].constellation == GPSConstellation::GPS);
    CHECK(reports[0].constellations[0].inView == 5);
    CHECK(receiver.reports<GPSDecodedPosition>().empty());
    CHECK(receiver.writes == 0);
}

void satelliteEpochBoundaries()
{
    Receiver receiver;
    const auto send = [&](const char* body) { receiver.feed(nmeaSentence(body)); };
    send("GPGSV,2,1,05,01,10,20,30,02,20,30,40,03,30,40,50,04,40,50,60,1");
    send("GLGSV,1,1,01,65,10,20,30,1");
    send("GPGSV,2,2,05,05,50,60,70,1");
    send("GPGSV,1,1,01,07,10,20,45,7");
    CHECK(receiver.reports<GPSDecodedSatellites>().empty());
    send("GNRMC,120001.00,V,,,,,,,090926,,,N");
    auto reports = receiver.reports<GPSDecodedSatellites>();
    CHECK(reports.size() == 3);
    CHECK(reports[0].constellations[0].constellation == GPSConstellation::GPS &&
          reports[0].constellations[0].inView == 6);
    CHECK(reports[1].constellations[0].constellation == GPSConstellation::GLONASS &&
          reports[1].constellations[0].inView == 1);
    CHECK(reports[0].constellations[0].inViewTimestampUs == 1000000);
    receiver.events.clear();
    receiver.feed(nmeaSentence("GPGSA,A,3,01,,,,,,,,,,,,1.0,0.8,0.6"));
    receiver.clock.advanceBy(NMEA::SatelliteAssembler::IDLE_TIMEOUT_US);
    receiver.consume({});
    reports = receiver.reports<GPSDecodedSatellites>();
    CHECK(reports.size() == 2);
    CHECK(reports[0].constellations[0].inViewTimestampUs == 0);
    CHECK(reports[0].constellations[0].inUse == 1);
    CHECK(reports[1].constellations[0].inUse == 0);

    receiver.events.clear();
    send("GPGSV,2,1,05,01,10,20,30,02,20,30,40,03,30,40,50,04,40,50,60");
    send("GLGSV,1,1,01,66,10,20,30");
    receiver.clock.advanceBy(NMEA::SatelliteAssembler::IDLE_TIMEOUT_US);
    receiver.consume({});
    reports = receiver.reports<GPSDecodedSatellites>();
    CHECK(reports.size() == 1 && reports[0].constellations[0].constellation == GPSConstellation::GLONASS);
    receiver.events.clear();
    send("GPGSV,2,2,05,05,50,60,70");
    receiver.clock.advanceBy(NMEA::SatelliteAssembler::IDLE_TIMEOUT_US);
    receiver.consume({});
    CHECK(receiver.events.empty());
    send("GPGSV,1,1,00");
    receiver.clock.advanceBy(NMEA::SatelliteAssembler::IDLE_TIMEOUT_US);
    receiver.consume({});
    reports = receiver.reports<GPSDecodedSatellites>();
    CHECK(reports.size() == 2 && reports[0].constellations[0].inView == 0);

    receiver.events.clear();
    // More systems than one batch can drain: no complete epoch may overrun MAX_EVENTS.
    for (int epoch = 0; epoch < 3; ++epoch) {
        for (const char* body : {"GPGSV,1,1,01,01,10,20,30", "GLGSV,1,1,01,65,10,20,30", "GAGSV,1,1,01,01,10,20,30",
                                 "GBGSV,1,1,01,01,10,20,30", "GQGSV,1,1,01,01,10,20,30", "GIGSV,1,1,01,01,10,20,30"}) {
            send(body);
        }
        receiver.clock.advanceBy(NMEA::SatelliteAssembler::IDLE_TIMEOUT_US);
        receiver.consume({});
        receiver.consume({});
    }
    CHECK(receiver.reports<GPSDecodedSatellites>().size() == 21);
}

void satelliteBatchDeadline()
{
    Receiver receiver;
    receiver.feed(nmeaSentence("GLGSV,1,1,01,65,10,20,30"));
    for (int page = 1; page <= 10; ++page) {
        receiver.feed(
            nmeaSentence("GPGSV,64," + std::to_string(page) + ",256,01,10,20,30,02,20,30,40,03,30,40,50,04,40,50,60"));
        receiver.clock.advanceBy(100000);
    }
    CHECK(receiver.reports<GPSDecodedSatellites>().empty());
    receiver.consume({});
    const auto reports = receiver.reports<GPSDecodedSatellites>();
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
