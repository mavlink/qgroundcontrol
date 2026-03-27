#include "NTRIPGgaProviderTest.h"
#include "NMEAUtils.h"
#include "NTRIPGgaProvider.h"

#include <QtCore/QStringList>
#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QTest>

// Tests delegate checksum validation to the canonical NMEAUtils implementation
// — no local XOR loop, so there is one source of truth.
static inline bool validateChecksum(const QByteArray& sentence)
{
    return NMEAUtils::verifyChecksum(sentence);
}

static int countFields(const QByteArray& gga)
{
    // Count comma-separated fields between '$' and '*'.
    int star = gga.lastIndexOf('*');
    if (star < 0) {
        star = gga.size();
    }
    const QByteArray body = gga.mid(1, star - 1); // strip $ and *XX
    return body.count(',') + 1;
}

// ---------------------------------------------------------------------------
// Format / structure
// ---------------------------------------------------------------------------

void NTRIPGgaProviderTest::testMakeGGA_basicCoordinate()
{
    const QGeoCoordinate coord(47.3977, 8.5456, 450.0);
    const QByteArray gga = NMEAUtils::makeGGA(coord, 450.0);

    QVERIFY(gga.startsWith("$GPGGA,"));
    QVERIFY(gga.contains(",N,"));
    QVERIFY(gga.contains(",E,"));
    QVERIFY(gga.contains("450.0"));
    QVERIFY(validateChecksum(gga));
}

void NTRIPGgaProviderTest::testMakeGGA_checksumFormat()
{
    const QGeoCoordinate coord(35.6762, 139.6503, 40.0);
    const QByteArray gga = NMEAUtils::makeGGA(coord, 40.0);

    const int starIdx = gga.indexOf('*');
    QVERIFY(starIdx > 0);
    QCOMPARE(gga.size(), starIdx + 3);
    QVERIFY(validateChecksum(gga));
}

void NTRIPGgaProviderTest::testMakeGGA_fieldCount()
{
    // GGA body (between $ and *) has 15 comma-separated fields:
    // GPGGA, hhmmss, lat, N/S, lon, E/W, qual, nsat, hdop, alt, M, geoid, M, age, refid
    const QGeoCoordinate coord(40.0, -74.0);
    const QByteArray gga = NMEAUtils::makeGGA(coord, 10.0);

    QCOMPARE(countFields(gga), 15);
}

void NTRIPGgaProviderTest::testMakeGGA_timeFormat()
{
    const QGeoCoordinate coord(40.0, -74.0);
    const QByteArray gga = NMEAUtils::makeGGA(coord, 10.0);

    const QString ggaStr = QString::fromUtf8(gga);
    const QStringList fields = ggaStr.mid(1, ggaStr.indexOf('*') - 1).split(',');
    QVERIFY(fields.size() >= 2);

    const QString timeField = fields[1];
    QVERIFY2(timeField.size() >= 6, qPrintable(QString("Time field too short: %1").arg(timeField)));
    for (int i = 0; i < 6; ++i) {
        QVERIFY2(timeField[i].isDigit(),
                 qPrintable(QString("Non-digit at pos %1: %2").arg(i).arg(timeField)));
    }
    const int hh = timeField.left(2).toInt();
    const int mm = timeField.mid(2, 2).toInt();
    const int ss = timeField.mid(4, 2).toInt();
    QVERIFY2(hh >= 0 && hh <= 23, qPrintable(QString("Invalid hour: %1").arg(hh)));
    QVERIFY2(mm >= 0 && mm <= 59, qPrintable(QString("Invalid minute: %1").arg(mm)));
    QVERIFY2(ss >= 0 && ss <= 59, qPrintable(QString("Invalid second: %1").arg(ss)));
}

// ---------------------------------------------------------------------------
// Hemisphere encoding
// ---------------------------------------------------------------------------

void NTRIPGgaProviderTest::testMakeGGA_negativeCoordinates()
{
    const QGeoCoordinate coord(-33.8688, -151.2093, 10.0);
    const QByteArray gga = NMEAUtils::makeGGA(coord, 10.0);

    QVERIFY(gga.contains(",S,"));
    QVERIFY(gga.contains(",W,"));
    QVERIFY(validateChecksum(gga));
}

void NTRIPGgaProviderTest::testMakeGGA_equator()
{
    const QGeoCoordinate coord(0.0, 0.0, 0.0);
    const QByteArray gga = NMEAUtils::makeGGA(coord, 0.0);

    // Exactly 0.0 degrees encodes as N / E (>= 0.0 convention).
    QVERIFY(gga.contains(",0000.0000,N,00000.0000,E,"));
    QVERIFY(validateChecksum(gga));
}

void NTRIPGgaProviderTest::testMakeGGA_dateLine()
{
    const QGeoCoordinate coordEast(17.7134, 178.065);
    const QByteArray ggaEast = NMEAUtils::makeGGA(coordEast, 5.0);
    QVERIFY(ggaEast.contains(",E,"));
    QVERIFY(validateChecksum(ggaEast));

    const QGeoCoordinate coordWest(17.7134, -179.5);
    const QByteArray ggaWest = NMEAUtils::makeGGA(coordWest, 5.0);
    QVERIFY(ggaWest.contains(",W,"));
    QVERIFY(validateChecksum(ggaWest));
}

// ---------------------------------------------------------------------------
// Altitude edge cases
// ---------------------------------------------------------------------------

void NTRIPGgaProviderTest::testMakeGGA_zeroAltitude()
{
    const QGeoCoordinate coord(51.5074, -0.1278, 0.0);
    const QByteArray gga = NMEAUtils::makeGGA(coord, 0.0);

    QVERIFY(gga.contains(",0.0,M,"));
    QVERIFY(validateChecksum(gga));
}

void NTRIPGgaProviderTest::testMakeGGA_highAltitude()
{
    const QGeoCoordinate coord(27.9881, 86.9250, 8848.9);
    const QByteArray gga = NMEAUtils::makeGGA(coord, 8848.9);

    QVERIFY(gga.contains(",8848.9,M,"));
    QVERIFY(validateChecksum(gga));
}

void NTRIPGgaProviderTest::testMakeGGA_negativeAltitude()
{
    // Dead Sea: ~-430 m below mean sea level.
    const QGeoCoordinate coord(31.5, 35.5);
    const QByteArray gga = NMEAUtils::makeGGA(coord, -430.5);

    QVERIFY(gga.contains(",-430.5,M,"));
    QVERIFY(validateChecksum(gga));
}

// ---------------------------------------------------------------------------
// Coordinate precision (DDMM.mmmm)
// ---------------------------------------------------------------------------

void NTRIPGgaProviderTest::testMakeGGA_dmmPrecision()
{
    // 47.3977 deg = 47 deg 23.8620 min ; 8.5456 deg = 008 deg 32.7360 min
    const QGeoCoordinate coord(47.3977, 8.5456);
    const QByteArray gga = NMEAUtils::makeGGA(coord, 100.0);

    const QString ggaStr = QString::fromUtf8(gga);
    const QStringList fields = ggaStr.mid(1, ggaStr.indexOf('*') - 1).split(',');
    // fields: [0]=GPGGA [1]=time [2]=lat [3]=N/S [4]=lon [5]=E/W ...
    QVERIFY(fields.size() >= 6);

    QVERIFY2(fields[2].startsWith(QStringLiteral("4723")),
             qPrintable(QString("Lat field: %1").arg(fields[2])));
    QCOMPARE(fields[3], QStringLiteral("N"));
    QVERIFY2(fields[4].startsWith(QStringLiteral("00832")),
             qPrintable(QString("Lon field: %1").arg(fields[4])));
    QCOMPARE(fields[5], QStringLiteral("E"));

    QVERIFY(validateChecksum(gga));
}

UT_REGISTER_TEST(NTRIPGgaProviderTest, TestLabel::Unit)
