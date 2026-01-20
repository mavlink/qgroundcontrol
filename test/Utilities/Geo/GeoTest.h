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

    void _convertGeodeticToEcef_test(void);
    void _convertEcefToGeodetic_test(void);
    void _convertGpsToEnu_test(void);
    void _convertEnuToGps_test(void);
    void _convertEcefToEnu_test(void);
    void _convertEnuToEcef_test(void);

    void _geodesicDistance_test(void);
    void _geodesicAzimuth_test(void);
    void _geodesicDestination_test(void);
    void _geodesicRoundTrip_test(void);

    void _pathLength_test(void);
    void _polygonArea_test(void);
    void _polygonPerimeter_test(void);

    void _interpolatePath_test(void);
    void _interpolateAtDistance_test(void);

private:
     /// Use ETH campus (47.3764° N, 8.5481° E)
    const QGeoCoordinate m_origin{47.3764, 8.5481, 0.0};
};
