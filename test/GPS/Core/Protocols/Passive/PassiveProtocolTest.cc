#include "PassiveProtocolTest.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <string>
#include <vector>

#include "GPSProtocolRuntime.h"
#include "GPSReceiverFamilies.h"
#include "NMEASatellites.h"
#include "Protocols/Support/GPSProtocolTestBase.h"
#include "Protocols/Support/ReceiverBench.h"
#include "Protocols/Support/SBFReceiverModel.h"

using namespace std::chrono_literals;
using namespace GPSTest;

namespace {
/// The passive family on the runtime, with a link that counts every operation and never delivers data.
struct Receiver : LoggedRuntime
{
    explicit Receiver(GPSTestClock& testClock)
        : LoggedRuntime(Passive::FAMILY, _link(this, testClock))
        , clock(testClock)
    {}

    Receiver(const Receiver&) = delete;
    Receiver& operator=(const Receiver&) = delete;

    void feed(std::string_view text)
    {
        (void) runtime.consume({reinterpret_cast<const uint8_t*>(text.data()), text.size()});
    }

    void consume(std::span<const uint8_t> bytes) { (void) runtime.consume(bytes); }

    void feed(const QByteArray& bytes)
    {
        (void) runtime.consume({reinterpret_cast<const uint8_t*>(bytes.constData()), size_t(bytes.size())});
    }

    std::vector<GPSType> inputs() const
    {
        std::vector<GPSType> result;
        for (const auto& input : reports<GPSInputProtocol>()) {
            result.push_back(input.family);
        }
        return result;
    }

    GPSTestClock& clock;
    int writes = 0;
    int reads = 0;
    unsigned baud = 0;
    GPSReadStatus readStatus = GPSReadStatus::TimedOut;
    GPSBaudStatus baudStatus = GPSBaudStatus::Configured;

private:
    static GPSRuntimeIO _link(Receiver* self, GPSTestClock& clock)
    {
        clock.reset(GPSTestClock::START_US);
        GPSRuntimeIO io = makeGPSRuntimeTestIO(clock);
        io.write = [self](std::span<const uint8_t> bytes, GPSDeadline) {
            ++self->writes;
            return GPSWriteResult{GPSWriteStatus::Completed, int(bytes.size()), int(bytes.size())};
        };
        io.read = [self, &clock](std::span<uint8_t>, GPSDeadline deadline) {
            ++self->reads;
            clock.advanceTo(deadline.untilUs);
            return GPSReadResult{self->readStatus};
        };
        io.setBaudrate = [self](unsigned value) {
            self->baud = value;
            return self->baudStatus;
        };
        return io;
    }
};

void configuration(GPSTestClock& clock)
{
    const GPSProtocolLogCapture log;
    Receiver receiver(clock);
    auto& runtime = receiver.runtime;
    GPSConfig config;
    unsigned baud = 115200;
    QVERIFY(runtime.configure(config, baud));
    QVERIFY(runtime.receiverReady());
    QCOMPARE(receiver.baud, 115200);
    QCOMPARE(runtime.receive(10ms), GPSReceiveUpdates{});
    QCOMPARE(runtime.error(), GPSProtocolError::None);
    receiver.readStatus = GPSReadStatus::Cancelled;
    QCOMPARE(runtime.receive(10ms), GPSReceiveUpdates{});
    QCOMPARE(runtime.error(), GPSProtocolError::Cancelled);
    const auto reads = receiver.reads;
    QCOMPARE(runtime.receive(10ms), GPSReceiveUpdates{});
    QCOMPARE(runtime.error(), GPSProtocolError::Cancelled);
    QCOMPARE(receiver.reads, reads);
    QCOMPARE(receiver.writes, 0);
    receiver.baudStatus = GPSBaudStatus::Error;
    QVERIFY(!runtime.configure(config, baud));
    QVERIFY(!runtime.receiverReady());
    QCOMPARE(runtime.error(), GPSProtocolError::Transport);
    QCOMPARE(runtime.errorDetail(), QStringLiteral("Cannot set the receiver link to 115200 baud"));
    receiver.baudStatus = GPSBaudStatus::Cancelled;
    const auto warnings = log.warnings().size();
    QVERIFY(!runtime.configure(config, baud));
    QCOMPARE(runtime.error(), GPSProtocolError::Cancelled);
    // A requested stop is not a link fault worth a warning.
    QCOMPARE(log.warnings().size(), warnings);
    QCOMPARE(receiver.writes, 0);
}

void navigation(GPSTestClock& clock)
{
    Receiver receiver(clock);
    const auto gga = nmeaSentence("GNGGA,123519,4807.038,N,01131.000,E,4,00,0.9,0.0,M,,M,,");
    const auto gst = nmeaSentence("GNGST,123519,0,0,0,0,0.3,0.4,0.6");
    receiver.feed(gst);
    QVERIFY(receiver.reports<GPSDecodedPosition>().empty());
    for (const char byte : gga) {
        receiver.feed(std::string_view(&byte, 1));
    }
    const auto fixes = receiver.reports<GPSDecodedPosition>();
    QCOMPARE(fixes.size(), 1);
    QCOMPARE(fixes[0].navigation.fixType, GPSPositionReport::FixType::RTKFixed);
    QVERIFY(std::abs(fixes[0].navigation.latitudeDegrees - 48.1173) < 1e-8);
    QCOMPARE(fixes[0].navigation.altitudeMslMeters, 0);
    QVERIFY(std::isnan(fixes[0].navigation.altitudeEllipsoidMeters));
    QVERIFY(std::abs(fixes[0].navigation.horizontalAccuracyMeters - 0.5) < 1e-6);
    QCOMPARE(receiver.reports<GPSDecodedSatelliteUsage>().back().usedCount, 0);
    receiver.events.clear();
    receiver.clock.advanceBy(1000000);
    receiver.feed(nmeaSentence("GNGGA,123520,4807.038,N,01131.000,E,1,,0.9,1.0,M,2.0,M,,"));
    QVERIFY(std::isnan(receiver.reports<GPSDecodedPosition>().back().navigation.horizontalAccuracyMeters));
    QVERIFY(!receiver.reports<GPSDecodedSatelliteUsage>().back().usedCount);
    const auto positionTime = receiver.position.navigation.timestampUs;
    receiver.clock.advanceBy(1000);
    receiver.feed(nmeaSentence("GNGST,123520,0,0,0,0,0.6,0.8,1.0"));
    QCOMPARE(receiver.reports<GPSDecodedPosition>().size(), 2);
    QCOMPARE(receiver.position.navigation.timestampUs, positionTime);
    QVERIFY(std::abs(receiver.position.navigation.horizontalAccuracyMeters - 1.0) < 1e-6);
    receiver.events.clear();
    auto corrupt = gga;
    corrupt[10] ^= 1;
    receiver.feed(corrupt);
    receiver.feed(gga.substr(0, 8) + "\r" + gga.substr(8));
    QVERIFY(receiver.reports<GPSDecodedPosition>().empty());
    receiver.feed(nmeaSentence("GNGGA,123521,,,,,0,00,0.9,,M,,M,,"));
    QCOMPARE(receiver.reports<GPSDecodedPosition>().size(), 1);
    QCOMPARE(receiver.position.navigation.fixType, GPSPositionReport::FixType::NoFix);
    QVERIFY(std::isnan(receiver.position.navigation.latitudeDegrees) &&
            std::isnan(receiver.position.navigation.longitudeDegrees));
    QCOMPARE(receiver.reports<GPSDecodedSatelliteUsage>().back().usedCount, 0);
    receiver.events.clear();
    receiver.feed(std::string(10000, 'A') + "\n" + gga);
    QCOMPARE(receiver.reports<GPSDecodedPosition>().size(), 1);
    QVERIFY(receiver.reports<GPSSurveyReport>().empty());
    QVERIFY(receiver.writes == 0 && receiver.reads == 0 && receiver.baud == 0);

    for (const auto* body :
         {"GNRMC,123522,V,,,,,,,090926,,,N", "GNGLL,,,,,123522,V", "GPGSA,A,1,,,,,,,,,,,,,1.0,0.8,0.6"}) {
        receiver.feed(gga);
        receiver.events.clear();
        receiver.feed(nmeaSentence(body));
        const auto invalid = receiver.reports<GPSDecodedPosition>();
        QVERIFY(invalid.size() == 1 && invalid.front().navigation.fixType == GPSPositionReport::FixType::NoFix);
        QVERIFY(std::isnan(invalid.front().navigation.latitudeDegrees) &&
                std::isnan(invalid.front().navigation.horizontalAccuracyMeters));
    }
}

void corrections(GPSTestClock& clock)
{
    const auto text = nmeaSentence("GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,");
    std::vector<uint8_t> payload{0x3e, 0xd0};
    payload.insert(payload.end(), text.begin(), text.end());
    const auto binary = rtcmPacket(payload);
    for (size_t split = 0; split <= binary.size(); ++split) {
        Receiver receiver(clock);
        receiver.consume(std::span(binary).first(split));
        receiver.consume(std::span(binary).subspan(split));
        QVERIFY(receiver.reports<GPSDecodedPosition>().empty());
        QCOMPARE(receiver.reports<GPSRTCMFrame>().size(), 1);
        const auto correction = receiver.reports<GPSRTCMFrame>().front();
        QCOMPARE(correction.bytes,
                 QByteArray(reinterpret_cast<const char*>(binary.data()), static_cast<qsizetype>(binary.size())));
        QVERIFY(receiver.writes == 0 && receiver.reads == 0);
    }
    Receiver receiver(clock);
    auto corrupt = binary;
    corrupt.back() ^= 1;
    receiver.consume(corrupt);
    QVERIFY(receiver.reports<GPSRTCMFrame>().empty());
    receiver.feed(text);
    QCOMPARE(receiver.reports<GPSDecodedPosition>().size(), 1);

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
    QCOMPARE(receiver.reports<GPSRTCMFrame>().size(), 30);
}

void satellites(GPSTestClock& clock)
{
    Receiver receiver(clock);
    receiver.feed(nmeaSentence("GPGSV,2,1,05,01,10,20,30,02,20,30,40,03,30,40,50,04,40,50,60"));
    QVERIFY(receiver.reports<GPSDecodedSatellites>().empty());
    receiver.feed(nmeaSentence("GPGSV,2,2,05,05,50,60,70"));
    QVERIFY(receiver.reports<GPSDecodedSatellites>().empty());
    receiver.clock.advanceBy(NMEA::SatelliteAssembler::IDLE_TIMEOUT);
    receiver.consume({});
    const auto reports = receiver.reports<GPSDecodedSatellites>();
    QCOMPARE(reports.size(), 2);
    QCOMPARE(reports[0].constellations[0].constellation, GPSConstellation::GPS);
    QCOMPARE(reports[0].constellations[0].inView, 5);
    QVERIFY(receiver.reports<GPSDecodedPosition>().empty());
    QCOMPARE(receiver.writes, 0);
}

void satelliteEpochBoundaries(GPSTestClock& clock)
{
    Receiver receiver(clock);
    const auto send = [&](const char* body) { receiver.feed(nmeaSentence(body)); };
    send("GPGSV,2,1,05,01,10,20,30,02,20,30,40,03,30,40,50,04,40,50,60,1");
    send("GLGSV,1,1,01,65,10,20,30,1");
    send("GPGSV,2,2,05,05,50,60,70,1");
    send("GPGSV,1,1,01,07,10,20,45,7");
    QVERIFY(receiver.reports<GPSDecodedSatellites>().empty());
    send("GNRMC,120001.00,V,,,,,,,090926,,,N");
    auto reports = receiver.reports<GPSDecodedSatellites>();
    QCOMPARE(reports.size(), 3);
    QVERIFY(reports[0].constellations[0].constellation == GPSConstellation::GPS &&
            reports[0].constellations[0].inView == 6);
    QVERIFY(reports[1].constellations[0].constellation == GPSConstellation::GLONASS &&
            reports[1].constellations[0].inView == 1);
    QCOMPARE(reports[0].constellations[0].inViewTimestampUs, 1000000);
    receiver.events.clear();
    receiver.feed(nmeaSentence("GPGSA,A,3,01,,,,,,,,,,,,1.0,0.8,0.6"));
    receiver.clock.advanceBy(NMEA::SatelliteAssembler::IDLE_TIMEOUT);
    receiver.consume({});
    reports = receiver.reports<GPSDecodedSatellites>();
    QCOMPARE(reports.size(), 2);
    QCOMPARE(reports[0].constellations[0].inViewTimestampUs, 0);
    QCOMPARE(reports[0].constellations[0].inUse, 1);
    QCOMPARE(reports[1].constellations[0].inUse, 0);

    receiver.events.clear();
    send("GPGSV,2,1,05,01,10,20,30,02,20,30,40,03,30,40,50,04,40,50,60");
    send("GLGSV,1,1,01,66,10,20,30");
    receiver.clock.advanceBy(NMEA::SatelliteAssembler::IDLE_TIMEOUT);
    receiver.consume({});
    reports = receiver.reports<GPSDecodedSatellites>();
    QVERIFY(reports.size() == 1 && reports[0].constellations[0].constellation == GPSConstellation::GLONASS);
    receiver.events.clear();
    send("GPGSV,2,2,05,05,50,60,70");
    receiver.clock.advanceBy(NMEA::SatelliteAssembler::IDLE_TIMEOUT);
    receiver.consume({});
    QVERIFY(receiver.events.empty());
    send("GPGSV,1,1,00");
    receiver.clock.advanceBy(NMEA::SatelliteAssembler::IDLE_TIMEOUT);
    receiver.consume({});
    reports = receiver.reports<GPSDecodedSatellites>();
    QVERIFY(reports.size() == 2 && reports[0].constellations[0].inView == 0);
}

void satelliteBatchDeadline(GPSTestClock& clock)
{
    Receiver receiver(clock);
    receiver.feed(nmeaSentence("GLGSV,1,1,01,65,10,20,30"));
    for (int page = 1; page <= 10; ++page) {
        receiver.feed(
            nmeaSentence("GPGSV,64," + std::to_string(page) + ",256,01,10,20,30,02,20,30,40,03,30,40,50,04,40,50,60"));
        receiver.clock.advanceBy(100000);
    }
    QVERIFY(receiver.reports<GPSDecodedSatellites>().empty());
    receiver.consume({});
    const auto reports = receiver.reports<GPSDecodedSatellites>();
    QCOMPARE(reports.size(), 1);
    QCOMPARE(reports[0].constellations[0].constellation, GPSConstellation::GLONASS);
    QCOMPARE(reports[0].constellations[0].inViewTimestampUs, 1000000);
}

/// A NAV-PVT 3D fix at 47 N 8 E, ended by NAV-EOE.

void protocolSwitch(GPSTestClock& clock)
{
    Receiver receiver(clock);
    receiver.feed(nmeaSentence("GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,"));
    receiver.consume(ubxNavigationEpoch(ubxFix3D(), 1000));
    // NMEA, recognised first, supplies the positions while it delivers them.
    QCOMPARE(receiver.inputs(), std::vector<GPSType>{GPSType::passive});
    QCOMPARE(receiver.reports<GPSDecodedPosition>().size(), 1);
    QVERIFY(std::isnan(receiver.position.navigation.horizontalAccuracyMeters));
    receiver.events.clear();
    receiver.clock.advanceBy(2999999);
    receiver.consume(ubxNavigationEpoch(ubxFix3D(), 2000));
    QVERIFY(receiver.events.empty());
    // Silent for three seconds, it gives way to UBX, whose positions then win.
    receiver.clock.advanceBy(1);
    receiver.consume(ubxNavigationEpoch(ubxFix3D(), 3000));
    QCOMPARE(receiver.inputs(), std::vector<GPSType>{GPSType::ublox});
    QCOMPARE(receiver.reports<GPSDecodedPosition>().size(), 1);
    QCOMPARE(receiver.position.navigation.horizontalAccuracyMeters, 0.0f);
    receiver.events.clear();
    receiver.feed(nmeaSentence("GPGGA,123523,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,"));
    QVERIFY(receiver.reports<GPSDecodedPosition>().empty());
    QCOMPARE(receiver.writes, 0);
}

constexpr GPSTest::ProtocolScenario SCENARIOS[] = {
    {"configuration", configuration},
    {"navigation", navigation},
    {"corrections", corrections},
    {"satellites", satellites},
    {"satellite-epoch-boundaries", satelliteEpochBoundaries},
    {"satellite-batch-deadline", satelliteBatchDeadline},
    {"protocol-switch", protocolSwitch},
};
}  // namespace

void PassiveProtocolTest::_scenario_data()
{
    addScenarioRows(SCENARIOS);
}

void PassiveProtocolTest::_scenario()
{
    runScenario(SCENARIOS);
}

UT_REGISTER_TEST_LIGHTWEIGHT(PassiveProtocolTest, TestLabel::Unit)
