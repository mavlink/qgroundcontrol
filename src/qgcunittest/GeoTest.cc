/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

/// @file
///     @brief Unit test fpr QGCGeo coordinate project math.
///
///     @author David Goodman <dagoodma@gmail.com>

#include "GeoTest.h"
#include "QGCGeo.h"

/*
GeoTest::GeoTest(void)
{

}
*/

void GeoTest::_convertGeoToNed_test(void)
{
    QGeoCoordinate coord(47.364869, 8.594398, 0.0);

    double expectedX = -1281.152128182419801305514;
    double expectedY = 3486.949719522415307437768;
    double expectedZ = 0;

    double x, y, z;
    convertGeoToNed(coord, _origin, &x, &y, &z);

    QCOMPARE(x, expectedX);
    QCOMPARE(y, expectedY);
    QCOMPARE(z, expectedZ);
}

void GeoTest::_convertGeoToNedAtOrigin_test(void)
{
    QGeoCoordinate coord(_origin);
    coord.setAltitude(10.0); // offset altitude to test z

    double expectedX = 0.0;
    double expectedY = 0.0;
    double expectedZ = -10.0;

    double x, y, z;
    convertGeoToNed(coord, _origin, &x, &y, &z);

    QCOMPARE(x, expectedX);
    QCOMPARE(y, expectedY);
    QCOMPARE(z, expectedZ);
}

void GeoTest::_convertNedToGeo_test(void)
{
    double x = -1281.152128182419801305514;
    double y = 3486.949719522415307437768;
    double z = 0;

    double expectedLat = 47.364869;
    double expectedLon = 8.594398;
    double expectedAlt = 0.0;

    QGeoCoordinate coord;
    convertNedToGeo(x, y, z, _origin, &coord);

    QCOMPARE(coord.latitude(), expectedLat);
    QCOMPARE(coord.longitude(), expectedLon);
    QCOMPARE(coord.altitude(), expectedAlt);
}

void GeoTest::_convertNedToGeoAtOrigin_test(void)
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    double expectedLat = _origin.latitude();
    double expectedLon = _origin.longitude();
    double expectedAlt = _origin.altitude();

    QGeoCoordinate coord;
    convertNedToGeo(x, y, z, _origin, &coord);

    QCOMPARE(coord.latitude(), expectedLat);
    QCOMPARE(coord.longitude(), expectedLon);
    QCOMPARE(coord.altitude(), expectedAlt);
}
