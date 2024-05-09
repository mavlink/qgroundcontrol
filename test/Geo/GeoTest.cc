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

void GeoTest::_convertGeoToNed_test(void)
{
    const QGeoCoordinate coord(47.364869, 8.594398, 0.0);

    const double expectedX = -1282.58731618;
    const double expectedY = 3490.85591324;
    const double expectedZ = 0.;

    double x = 0., y = 0., z = 0.;
    QGCGeo::convertGeoToNed(coord, m_origin, x, y, z);

    QCOMPARE(x, expectedX);
    QCOMPARE(y, expectedY);
    QCOMPARE(z, expectedZ);
}

void GeoTest::_convertGeoToNedAtOrigin_test(void)
{
    QGeoCoordinate coord(m_origin);
    coord.setAltitude(10.); // offset altitude to test z

    const double expectedX = 0.;
    const double expectedY = 0.;
    const double expectedZ = -10.;

    double x = 0., y = 0., z = 0.;
    QGCGeo::convertGeoToNed(coord, m_origin, x, y, z);

    QCOMPARE(x, expectedX);
    QCOMPARE(y, expectedY);
    QCOMPARE(z, expectedZ);
}

void GeoTest::_convertNedToGeo_test(void)
{
    const double x = -1282.58731618;
    const double y = 3490.85591324;
    const double z = 0;

    QGeoCoordinate coord(0,0,0);
    QGCGeo::convertNedToGeo(x, y, z, m_origin, coord);

    QVERIFY(coord.isValid());
    QCOMPARE(coord.latitude(), m_origin.latitude());
    QCOMPARE(coord.longitude(), m_origin.longitude());
    QCOMPARE(coord.altitude(), m_origin.altitude());
}

void GeoTest::_convertNedToGeoAtOrigin_test(void)
{
    const double x = 0.0;
    const double y = 0.0;
    const double z = 0.0;

    QGeoCoordinate coord(0,0,0);
    QGCGeo::convertNedToGeo(x, y, z, m_origin, coord);

    QVERIFY(coord.isValid());
    QCOMPARE(coord.latitude(), m_origin.latitude());
    QCOMPARE(coord.longitude(), m_origin.longitude());
    QCOMPARE(coord.altitude(), m_origin.altitude());
}

void GeoTest::_convertGeoToUTM_test(void) {
    const QGeoCoordinate coord(m_origin);

    double easting = 0.;
    double northing = 0.;
    const int zone = QGCGeo::convertGeoToUTM(coord, easting, northing);

    QCOMPARE(zone, 32);
    QCOMPARE(easting, 465886.092246);
    QCOMPARE(northing, 5247092.45);
}

void GeoTest::_convertUTMToGeo_test(void) {
    const double easting = 465886.092246;
    const double northing = 5247092.45;
    const int zone = 32;
    const bool southhemi = false;

    QGeoCoordinate coord(0,0,0);
    const bool result = QGCGeo::convertUTMToGeo(easting, northing, zone, southhemi, coord);

    QVERIFY(result);
    QVERIFY(coord.isValid());
    QCOMPARE(coord.latitude(), m_origin.latitude());
    QCOMPARE(coord.longitude(), m_origin.longitude());
    QCOMPARE(coord.altitude(), m_origin.altitude());
}

void GeoTest::_convertGeoToMGRS_test(void) {
    const QGeoCoordinate coord(m_origin);

    const QString result = QGCGeo::convertGeoToMGRS(coord);

    QCOMPARE(result, "32T MT 65886 47092");
}

void GeoTest::_convertMGRSToGeo_test(void) {
    const QString mgrs("32T MT 65886 47092");

    QGeoCoordinate coord(0,0,0);
    const bool result = QGCGeo::convertMGRSToGeo(mgrs, coord);

    QVERIFY(result);
    QVERIFY(coord.isValid());
    QCOMPARE(coord.latitude(), m_origin.latitude());
    QCOMPARE(coord.longitude(), m_origin.longitude());
    QCOMPARE(coord.altitude(), m_origin.altitude());
}
