#include "GPSSatelliteReportTest.h"

#include <chrono>
#include <optional>
#include <span>
#include <vector>

#include <QtCore/QByteArray>
#include <QtCore/QList>

#include "GPSDecodedData.h"
#include "GPSDriver.h"
#include "GPSProtocolRuntime.h"
#include "GPSReceiverFamilies.h"
#include "GPSReceiverReports.h"
#include "NMEASatellites.h"
#include "Protocols/Support/ProtocolTestPackets.h"
#include "Protocols/Support/ReceiverBench.h"
#include "Protocols/Support/ScriptedReceiver.h"
#include "UBX/UBXFrame.h"

using namespace std::chrono_literals;
using namespace GPSTest;

namespace {

using ReportedConstellation = GPSSatelliteReport::Constellation;
using Use = GPSSatellite::Use;
using Satellites = QList<GPSSatellite>;

/// A satellite the receiver uses or not, at a known direction.
GPSSatellite satellite(GPSConstellation constellation, int id, int elevation, int azimuth, int signal, bool used)
{
    return {.constellation = constellation,
            .id = static_cast<uint16_t>(id),
            .azimuthDegrees = static_cast<uint16_t>(azimuth),
            .elevationDegrees = static_cast<int8_t>(elevation),
            .directionKnown = true,
            .signalDbHz = static_cast<uint8_t>(signal),
            .use = used ? Use::Used : Use::Unused};
}

QByteArray describe(std::span<const GPSSatellite> satellites)
{
    QByteArray text;
    for (const GPSSatellite& listed : satellites) {
        text += QByteArray::number(static_cast<int>(listed.constellation)) + '-' + QByteArray::number(listed.id) +
                (listed.directionKnown ? ' ' + QByteArray::number(listed.elevationDegrees) + '/' +
                                             QByteArray::number(listed.azimuthDegrees)
                                       : QByteArray(" -/-")) +
                ' ' + QByteArray::number(listed.signalDbHz) + " use " +
                QByteArray::number(static_cast<int>(listed.use)) + "; ";
    }
    return text;
}

Satellites satellitesOf(const GPSSatelliteReport& report)
{
    const auto listed = report.satellitesInView();
    return Satellites(listed.begin(), listed.end());
}

/// The NAV-SAT satellites of two captures, read from the bytes at the offsets of the u-blox interface description
/// (gnssId, svId, cno, elev, azim and svUsed in bit 3 of the flags), in the order of a report: by constellation and
/// then by number. No value comes from a QGC decoder.
struct CapturedSatellite
{
    const char* file;
    GPSConstellation constellation;
    int id;
    int elevation;
    int azimuth;
    bool directionKnown;
    int signal;
    bool used;
};

constexpr const char* ROVER = "gpsd-neo-m8p-rover.cap";
constexpr const char* SESSION = "pyubx2-zed-f9p-session.cap";

constexpr CapturedSatellite CAPTURED[] = {
    {ROVER, GPSConstellation::GPS, 1, 56, 148, true, 33, true},
    {ROVER, GPSConstellation::GPS, 3, 83, 352, true, 33, true},
    {ROVER, GPSConstellation::GPS, 4, 49, 198, true, 31, true},
    {ROVER, GPSConstellation::GPS, 6, 9, 311, true, 0, false},
    {ROVER, GPSConstellation::GPS, 9, 16, 215, true, 23, false},
    {ROVER, GPSConstellation::GPS, 12, 2, 355, true, 0, false},
    {ROVER, GPSConstellation::GPS, 17, 40, 270, true, 27, true},
    {ROVER, GPSConstellation::GPS, 19, 33, 300, true, 29, true},
    {ROVER, GPSConstellation::GPS, 21, 36, 147, true, 32, true},
    {ROVER, GPSConstellation::GPS, 22, 61, 83, true, 35, true},
    {ROVER, GPSConstellation::GPS, 31, 29, 79, true, 19, false},
    {ROVER, GPSConstellation::GPS, 32, 1, 45, true, 0, false},
    {ROVER, GPSConstellation::GLONASS, 5, 40, 47, true, 26, true},
    {ROVER, GPSConstellation::GLONASS, 6, 70, 270, true, 0, false},
    {ROVER, GPSConstellation::GLONASS, 7, 21, 242, true, 0, false},
    {ROVER, GPSConstellation::GLONASS, 13, 0, 341, true, 0, false},
    {ROVER, GPSConstellation::GLONASS, 14, 16, 29, true, 0, false},
    {ROVER, GPSConstellation::GLONASS, 15, 12, 79, true, 0, false},
    {ROVER, GPSConstellation::GLONASS, 20, 25, 168, true, 0, false},
    {ROVER, GPSConstellation::GLONASS, 21, 71, 244, true, 18, true},
    {ROVER, GPSConstellation::GLONASS, 22, 36, 323, true, 0, false},
    {SESSION, GPSConstellation::GPS, 3, 46, 84, true, 24, true},
    {SESSION, GPSConstellation::GPS, 4, 70, 112, true, 27, true},
    {SESSION, GPSConstellation::GPS, 6, 52, 297, true, 28, true},
    {SESSION, GPSConstellation::GPS, 9, 55, 195, true, 31, true},
    {SESSION, GPSConstellation::GPS, 17, 27, 220, true, 26, true},
    {SESSION, GPSConstellation::GPS, 19, 39, 244, true, 46, true},
    // Its elevation is -91: the receiver does not know where it is.
    {SESSION, GPSConstellation::GPS, 32, 0, 0, false, 0, false},
    {SESSION, GPSConstellation::GLONASS, 2, 8, 54, true, 23, false},
    {SESSION, GPSConstellation::GLONASS, 3, 59, 45, true, 12, false},
    {SESSION, GPSConstellation::GLONASS, 4, 63, 250, true, 52, true},
    {SESSION, GPSConstellation::GLONASS, 5, 12, 239, true, 25, false},
    {SESSION, GPSConstellation::GLONASS, 19, 71, 232, true, 20, true},
    {SESSION, GPSConstellation::SBAS, 123, 22, 140, true, 0, false},
    {SESSION, GPSConstellation::SBAS, 127, 10, 117, true, 0, false},
    {SESSION, GPSConstellation::SBAS, 136, 29, 171, true, 0, false},
};

Satellites capturedIn(const QByteArray& file)
{
    Satellites result;
    for (const CapturedSatellite& row : CAPTURED) {
        if (file == row.file) {
            GPSSatellite listed =
                satellite(row.constellation, row.id, row.elevation, row.azimuth, row.signal, row.used);
            listed.directionKnown = row.directionKnown;
            result.append(listed);
        }
    }
    return result;
}

/// The satellite events a u-blox receiver's @a bytes decode to, on a decoder armed for a receiver configured
/// elsewhere.
std::vector<GPSDecodedSatellites> ubxSatelliteEvents(GPSTestClock& clock, std::span<const uint8_t> bytes)
{
    GPSProtocolRuntime runtime(UBX::FAMILY, makeDecoderOnlyIO(clock));
    runtime.armNavigationDecode({.corrections = true});
    std::vector<GPSDecodedSatellites> result;
    for (const auto& event : runtime.decode(bytes).events) {
        if (const auto* satellites = std::get_if<GPSDecodedSatellites>(&event)) {
            result.push_back(*satellites);
        }
    }
    return result;
}

/// Stamps every constellation of @a report as seen and counted at @a receivedAtUs.
void stamp(GPSDecodedSatellites& report, uint64_t receivedAtUs)
{
    for (uint8_t index = 0; index < report.count; ++index) {
        report.constellations[index].inViewTimestampUs = receivedAtUs;
        report.constellations[index].inUseTimestampUs = receivedAtUs;
    }
}

GPSDecodedSatellites decodedSatellites(std::initializer_list<GPSSatellite> satellites, uint64_t receivedAtUs,
                                       bool fullSnapshot)
{
    GPSDecodedSatellites report;
    report.fullSnapshot = fullSnapshot;
    for (const GPSSatellite& listed : satellites) {
        report.addSatellite(listed);
    }
    stamp(report, receivedAtUs);
    return report;
}

/// Satellite @a id of @a constellation, with a direction and a signal of its own.
GPSSatellite numbered(GPSConstellation constellation, int id, bool used)
{
    return satellite(constellation, id, id % 90, id * 7 % 360, 20 + id % 30, used);
}

GPSSatellite gps(int id, bool used = true)
{
    return numbered(GPSConstellation::GPS, id, used);
}

GPSSatellite glonass(int id, bool used = true)
{
    return numbered(GPSConstellation::GLONASS, id, used);
}

QList<ReportedConstellation> constellationsOf(const GPSSatelliteReport& report)
{
    const auto listed = report.constellationsInView();
    return QList<ReportedConstellation>(listed.begin(), listed.end());
}

/// A passive driver on a scripted link, and the reports it published that cover satellites in view.
struct PassiveSatellites
{
    PassiveSatellites()
        : driver(GPSType::Passive, link, {.baudRate = 115200}, sinks(), clock.source())
    {}

    GPSDriverSinks sinks()
    {
        GPSDriverSinks result;
        result.onSatelliteInfo = [this](const GPSSatelliteReport& report) {
            if (report.inView) {
                views.push_back(report);
            }
        };
        return result;
    }

    /// What the receiver outputs next, received to its last byte: a receive cycle ends at its first report.
    GPSReceiveResult receive(const QByteArray& bytes)
    {
        link.clearReplies();
        link.queueReply(bytes);
        GPSReceiveResult result = driver.receiveOutcome(0ms);
        while (result.error == GPSProtocolError::None && link.hasQueuedReadData()) {
            result = driver.receiveOutcome(0ms);
        }
        return result;
    }

    GPSTestClock clock{GPSTestClock::START_US};
    ModelReceiver<ScriptedReceiver::Model> link{clock};
    std::vector<GPSSatelliteReport> views;
    GPSDriver driver;
};

}  // namespace

void GPSSatelliteReportTest::_nmeaEpochIsOneReport()
{
    PassiveSatellites receiver;
    QVERIFY(receiver.driver.configure());
    // One epoch of three constellations, each with its satellites in use and in view, which the next position ends.
    QByteArray epoch = nmeaBytes("GPGGA,123519,4807.038,N,01131.000,E,1,05,0.9,545.4,M,46.9,M,,");
    for (const char* sentence :
         {"GNGSA,A,3,01,02,,,,,,,,,,,1.0,0.8,0.6,1", "GNGSA,A,3,65,66,,,,,,,,,,,1.0,0.8,0.6,2",
          "GNGSA,A,3,03,,,,,,,,,,,,1.0,0.8,0.6,3", "GPGSV,1,1,03,01,40,083,46,02,17,308,41,12,07,344,39",
          "GLGSV,1,1,02,65,50,120,40,66,30,200,35", "GAGSV,1,1,02,03,60,010,45,08,20,300,30"}) {
        epoch += nmeaBytes(sentence);
    }
    epoch += nmeaBytes("GPGGA,123520,4807.038,N,01131.000,E,1,05,0.9,545.4,M,46.9,M,,");
    QCOMPARE(receiver.receive(epoch).error, GPSProtocolError::None);
    // The consumer gets the epoch whole, not once for each constellation.
    QCOMPARE(receiver.views.size(), size_t(1));
    QCOMPARE(constellationsOf(receiver.views.front()),
             (QList<ReportedConstellation>{
                 {GPSConstellation::GPS, 3, 2}, {GPSConstellation::GLONASS, 2, 2}, {GPSConstellation::Galileo, 2, 1}}));
}

void GPSSatelliteReportTest::_ubxCaptureSatellites_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QList<ReportedConstellation>>("constellations");
    using Constellation = GPSConstellation;
    QTest::newRow("neo-m8p-rover") << QString::fromLatin1(ROVER)
                                   << QList<ReportedConstellation>{{Constellation::GPS, 12, 7},
                                                                   {Constellation::GLONASS, 9, 2}};
    QTest::newRow("zed-f9p-session") << QString::fromLatin1(SESSION)
                                     << QList<ReportedConstellation>{{Constellation::GPS, 7, 6},
                                                                     {Constellation::GLONASS, 5, 2},
                                                                     {Constellation::SBAS, 3, 0}};
    QTest::newRow("zed-f9p-rtk") << QStringLiteral("gpsd-zed-f9p-rtk.cap")
                                 << QList<ReportedConstellation>{{Constellation::GPS, 13, 6},
                                                                 {Constellation::GLONASS, 9, 5},
                                                                 {Constellation::Galileo, 10, 4},
                                                                 {Constellation::BeiDou, 18, 6},
                                                                 {Constellation::SBAS, 4, 0}};
    QTest::newRow("zed-f9t-base") << QStringLiteral("gpsd-zed-f9t-base.cap")
                                  << QList<ReportedConstellation>{{Constellation::GPS, 11, 9},
                                                                  {Constellation::GLONASS, 10, 9},
                                                                  {Constellation::Galileo, 10, 8},
                                                                  {Constellation::BeiDou, 9, 4},
                                                                  {Constellation::QZSS, 1, 1}};
    QTest::newRow("navigation") << QStringLiteral("navigation.ubx")
                                << QList<ReportedConstellation>{{Constellation::GPS, 13, 8},
                                                                {Constellation::GLONASS, 9, 6},
                                                                {Constellation::Galileo, 10, 7},
                                                                {Constellation::BeiDou, 9, 5},
                                                                {Constellation::SBAS, 2, 0}};
}

void GPSSatelliteReportTest::_ubxCaptureSatellites()
{
    QFETCH(QString, file);
    QFETCH(QList<ReportedConstellation>, constellations);
    const auto bytes = fixtureBytes(file.toLatin1().constData());
    QVERIFY(bytes);
    GPSTestClock clock(GPSTestClock::START_US);
    const auto events = ubxSatelliteEvents(clock, *bytes);
    // Each capture has one NAV-SAT.
    QCOMPARE(events.size(), size_t(1));
    GPSDecodedData::SatelliteCounts counts;
    const GPSSatelliteReport report = counts.update(events.front(), clock.nowUs());
    QCOMPARE(constellationsOf(report), constellations);

    // The report lists the satellites its counts sum: those of each constellation by number, and the constellations
    // in the order of the counts.
    const Satellites listed = satellitesOf(report);
    qsizetype next = 0;
    for (const ReportedConstellation& constellation : constellations) {
        int used = 0;
        for (int index = 0; index < constellation.inView; ++index, ++next) {
            QVERIFY(next < listed.size());
            const GPSSatellite& satellite = listed.at(next);
            QCOMPARE(satellite.constellation, constellation.constellation);
            QVERIFY(index == 0 || satellite.id > listed.at(next - 1).id);
            QVERIFY(satellite.use != Use::Unknown);
            used += satellite.use == Use::Used;
        }
        QCOMPARE(std::optional<int>(used), constellation.used);
    }
    QCOMPARE(listed.size(), next);
    // Every field of the two captures read by hand.
    if (const Satellites captured = capturedIn(file.toLatin1()); !captured.isEmpty()) {
        QVERIFY2(listed == captured, describe(report.satellitesInView()).constData());
    }
}

void GPSSatelliteReportTest::_ubxLegacyNumbering()
{
    // NAV-SVINFO has no constellation field: its svid numbers the satellites of all constellations in one range.
    struct Channel
    {
        uint8_t svid;
        GPSConstellation constellation;
        int id;
    };

    constexpr Channel CHANNELS[] = {
        {1, GPSConstellation::GPS, 1},
        {32, GPSConstellation::GPS, 32},
        {33, GPSConstellation::BeiDou, 6},
        {64, GPSConstellation::BeiDou, 37},
        {65, GPSConstellation::GLONASS, 1},
        {96, GPSConstellation::GLONASS, 32},
        {120, GPSConstellation::SBAS, 120},
        {158, GPSConstellation::SBAS, 158},
        {159, GPSConstellation::BeiDou, 1},
        {163, GPSConstellation::BeiDou, 5},
        {193, GPSConstellation::QZSS, 1},
        {202, GPSConstellation::QZSS, 10},
        {211, GPSConstellation::Galileo, 1},
        {246, GPSConstellation::Galileo, 36},
        // A GLONASS satellite whose slot the receiver does not know yet.
        {255, GPSConstellation::GLONASS, 255},
        // Outside every range, as IMES is: counted, in no constellation.
        {100, GPSConstellation::Unknown, 100},
        {173, GPSConstellation::Unknown, 173},
    };
    constexpr qsizetype HEADER_BYTES = 8;
    constexpr qsizetype CHANNEL_BYTES = 12;
    QByteArray payload(HEADER_BYTES + CHANNEL_BYTES * qsizetype(std::size(CHANNELS)), '\0');
    payload[4] = static_cast<char>(std::size(CHANNELS));
    Satellites expected;
    for (size_t index = 0; index < std::size(CHANNELS); ++index) {
        // Every other channel is used; signal, elevation and azimuth count up, the azimuth past one byte.
        const bool used = index % 2 == 0;
        const int signal = 20 + int(index);
        const int elevation = 5 * int(index);
        const int azimuth = 21 * int(index);
        char* const channel = payload.data() + HEADER_BYTES + CHANNEL_BYTES * qsizetype(index);
        channel[0] = static_cast<char>(index);
        channel[1] = static_cast<char>(CHANNELS[index].svid);
        channel[2] = used ? 0x01 : 0x00;
        channel[4] = static_cast<char>(signal);
        channel[5] = static_cast<char>(elevation);
        channel[6] = static_cast<char>(azimuth & 0xff);
        channel[7] = static_cast<char>(azimuth >> 8);
        expected.append(satellite(CHANNELS[index].constellation, CHANNELS[index].id, elevation, azimuth, signal, used));
    }
    const QByteArray frame = ubxBytes(UBX::Msg::NAV_SVINFO.value(), payload);
    GPSTestClock clock(GPSTestClock::START_US);
    const auto events = ubxSatelliteEvents(
        clock, {reinterpret_cast<const uint8_t*>(frame.constData()), static_cast<size_t>(frame.size())});
    QCOMPARE(events.size(), size_t(1));
    const auto decoded = events.front().satellitesInView();
    QVERIFY2(Satellites(decoded.begin(), decoded.end()) == expected, describe(decoded).constData());
    // Each constellation has its own count, which the receiver status lists.
    GPSDecodedData::SatelliteCounts counts;
    QCOMPARE(constellationsOf(counts.update(events.front(), clock.nowUs())),
             (QList<ReportedConstellation>{{GPSConstellation::Unknown, 2, 1},
                                           {GPSConstellation::GPS, 2, 1},
                                           {GPSConstellation::GLONASS, 3, 2},
                                           {GPSConstellation::Galileo, 2, 1},
                                           {GPSConstellation::BeiDou, 4, 2},
                                           {GPSConstellation::QZSS, 2, 1},
                                           {GPSConstellation::SBAS, 2, 1}}));
}

void GPSSatelliteReportTest::_nmeaCaptureSatellites()
{
    // An Ashtech AC12: GSA names three satellites in use of the ten its GSV pages describe, two of them SBAS.
    const auto bytes = fixtureBytes("gpsd-ac12-pashr.cap");
    QVERIFY(bytes);
    PassiveSatellites receiver;
    QVERIFY(receiver.driver.configure());
    const QByteArray capture(reinterpret_cast<const char*>(bytes->data()), static_cast<qsizetype>(bytes->size()));
    // The capture holds two cycles, which the receiver sent a second apart; each begins with ZDA.
    const qsizetype secondCycle = capture.indexOf("$GPZDA", 1);
    QVERIFY(secondCycle > 0);
    for (const QByteArray& cycle : {capture.first(secondCycle), capture.sliced(secondCycle)}) {
        QCOMPARE(receiver.receive(cycle).error, GPSProtocolError::None);
        receiver.clock.advanceBy(NMEA::SatelliteAssembler::BATCH_TIMEOUT);
        QCOMPARE(receiver.receive({}).error, GPSProtocolError::None);
    }
    // Both cycles describe the same sky, each in one report.
    QCOMPARE(receiver.views.size(), size_t(2));
    const auto unused = [](GPSConstellation constellation, int id, int elevation, int azimuth) {
        return satellite(constellation, id, elevation, azimuth, 0, false);
    };
    const Satellites expected{satellite(GPSConstellation::GPS, 2, 55, 274, 50, true),
                              unused(GPSConstellation::GPS, 4, 61, 358),
                              unused(GPSConstellation::GPS, 9, 21, 281),
                              satellite(GPSConstellation::GPS, 10, 17, 193, 50, true),
                              unused(GPSConstellation::GPS, 12, 10, 322),
                              unused(GPSConstellation::GPS, 17, 38, 54),
                              unused(GPSConstellation::GPS, 20, 3, 38),
                              satellite(GPSConstellation::GPS, 28, 38, 139, 46, true),
                              unused(GPSConstellation::SBAS, 120, 8, 96),
                              unused(GPSConstellation::SBAS, 122, 45, 118)};
    for (const GPSSatelliteReport& report : receiver.views) {
        QVERIFY2(satellitesOf(report) == expected, describe(report.satellitesInView()).constData());
        QCOMPARE(constellationsOf(report),
                 (QList<ReportedConstellation>{{GPSConstellation::GPS, 8, 3}, {GPSConstellation::SBAS, 2, 0}}));
    }
}

void GPSSatelliteReportTest::_reportKeepsEachConstellation()
{
    using namespace std::chrono;
    constexpr uint64_t START_US = 10'000'000;
    constexpr uint64_t SECOND_US = 1'000'000;
    const uint64_t lifetimeUs =
        static_cast<uint64_t>(duration_cast<microseconds>(GPSDecodedData::SatelliteCounts::FRESHNESS_TIMEOUT).count());
    GPSDecodedData::SatelliteCounts counts;

    // The satellites of a snapshot, by constellation and then by number, in whatever order the receiver lists them.
    GPSSatelliteReport report =
        counts.update(decodedSatellites({glonass(7), gps(5, false), gps(2)}, START_US, true), START_US);
    QCOMPARE(satellitesOf(report), Satellites({gps(2), gps(5, false), glonass(7)}));

    // What reports one constellation, as NMEA does, replaces the satellites of that one.
    report =
        counts.update(decodedSatellites({glonass(9), glonass(3)}, START_US + SECOND_US, false), START_US + SECOND_US);
    QCOMPARE(satellitesOf(report), Satellites({gps(2), gps(5, false), glonass(3), glonass(9)}));

    // A used count without a view, as an epoch with GSA and no GSV has, keeps the satellites of the view.
    GPSDecodedSatellites usage;
    usage.fullSnapshot = false;
    auto* system = usage.ensureConstellation(GPSConstellation::GPS);
    QVERIFY(system);
    system->inUseTimestampUs = START_US + 2 * SECOND_US;
    system->inUse = 2;
    report = counts.update(usage, START_US + 2 * SECOND_US);
    QCOMPARE(satellitesOf(report), Satellites({gps(2), gps(5, false), glonass(3), glonass(9)}));
    QCOMPARE(constellationsOf(report).first(), (ReportedConstellation{GPSConstellation::GPS, 2, 2}));

    // A view older than the one held is not taken, and neither are its satellites.
    report = counts.update(decodedSatellites({gps(30)}, START_US - 1, false), START_US + 2 * SECOND_US);
    QCOMPARE(satellitesOf(report), Satellites({gps(2), gps(5, false), glonass(3), glonass(9)}));

    // The satellites of a constellation go with its view when that runs out.
    const auto expired = counts.expire(START_US + lifetimeUs);
    QVERIFY(expired);
    QCOMPARE(satellitesOf(*expired), Satellites({glonass(3), glonass(9)}));

    // A snapshot retires the constellations it leaves out, and a constellation without satellites lists none.
    report = counts.update(decodedSatellites({gps(8)}, START_US + lifetimeUs, true), START_US + lifetimeUs);
    QCOMPARE(satellitesOf(report), Satellites({gps(8)}));
    GPSDecodedSatellites none;
    QVERIFY(none.ensureConstellation(GPSConstellation::GPS));
    stamp(none, START_US + lifetimeUs + SECOND_US);
    report = counts.update(none, START_US + lifetimeUs + SECOND_US);
    QVERIFY(report.satellitesInView().empty());
    QCOMPARE(report.inView, std::optional<int>(0));
}

void GPSSatelliteReportTest::_reportIsBounded()
{
    constexpr uint64_t NOW_US = 10'000'000;
    constexpr int LIMIT = GPSSatelliteReport::MAX_SATELLITES;
    // One message of more satellites than a report lists: all are counted, and the first are listed.
    GPSDecodedSatellites crowded;
    for (int id = 1; id <= LIMIT + 10; ++id) {
        crowded.addSatellite(gps(id));
    }
    stamp(crowded, NOW_US);
    QCOMPARE(crowded.satellitesInView().size(), size_t(LIMIT));
    QCOMPARE(crowded.constellations[0].inView, LIMIT + 10);

    // Two constellations that fill a report between them: the report lists what fits, in its order, and counts all.
    GPSDecodedData::SatelliteCounts counts;
    GPSDecodedSatellites first;
    GPSDecodedSatellites second;
    second.fullSnapshot = false;
    for (int id = 1; id <= 100; ++id) {
        first.addSatellite(glonass(id));
        second.addSatellite(gps(id));
    }
    stamp(first, NOW_US);
    stamp(second, NOW_US);
    (void) counts.update(first, NOW_US);
    const GPSSatelliteReport report = counts.update(second, NOW_US);
    QCOMPARE(report.inView, std::optional<int>(200));
    const Satellites listed = satellitesOf(report);
    QCOMPARE(listed.size(), LIMIT);
    QCOMPARE(listed.at(99), gps(100));
    QCOMPARE(listed.at(100), glonass(1));
    QCOMPARE(listed.last(), glonass(LIMIT - 100));
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSSatelliteReportTest, TestLabel::Unit)
