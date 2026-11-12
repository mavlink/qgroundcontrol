#include "PassiveProtocolTest.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <string>
#include <utility>
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
/// The protocols a passive input reported its positions to come from, in order.
std::vector<GPSType> inputProtocols(const EventLog& log)
{
    std::vector<GPSType> result;
    for (const auto& input : log.reports<GPSInputProtocol>()) {
        result.push_back(input.family);
    }
    return result;
}

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

    std::vector<GPSType> inputs() const { return inputProtocols(*this); }

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
        io.setBaudRate = [self](unsigned value) {
            self->baud = value;
            return self->baudStatus;
        };
        return io;
    }
};

/// The passive family on a link whose receiver sends its stream at one rate and line noise at every other, in a read
/// every 100 ms. The link counts every write.
struct ListenedReceiver : LoggedRuntime
{
    ListenedReceiver(GPSTestClock& testClock, unsigned receiverRate, QByteArray receiverStream)
        : LoggedRuntime(Passive::FAMILY, _link(this, testClock))
        , clock(testClock)
        , rate(receiverRate)
        , stream(std::move(receiverStream))
    {}

    ListenedReceiver(const ListenedReceiver&) = delete;
    ListenedReceiver& operator=(const ListenedReceiver&) = delete;

    /// Listens for the receiver's rate. @return the rate found, or zero.
    unsigned listen()
    {
        unsigned baud = 0;
        const uint64_t start = clock.nowUs();
        const bool found = runtime.configure({}, baud);
        elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::microseconds(clock.nowUs() - start));
        return found ? baud : 0;
    }

    GPSTestClock& clock;
    unsigned rate;
    QByteArray stream;
    /// What arrives at every other rate: the start of an RTCM3 frame of the greatest length, which a window of these
    /// reads does not fill. A framer left inside it would take what arrives at the next rate.
    QByteArray noise = QByteArray::fromHex("d303ff") + QByteArray(30, 'U');
    /// The rates the link refuses.
    std::vector<unsigned> unsupported;
    /// The rates the link was set to, in order.
    std::vector<unsigned> rates;
    GPSReadStatus failure = GPSReadStatus::TimedOut;
    std::chrono::milliseconds elapsed{0};
    int writes = 0;

private:
    static GPSRuntimeIO _link(ListenedReceiver* self, GPSTestClock& clock)
    {
        constexpr uint64_t READ_INTERVAL_US = 100000;
        clock.reset(GPSTestClock::START_US);
        GPSRuntimeIO io = makeGPSRuntimeTestIO(clock);
        io.write = [self](std::span<const uint8_t> bytes, GPSDeadline) {
            ++self->writes;
            return GPSWriteResult{GPSWriteStatus::Completed, int(bytes.size()), int(bytes.size())};
        };
        io.read = [self, &clock](std::span<uint8_t> buffer, GPSDeadline deadline) {
            if (self->failure != GPSReadStatus::TimedOut) {
                return GPSReadResult{self->failure};
            }
            const QByteArray& bytes = self->rates.back() == self->rate ? self->stream : self->noise;
            if (bytes.isEmpty() || clock.nowUs() + READ_INTERVAL_US > deadline.untilUs) {
                clock.advanceTo(deadline.untilUs);
                return GPSReadResult{GPSReadStatus::TimedOut};
            }
            clock.advanceBy(READ_INTERVAL_US);
            const auto count = std::min(buffer.size(), static_cast<size_t>(bytes.size()));
            std::copy_n(bytes.cbegin(), count, buffer.begin());
            return GPSReadResult{GPSReadStatus::Data, static_cast<int>(count)};
        };
        io.setBaudRate = [self](unsigned value) {
            if (std::ranges::find(self->unsupported, value) != self->unsupported.end()) {
                return GPSBaudStatus::Unsupported;
            }
            self->rates.push_back(value);
            return GPSBaudStatus::Configured;
        };
        return io;
    }
};

const std::vector<unsigned> LISTEN_RATES{115200, 9600, 38400, 4800, 57600, 19200, 230400, 460800, 921600};

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
    // An epoch is published once: accuracy that arrives after it is not another position.
    receiver.clock.advanceBy(1000);
    receiver.feed(nmeaSentence("GNGST,123520,0,0,0,0,0.6,0.8,1.0"));
    QCOMPARE(receiver.reports<GPSDecodedPosition>().size(), 1);
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
    QVERIFY(receiver.reports<GPSBaseStatus>().empty());
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
    // One report for the epoch: GPS and the SBAS satellites a GP view would list.
    const auto reports = receiver.reports<GPSDecodedSatellites>();
    QCOMPARE(reports.size(), 1);
    QCOMPARE(reports[0].count, 2);
    QCOMPARE(reports[0].constellations[0].constellation, GPSConstellation::GPS);
    QCOMPARE(reports[0].constellations[0].inView, 5);
    QVERIFY(reports[0].constellations[1].constellation == GPSConstellation::SBAS &&
            reports[0].constellations[1].inView == 0);
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
    // The systems of the epoch arrive together.
    auto reports = receiver.reports<GPSDecodedSatellites>();
    QCOMPARE(reports.size(), 1);
    QCOMPARE(reports[0].count, 3);
    QVERIFY(reports[0].constellations[0].constellation == GPSConstellation::GPS &&
            reports[0].constellations[0].inView == 6);
    QVERIFY(reports[0].constellations[1].constellation == GPSConstellation::GLONASS &&
            reports[0].constellations[1].inView == 1);
    QCOMPARE(reports[0].constellations[0].inViewTimestampUs, 1000000);
    receiver.events.clear();
    receiver.feed(nmeaSentence("GPGSA,A,3,01,,,,,,,,,,,,1.0,0.8,0.6"));
    receiver.clock.advanceBy(NMEA::SatelliteAssembler::IDLE_TIMEOUT);
    receiver.consume({});
    reports = receiver.reports<GPSDecodedSatellites>();
    QCOMPARE(reports.size(), 1);
    QCOMPARE(reports[0].count, 2);
    QCOMPARE(reports[0].constellations[0].inViewTimestampUs, 0);
    QCOMPARE(reports[0].constellations[0].inUse, 1);
    QCOMPARE(reports[0].constellations[1].inUse, 0);

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
    QVERIFY(reports.size() == 1 && reports[0].count == 2 && reports[0].constellations[0].inView == 0);
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
    QCOMPARE(receiver.inputs(), std::vector<GPSType>{GPSType::Passive});
    QCOMPARE(receiver.reports<GPSDecodedPosition>().size(), 1);
    QVERIFY(std::isnan(receiver.position.navigation.horizontalAccuracyMeters));
    receiver.events.clear();
    receiver.clock.advanceBy(2999999);
    receiver.consume(ubxNavigationEpoch(ubxFix3D(), 2000));
    QVERIFY(receiver.events.empty());
    // Silent for three seconds, it gives way to UBX, whose positions then win.
    receiver.clock.advanceBy(1);
    receiver.consume(ubxNavigationEpoch(ubxFix3D(), 3000));
    QCOMPARE(receiver.inputs(), std::vector<GPSType>{GPSType::UBlox});
    QCOMPARE(receiver.reports<GPSDecodedPosition>().size(), 1);
    QCOMPARE(receiver.position.navigation.horizontalAccuracyMeters, 0.0f);
    receiver.events.clear();
    receiver.feed(nmeaSentence("GPGGA,123523,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,"));
    QVERIFY(receiver.reports<GPSDecodedPosition>().empty());
    QCOMPARE(receiver.writes, 0);
}

/// Without a rate, the receiver's is found by listening at each usual rate for 2.2 s, and nothing is written.
void listensForRate(GPSTestClock& clock)
{
    const QByteArray gga = nmeaBytes("GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,");

    const struct
    {
        unsigned rate;
        QByteArray stream;
        std::vector<GPSType> inputs;
    } receivers[] = {
        {115200, gga, {GPSType::Passive}},
        {9600, gga, {GPSType::Passive}},
        {38400, toByteArray(ubxNavigationEpoch(ubxFix3D(), 1000)), {GPSType::UBlox}},
        {230400, SBFReceiverModel::pvt(12, 1000), {GPSType::Septentrio}},
        // A receiver that only sends corrections has no protocol to name.
        {921600, toByteArray(rtcmPacket(std::array<uint8_t, 2>{0x3e, 0xd0})), {}},
    };

    for (const auto& sent : receivers) {
        const GPSProtocolLogCapture log;
        ListenedReceiver receiver(clock, sent.rate, sent.stream);
        QCOMPARE(receiver.listen(), sent.rate);
        QVERIFY(receiver.runtime.receiverReady());
        const auto tried = std::ranges::find(LISTEN_RATES, sent.rate) - LISTEN_RATES.begin() + 1;
        QCOMPARE(receiver.rates, std::vector<unsigned>(LISTEN_RATES.begin(), LISTEN_RATES.begin() + tried));
        // A window for each rate that only carried noise, and the read that delivered the stream.
        QCOMPARE(receiver.elapsed, std::chrono::milliseconds((tried - 1) * 2200 + 100));
        QCOMPARE(receiver.runtime.identity(), QStringLiteral("Found at %1 baud").arg(sent.rate));
        QCOMPARE(inputProtocols(receiver), sent.inputs);
        QCOMPARE(receiver.writes, 0);
        QVERIFY2(log.warnings().isEmpty(), qPrintable(log.warnings().join(u'\n')));
    }
}

/// Listening ends without a rate when nothing a passive input reads arrives at any, when the link runs at none, and
/// when the link fails.
void listensInVain(GPSTestClock& clock)
{
    const GPSProtocolLogCapture log;
    {
        // A device that is no GNSS receiver, or one that sends at a rate that is not listened at.
        ListenedReceiver receiver(clock, 1200,
                                  nmeaBytes("GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,"));
        QCOMPARE(receiver.listen(), 0U);
        QVERIFY(!receiver.runtime.receiverReady());
        QCOMPARE(receiver.runtime.error(), GPSProtocolError::Protocol);
        QCOMPARE(receiver.runtime.errorDetail(),
                 QStringLiteral("No NMEA, UBX, SBF or RTCM output arrived at 115200, 9600, 38400, 4800, 57600, 19200, "
                                "230400, 460800, 921600 baud. Select the receiver's baud rate if it reports less often "
                                "than every two seconds"));
        QCOMPARE(receiver.rates, LISTEN_RATES);
        QCOMPARE(receiver.elapsed, std::chrono::milliseconds(9 * 2200));
        QVERIFY(receiver.runtime.identity().isEmpty());
        QVERIFY(receiver.events.empty());
        QCOMPARE(receiver.writes, 0);
    }
    {
        // A silent link takes as long, and a rate the link refuses is left out.
        ListenedReceiver receiver(clock, 0, {});
        receiver.noise.clear();
        receiver.unsupported = {115200, 4800, 921600};
        QCOMPARE(receiver.listen(), 0U);
        QCOMPARE(receiver.rates, (std::vector<unsigned>{9600, 38400, 57600, 19200, 230400, 460800}));
        QVERIFY2(receiver.runtime.errorDetail().contains(
                     QStringLiteral("arrived at 9600, 38400, 57600, 19200, 230400, 460800 baud.")),
                 qPrintable(receiver.runtime.errorDetail()));
        QCOMPARE(receiver.elapsed, std::chrono::milliseconds(6 * 2200));
    }
    {
        ListenedReceiver receiver(clock, 0, {});
        receiver.unsupported = LISTEN_RATES;
        QCOMPARE(receiver.listen(), 0U);
        QCOMPARE(receiver.runtime.errorDetail(),
                 QStringLiteral("The link cannot run at any baud rate the receiver is listened for at"));
        QCOMPARE(receiver.elapsed, std::chrono::milliseconds::zero());
    }
    {
        ListenedReceiver receiver(clock, 0, {});
        receiver.failure = GPSReadStatus::Cancelled;
        QCOMPARE(receiver.listen(), 0U);
        QCOMPARE(receiver.runtime.error(), GPSProtocolError::Cancelled);
        QCOMPARE(receiver.rates, std::vector<unsigned>{115200});
        QCOMPARE(receiver.writes, 0);
    }
}

constexpr GPSTest::ProtocolScenario SCENARIOS[] = {
    {"configuration", configuration},
    {"listens-for-rate", listensForRate},
    {"listens-in-vain", listensInVain},
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
