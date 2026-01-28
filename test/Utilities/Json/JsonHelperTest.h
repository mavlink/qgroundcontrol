#pragma once

#include "TestFixtures.h"

/// Unit tests for JsonHelper utility functions.
class JsonHelperTest : public OfflineTest
{
    Q_OBJECT

public:
    JsonHelperTest() = default;

private slots:
    // JSON parsing tests
    void _isJsonFileValidTest();
    void _isJsonFileInvalidTest();
    void _isJsonFileBytesTest();

    // Header validation tests
    void _saveAndValidateHeaderTest();
    void _validateExternalQGCJsonFileTest();
    void _validateInternalQGCJsonFileTest();
    void _validateVersionRangeTest();

    // Key validation tests
    void _validateRequiredKeysTest();
    void _validateKeyTypesTest();
    void _validateKeysTest();

    // GeoCoordinate tests
    void _loadGeoCoordinateTest();
    void _loadGeoCoordinateNoAltitudeTest();
    void _loadGeoCoordinateGeoJsonFormatTest();
    void _saveGeoCoordinateTest();
    void _loadGeoCoordinateArrayTest();
    void _saveGeoCoordinateArrayTest();

    // Utility tests
    void _possibleNaNJsonValueTest();
};
