#include "NTRIPManagerTest.h"
#include "NTRIPManager.h"

#include <QtPositioning/QGeoCoordinate>

static bool verifyNmeaChecksum(const QByteArray& sentence)
{
    if (sentence.size() < 6 || sentence.at(0) != '$') {
        return false;
    }

    int star = sentence.lastIndexOf('*');
    if (star < 2 || star + 3 > sentence.size()) {
        return false;
    }

    quint8 calc = 0;
    for (int i = 1; i < star; ++i) {
        calc ^= static_cast<quint8>(sentence.at(i));
    }

    QByteArray expected = QByteArray::number(calc, 16).rightJustified(2, '0').toUpper();
    QByteArray actual = sentence.mid(star + 1, 2).toUpper();
    return actual == expected;
}

static int countFields(const QByteArray& gga)
{
    // Count comma-separated fields between $ and *
    int star = gga.lastIndexOf('*');
    if (star < 0) star = gga.size();
    QByteArray body = gga.mid(1, star - 1); // strip $ and *XX
    return body.count(',') + 1;
}

// ---------------------------------------------------------------------------
// GGA Format and Structure
// ---------------------------------------------------------------------------

void NTRIPManagerTest::_testMakeGGAFormat()
{
    QGeoCoordinate coord(47.3977, 8.5456);
    QByteArray gga = NTRIPManager::makeGGA(coord, 408.0);

    QVERIFY(gga.startsWith("$GPGGA,"));
    QVERIFY(gga.contains("*"));
    QVERIFY(!gga.contains("\r\n"));

    QVERIFY(gga.contains(",N,"));
    QVERIFY(gga.contains(",E,"));

    QVERIFY(gga.contains(",408.0,M,"));
}

void NTRIPManagerTest::_testMakeGGAFieldCount()
{
    // GGA has 15 fields: $GPGGA,hhmmss,lat,N,lon,E,qual,nsat,hdop,alt,M,geoid,M,age,refid
    QGeoCoordinate coord(40.0, -74.0);
    QByteArray gga = NTRIPManager::makeGGA(coord, 10.0);

    // GPGGA sentence: the body (between $ and *) has 15 comma-separated fields
    QCOMPARE(countFields(gga), 15);
}

void NTRIPManagerTest::_testMakeGGAChecksum()
{
    QGeoCoordinate coord(37.7749, -122.4194);
    QByteArray gga = NTRIPManager::makeGGA(coord, 16.0);

    QVERIFY(verifyNmeaChecksum(gga));
}

// ---------------------------------------------------------------------------
// GGA Hemisphere Encoding
// ---------------------------------------------------------------------------

void NTRIPManagerTest::_testMakeGGANorthEast()
{
    QGeoCoordinate coord(51.5074, 0.1278);
    QByteArray gga = NTRIPManager::makeGGA(coord, 11.0);

    QVERIFY(gga.contains(",N,"));
    QVERIFY(gga.contains(",E,"));
    QVERIFY(verifyNmeaChecksum(gga));
}

void NTRIPManagerTest::_testMakeGGASouthWest()
{
    QGeoCoordinate coord(-33.8688, -151.2093);
    QByteArray gga = NTRIPManager::makeGGA(coord, 58.0);

    QVERIFY(gga.contains(",S,"));
    QVERIFY(gga.contains(",W,"));
    QVERIFY(verifyNmeaChecksum(gga));
}

void NTRIPManagerTest::_testMakeGGAEquator()
{
    // Latitude exactly 0 should be N (>= 0.0)
    QGeoCoordinate coord(0.0, 10.0);
    QByteArray gga = NTRIPManager::makeGGA(coord, 0.0);

    QVERIFY(gga.contains(",N,"));
    QVERIFY(verifyNmeaChecksum(gga));
}

void NTRIPManagerTest::_testMakeGGADateLine()
{
    // Longitude near 180 (Fiji)
    QGeoCoordinate coordEast(17.7134, 178.065);
    QByteArray ggaEast = NTRIPManager::makeGGA(coordEast, 5.0);
    QVERIFY(ggaEast.contains(",E,"));
    QVERIFY(verifyNmeaChecksum(ggaEast));

    // Longitude near -180 (across the date line)
    QGeoCoordinate coordWest(17.7134, -179.5);
    QByteArray ggaWest = NTRIPManager::makeGGA(coordWest, 5.0);
    QVERIFY(ggaWest.contains(",W,"));
    QVERIFY(verifyNmeaChecksum(ggaWest));
}

// ---------------------------------------------------------------------------
// GGA Altitude
// ---------------------------------------------------------------------------

void NTRIPManagerTest::_testMakeGGAZeroAltitude()
{
    QGeoCoordinate coord(0.0001, 0.0001);
    QByteArray gga = NTRIPManager::makeGGA(coord, 0.0);

    QVERIFY(gga.contains(",0.0,M,"));
    QVERIFY(verifyNmeaChecksum(gga));
}

void NTRIPManagerTest::_testMakeGGAHighAltitude()
{
    QGeoCoordinate coord(27.9881, 86.9250);
    QByteArray gga = NTRIPManager::makeGGA(coord, 8848.9);

    QVERIFY(gga.contains(",8848.9,M,"));
    QVERIFY(verifyNmeaChecksum(gga));
}

void NTRIPManagerTest::_testMakeGGANegativeAltitude()
{
    // Dead Sea: ~-430m
    QGeoCoordinate coord(31.5, 35.5);
    QByteArray gga = NTRIPManager::makeGGA(coord, -430.5);

    QVERIFY(gga.contains(",-430.5,M,"));
    QVERIFY(verifyNmeaChecksum(gga));
}

// ---------------------------------------------------------------------------
// GGA Coordinate Precision
// ---------------------------------------------------------------------------

void NTRIPManagerTest::_testMakeGGADMMPrecision()
{
    // 47.3977 degrees = 47 degrees, 23.8620 minutes
    // 8.5456 degrees = 008 degrees, 32.7360 minutes
    QGeoCoordinate coord(47.3977, 8.5456);
    QByteArray gga = NTRIPManager::makeGGA(coord, 100.0);

    // Extract lat field: after "GPGGA,HHMMSS," the next field is lat
    QString ggaStr = QString::fromUtf8(gga);
    QStringList fields = ggaStr.mid(1, ggaStr.indexOf('*') - 1).split(',');
    // fields[0] = GPGGA, [1] = time, [2] = lat, [3] = N/S, [4] = lon, [5] = E/W
    QVERIFY(fields.size() >= 6);

    // Latitude: 4723.xxxx
    QString lat = fields[2];
    QVERIFY2(lat.startsWith("4723"), qPrintable(QString("Lat field: %1").arg(lat)));
    QCOMPARE(fields[3], QStringLiteral("N"));

    // Longitude: 00832.xxxx
    QString lon = fields[4];
    QVERIFY2(lon.startsWith("00832"), qPrintable(QString("Lon field: %1").arg(lon)));
    QCOMPARE(fields[5], QStringLiteral("E"));

    QVERIFY(verifyNmeaChecksum(gga));
}

// ---------------------------------------------------------------------------
// GGA Time Field
// ---------------------------------------------------------------------------

void NTRIPManagerTest::_testMakeGGATimeFormat()
{
    QGeoCoordinate coord(40.0, -74.0);
    QByteArray gga = NTRIPManager::makeGGA(coord, 10.0);

    // Extract time field (field index 1, after $GPGGA,)
    QString ggaStr = QString::fromUtf8(gga);
    QStringList fields = ggaStr.mid(1, ggaStr.indexOf('*') - 1).split(',');
    QVERIFY(fields.size() >= 2);

    const QString timeField = fields[1];
    // Should be HHMMSS.SS format (8 or more chars: 6 digits + dot + decimals)
    QVERIFY2(timeField.size() >= 6, qPrintable(QString("Time field too short: %1").arg(timeField)));
    // First 6 chars should be digits
    for (int i = 0; i < 6; ++i) {
        QVERIFY2(timeField[i].isDigit(), qPrintable(QString("Non-digit at pos %1: %2").arg(i).arg(timeField)));
    }
    // HH should be 00-23
    int hh = timeField.left(2).toInt();
    QVERIFY2(hh >= 0 && hh <= 23, qPrintable(QString("Invalid hour: %1").arg(hh)));
    // MM should be 00-59
    int mm = timeField.mid(2, 2).toInt();
    QVERIFY2(mm >= 0 && mm <= 59, qPrintable(QString("Invalid minute: %1").arg(mm)));
    // SS should be 00-59
    int ss = timeField.mid(4, 2).toInt();
    QVERIFY2(ss >= 0 && ss <= 59, qPrintable(QString("Invalid second: %1").arg(ss)));
}

UT_REGISTER_TEST(NTRIPManagerTest, TestLabel::Unit)
