/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @brief Unit test for QGCGeo coordinate transformation math.
///
///     @author David Goodman <dagoodma@gmail.com>

#pragma once

#include "UnitTest.h"

#include <QtPositioning/QGeoCoordinate>

class GeoTest : public UnitTest
{
    Q_OBJECT

public:
    GeoTest(void) = default;

private slots:
    void _convertGeoToNed_test(void);
    void _convertGeoToNedAtOrigin_test(void);
    void _convertNedToGeo_test(void);
    void _convertNedToGeoAtOrigin_test(void);

    void _convertGeoToUTM_test(void);
    void _convertUTMToGeo_test(void);
    void _convertGeoToMGRS_test(void);
    void _convertMGRSToGeo_test(void);

private:
     /// Use ETH campus (47.3764° N, 8.5481° E)
    const QGeoCoordinate m_origin{47.3764, 8.5481, 0.0};
};
