#include "NTRIPGgaProviderTest.h"
#include "NTRIPGgaProvider.h"

#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QTest>

static bool validateChecksum(const QByteArray& sentence)
{
    const int starIdx = sentence.indexOf('*');
    if (starIdx < 0 || sentence.size() < starIdx + 3) return false;
    if (sentence.at(0) != '$') return false;

    quint8 cksum = 0;
    for (int i = 1; i < starIdx; ++i) {
        cksum ^= static_cast<quint8>(sentence.at(i));
    }

    const QByteArray expected = QByteArray::number(cksum, 16).toUpper().rightJustified(2, '0');
    return sentence.mid(starIdx + 1, 2) == expected;
}

void NTRIPGgaProviderTest::testMakeGGA_basicCoordinate()
{
    const QGeoCoordinate coord(47.3977, 8.5456, 450.0);
    const QByteArray gga = NTRIPGgaProvider::makeGGA(coord, 450.0);

    QVERIFY(gga.startsWith("$GPGGA,"));
    QVERIFY(gga.contains(",N,"));
    QVERIFY(gga.contains(",E,"));
    QVERIFY(gga.contains("450.0"));
    QVERIFY(validateChecksum(gga));
}

void NTRIPGgaProviderTest::testMakeGGA_negativeCoordinates()
{
    const QGeoCoordinate coord(-33.8688, -151.2093, 10.0);
    const QByteArray gga = NTRIPGgaProvider::makeGGA(coord, 10.0);

    QVERIFY(gga.contains(",S,"));
    QVERIFY(gga.contains(",W,"));
    QVERIFY(validateChecksum(gga));
}

void NTRIPGgaProviderTest::testMakeGGA_equator()
{
    const QGeoCoordinate coord(0.0, 0.0, 0.0);
    const QByteArray gga = NTRIPGgaProvider::makeGGA(coord, 0.0);

    QVERIFY(gga.contains(",0000.0000,N,00000.0000,E,"));
    QVERIFY(validateChecksum(gga));
}

void NTRIPGgaProviderTest::testMakeGGA_zeroAltitude()
{
    const QGeoCoordinate coord(51.5074, -0.1278, 0.0);
    const QByteArray gga = NTRIPGgaProvider::makeGGA(coord, 0.0);

    QVERIFY(gga.contains(",0.0,M,"));
    QVERIFY(validateChecksum(gga));
}

void NTRIPGgaProviderTest::testMakeGGA_checksumFormat()
{
    const QGeoCoordinate coord(35.6762, 139.6503, 40.0);
    const QByteArray gga = NTRIPGgaProvider::makeGGA(coord, 40.0);

    const int starIdx = gga.indexOf('*');
    QVERIFY(starIdx > 0);
    QCOMPARE(gga.size(), starIdx + 3);
}

void NTRIPGgaProviderTest::testMakeGGA_highAltitude()
{
    const QGeoCoordinate coord(27.9881, 86.9250, 8848.0);
    const QByteArray gga = NTRIPGgaProvider::makeGGA(coord, 8848.0);

    QVERIFY(gga.contains("8848.0"));
    QVERIFY(validateChecksum(gga));
}

UT_REGISTER_TEST(NTRIPGgaProviderTest, TestLabel::Unit)
