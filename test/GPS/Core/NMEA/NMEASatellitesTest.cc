#include "NMEASatellitesTest.h"

#include <algorithm>
#include <optional>
#include <string>

#include <QtCore/QByteArray>
#include <QtCore/QList>

#include "NMEASatellites.h"
#include "NMEASentence.h"
#include "Protocols/Support/ProtocolTestPackets.h"

namespace {

using Use = GPSSatellite::Use;
using Satellites = QList<GPSSatellite>;

/// A satellite whose direction is known; use(), unknownDirection() and the like change it.
GPSSatellite satellite(GPSConstellation constellation, int id, int elevation, int azimuth, int signal)
{
    return {.constellation = constellation,
            .id = static_cast<uint16_t>(id),
            .azimuthDegrees = static_cast<uint16_t>(azimuth),
            .elevationDegrees = static_cast<int8_t>(elevation),
            .directionKnown = true,
            .signalDbHz = static_cast<uint8_t>(signal)};
}

GPSSatellite gps(int id, int elevation, int azimuth, int signal, Use use = Use::Unknown)
{
    GPSSatellite result = satellite(GPSConstellation::GPS, id, elevation, azimuth, signal);
    result.use = use;
    return result;
}

GPSSatellite withoutDirection(GPSConstellation constellation, int id, int signal)
{
    return {
        .constellation = constellation, .id = static_cast<uint16_t>(id), .signalDbHz = static_cast<uint8_t>(signal)};
}

QByteArray describe(const Satellites& satellites)
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

/// The systems an epoch of @a bodies completes when the assembler is flushed after them.
NMEA::SatelliteEpoch epochOf(std::initializer_list<QByteArray> bodies)
{
    NMEA::SatelliteAssembler assembler;
    uint64_t receivedAtUs = 1000;
    for (const QByteArray& body : bodies) {
        const std::string text = GPSTest::nmeaSentence(body.toStdString(), "");
        const auto sentence = NMEA::sentence(text);
        if (!sentence || !assembler.ingest(*sentence, receivedAtUs += 1000).accepted) {
            return {};
        }
    }
    return assembler.flush();
}

Satellites satellitesOf(const NMEA::SatelliteEpoch& epoch, GPSConstellation constellation)
{
    const auto system = std::ranges::find(epoch, constellation, &NMEA::SatelliteSystem::constellation);
    return system == epoch.end() ? Satellites() : Satellites(system->satellites.begin(), system->satellites.end());
}

}  // namespace

void NMEASatellitesTest::_gsvSatellites_data()
{
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<Satellites>("expected");
    // Number, elevation, azimuth and C/N0 of each satellite.
    QTest::newRow("tracked") << QByteArray("GPGSV,1,1,03,01,40,083,46,02,17,308,41,12,07,344,39")
                             << Satellites{gps(1, 40, 83, 46), gps(2, 17, 308, 41), gps(12, 7, 344, 39)};
    // A satellite the receiver does not track has no C/N0.
    QTest::newRow("untracked") << QByteArray("GPGSV,1,1,02,04,61,358,,09,21,281,")
                               << Satellites{gps(4, 61, 358, 0), gps(9, 21, 281, 0)};
    // One it tracks without orbit data has no direction.
    QTest::newRow("no-direction") << QByteArray("GPGSV,1,1,01,12,,,39")
                                  << Satellites{withoutDirection(GPSConstellation::GPS, 12, 39)};
    QTest::newRow("one-direction-field") << QByteArray("GPGSV,1,1,01,12,45,,39")
                                         << Satellites{withoutDirection(GPSConstellation::GPS, 12, 39)};
    // Identifiers are the constellation's own numbers, as in the counts.
    QTest::newRow("sbas") << QByteArray("GPGSV,1,1,01,33,08,096,35")
                          << Satellites{satellite(GPSConstellation::SBAS, 120, 8, 96, 35)};
    QTest::newRow("glonass") << QByteArray("GLGSV,1,1,01,65,05,359,30")
                             << Satellites{satellite(GPSConstellation::GLONASS, 1, 5, 359, 30)};
    QTest::newRow("north-as-360") << QByteArray("GPGSV,1,1,01,05,10,360,30") << Satellites{gps(5, 10, 0, 30)};
    QTest::newRow("below-horizon") << QByteArray("GPGSV,1,1,01,05,-03,010,30") << Satellites{gps(5, -3, 10, 30)};
    // A value outside its range is not reported; the satellite still counts.
    QTest::newRow("out-of-range") << QByteArray("GPGSV,1,1,03,05,91,010,30,06,10,361,100,07,x,010,y")
                                  << Satellites{withoutDirection(GPSConstellation::GPS, 5, 30),
                                                withoutDirection(GPSConstellation::GPS, 6, 0),
                                                withoutDirection(GPSConstellation::GPS, 7, 0)};
}

void NMEASatellitesTest::_gsvSatellites()
{
    QFETCH(QByteArray, body);
    QFETCH(Satellites, expected);
    const std::string text = GPSTest::nmeaSentence(body.toStdString(), "");
    const auto sentence = NMEA::sentence(text);
    QVERIFY(sentence);
    const auto page = NMEA::gsv(*sentence);
    QVERIFY(page);
    const Satellites listed(page->satellites.begin(), page->satellites.end());
    QVERIFY2(listed == expected, describe(listed).constData());
}

void NMEASatellitesTest::_epochSatellites()
{
    // GPS on two signals, whose second view has a satellite the first does not, and GLONASS; GSA names the
    // satellites in use of both systems.
    const NMEA::SatelliteEpoch epoch = epochOf({
        "GNGSA,A,3,05,02,,,,,,,,,,,1.0,0.8,0.6,1",
        "GNGSA,A,3,66,,,,,,,,,,,,1.0,0.8,0.6,2",
        "GPGSV,1,1,03,05,40,083,46,02,17,308,41,12,,,,1",
        "GPGSV,1,1,03,02,17,308,44,05,40,083,38,25,07,344,22,6",
        "GLGSV,1,1,02,66,30,200,35,65,50,120,40,1",
    });
    // Each satellite once, by number, with the strongest of its signals and the direction any view gives it.
    const Satellites gpsSatellites = satellitesOf(epoch, GPSConstellation::GPS);
    Satellites expected{gps(2, 17, 308, 44, Use::Used), gps(5, 40, 83, 46, Use::Used),
                        withoutDirection(GPSConstellation::GPS, 12, 0), gps(25, 7, 344, 22, Use::Unused)};
    expected[2].use = Use::Unused;
    QVERIFY2(gpsSatellites == expected, describe(gpsSatellites).constData());
    GPSSatellite used = satellite(GPSConstellation::GLONASS, 2, 30, 200, 35);
    used.use = Use::Used;
    GPSSatellite unused = satellite(GPSConstellation::GLONASS, 1, 50, 120, 40);
    unused.use = Use::Unused;
    const Satellites glonass = satellitesOf(epoch, GPSConstellation::GLONASS);
    QVERIFY2(glonass == Satellites({unused, used}), describe(glonass).constData());
    // The counts are of the satellites listed.
    const auto system = std::ranges::find(epoch, GPSConstellation::GPS, &NMEA::SatelliteSystem::constellation);
    QVERIFY(system != epoch.end());
    QCOMPARE(system->inView, 4);
    QCOMPARE(system->inUse, std::optional<int>(2));
}

void NMEASatellitesTest::_epochWithoutUse()
{
    // A receiver that sends no GSA does not say which satellites it uses.
    NMEA::SatelliteEpoch epoch = epochOf({"GPGSV,1,1,02,05,40,083,46,02,17,308,41"});
    Satellites listed = satellitesOf(epoch, GPSConstellation::GPS);
    QVERIFY2(listed == Satellites({gps(2, 17, 308, 41), gps(5, 40, 83, 46)}), describe(listed).constData());
    // One without a fix uses none.
    epoch = epochOf({QByteArray("GPGSA,A,1") + QByteArray(15, ','), "GPGSV,1,1,02,05,40,083,46,02,17,308,41"});
    listed = satellitesOf(epoch, GPSConstellation::GPS);
    QVERIFY2(listed == Satellites({gps(2, 17, 308, 41, Use::Unused), gps(5, 40, 83, 46, Use::Unused)}),
             describe(listed).constData());
}

UT_REGISTER_TEST_LIGHTWEIGHT(NMEASatellitesTest, TestLabel::Unit)
