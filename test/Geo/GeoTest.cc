/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @brief Unit test fpr QGCGeo coordinate project math.
///
///     @author David Goodman <dagoodma@gmail.com>

#include "GeoTest.h"
#include "QGCGeo.h"

#include <QtTest/QTest>

static bool compareDoubles(double actual, double expected, double epsilon = 0.00001)
{
    return (qAbs(actual - expected) <= epsilon);
}

void GeoTest::_convertGeoToNed_test()
{
    const QGeoCoordinate coord(47.364869, 8.594398, 0.0);

    const double expectedX = -1282.58731618;
    const double expectedY = 3490.85591324;
    const double expectedZ = 0.;

    double x = 0., y = 0., z = 0.;
    QGCGeo::convertGeoToNed(coord, m_origin, x, y, z);

    QVERIFY(compareDoubles(x, expectedX));
    QVERIFY(compareDoubles(y, expectedY));
    QVERIFY(compareDoubles(z, expectedZ));
}

void GeoTest::_convertGeoToNedAtOrigin_test()
{
    QGeoCoordinate coord(m_origin);
    coord.setAltitude(10.); // offset altitude to test z

    const double expectedX = 0.;
    const double expectedY = 0.;
    const double expectedZ = -10.;

    double x = 0., y = 0., z = 0.;
    QGCGeo::convertGeoToNed(coord, m_origin, x, y, z);

    QVERIFY(compareDoubles(x, expectedX));
    QVERIFY(compareDoubles(y, expectedY));
    QVERIFY(compareDoubles(z, expectedZ));
}

void GeoTest::_convertNedToGeo_test()
{
    const double x = -1282.58731618;
    const double y = 3490.85591324;
    const double z = 0.;

    QGeoCoordinate coord(0,0,0);
    QGCGeo::convertNedToGeo(x, y, z, m_origin, coord);

    QVERIFY(coord.isValid());
    QVERIFY(compareDoubles(coord.latitude(), 47.364869));
    QVERIFY(compareDoubles(coord.longitude(), 8.594398));
    QVERIFY(compareDoubles(coord.altitude(), 0.0));
}

void GeoTest::_convertNedToGeoAtOrigin_test()
{
    const double x = 0.0;
    const double y = 0.0;
    const double z = 0.0;

    QGeoCoordinate coord(0,0,0);
    QGCGeo::convertNedToGeo(x, y, z, m_origin, coord);

    QVERIFY(coord.isValid());
    QVERIFY(compareDoubles(coord.latitude(), m_origin.latitude()));
    QVERIFY(compareDoubles(coord.longitude(), m_origin.longitude()));
    QVERIFY(compareDoubles(coord.altitude(), m_origin.altitude()));
}

void GeoTest::_convertGeoToUTM_test()
{
    const QGeoCoordinate coord(m_origin);

    double easting = 0.;
    double northing = 0.;
    const int zone = QGCGeo::convertGeoToUTM(coord, easting, northing);

    QCOMPARE(zone, 32);
    QVERIFY(compareDoubles(easting, 465886.092246));
    QVERIFY(compareDoubles(northing, 5247092.44892));
}

void GeoTest::_convertUTMToGeo_test()
{
    const double easting = 465886.092246;
    const double northing = 5247092.44892;
    const int zone = 32;
    const bool southhemi = false;

    QGeoCoordinate coord(0,0,0);
    const bool result = QGCGeo::convertUTMToGeo(easting, northing, zone, southhemi, coord);

    QVERIFY(result);
    QVERIFY(coord.isValid());
    QVERIFY(compareDoubles(coord.latitude(), m_origin.latitude()));
    QVERIFY(compareDoubles(coord.longitude(), m_origin.longitude()));
    QVERIFY(compareDoubles(coord.altitude(), m_origin.altitude()));
}

void GeoTest::_convertGeoToMGRS_test()
{
    const QGeoCoordinate coord(m_origin);

    const QString result = QGCGeo::convertGeoToMGRS(coord);

    QCOMPARE(result, "32TMT 65886 47092");
}

void GeoTest::_convertMGRSToGeo_test()
{
    const QString mgrs("32T MT 65886 47092");

    QGeoCoordinate coord(0,0,0);
    const bool result = QGCGeo::convertMGRSToGeo(mgrs, coord);

    QVERIFY(result);
    QVERIFY(coord.isValid());
    QVERIFY(compareDoubles(coord.latitude(), m_origin.latitude()));
    QVERIFY(compareDoubles(coord.longitude(), m_origin.longitude()));
    QVERIFY(compareDoubles(coord.altitude(), m_origin.altitude()));
}
