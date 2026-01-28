#pragma once

#include "TestFixtures.h"

/// Unit tests for GeoJsonHelper coordinate loading/saving functions.
class GeoJsonHelperTest : public OfflineTest
{
    Q_OBJECT

public:
    GeoJsonHelperTest() = default;

private slots:
    // Coordinate loading tests
    void _loadGeoJsonCoordinateWithAltitudeTest();
    void _loadGeoJsonCoordinateNoAltitudeTest();
    void _loadGeoJsonCoordinateAltitudeRequiredTest();
    void _loadGeoJsonCoordinateInvalidTest();

    // Coordinate saving tests
    void _saveGeoJsonCoordinateWithAltitudeTest();
    void _saveGeoJsonCoordinateNoAltitudeTest();
};
