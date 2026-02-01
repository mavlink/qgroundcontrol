#pragma once

#include <QtPositioning/QGeoCoordinate>

#include "UnitTest.h"

class GeoTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _convertGeoToNed_test();
    void _convertGeoToNedAtOrigin_test();
    void _convertNedToGeo_test();
    void _convertNedToGeoAtOrigin_test();

    void _convertGeoToUTM_test();
    void _convertUTMToGeo_test();
    void _convertGeoToMGRS_test();
    void _convertMGRSToGeo_test();

    void _convertGeodeticToEcef_test();
    void _convertEcefToGeodetic_test();
    void _convertGpsToEnu_test();
    void _convertEnuToGps_test();
    void _convertEcefToEnu_test();
    void _convertEnuToEcef_test();

    void _geodesicDistance_test();
    void _geodesicAzimuth_test();
    void _geodesicDestination_test();
    void _geodesicRoundTrip_test();

    void _pathLength_test();
    void _polygonArea_test();
    void _polygonPerimeter_test();

    void _interpolatePath_test();
    void _interpolateAtDistance_test();

    // Property-based tests
    void _distanceProperties_test();
    void _nedRoundtripProperty_test();

    // Benchmarks (nanobench)
    void _benchmarkCoordinateConversions();

    // Benchmarks (QBENCHMARK)
    void _qbenchmarkGeodesicDistance();

private:
    /// Use ETH campus (47.3764° N, 8.5481° E)
    const QGeoCoordinate m_origin{47.3764, 8.5481, 0.0};
};
