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
///     @brief Unit test for QGCGeo coordinate transformation math.
///
///     @author David Goodman <dagoodma@gmail.com>

#ifndef GEOTEST_H
#define GEOTEST_H

#include <QGeoCoordinate>

#include "UnitTest.h"

class GeoTest : public UnitTest
{
    Q_OBJECT

public:
    GeoTest(void)
        : _origin(47.3764, 8.5481, 0.0) /// Use ETH campus (47.3764° N, 8.5481° E)
    { }

private slots:
    void _convertGeoToNed_test(void);
    void _convertGeoToNedAtOrigin_test(void);
    void _convertNedToGeo_test(void);
    void _convertNedToGeoAtOrigin_test(void);
private:
    QGeoCoordinate _origin;
};

#endif // GEOTEST_H
