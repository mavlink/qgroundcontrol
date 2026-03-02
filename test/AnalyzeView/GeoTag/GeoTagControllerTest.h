#pragma once

#include "TempDirectoryTest.h"

class GeoTagControllerTest : public TempDirectoryTest
{
    Q_OBJECT

private slots:
    // Controller tests
    void _propertyAccessorsTest();
    void _urlPathConversionTest();
    void _validationTest();
    void _calibrationMismatchTest();
    void _fullGeotaggingTest();
    void _previewModeTest();

    // Calibrator algorithm tests
    void _calibratorEmptyInputsTest();
    void _calibratorPerfectMatchTest();
    void _calibratorToleranceMatchTest();
    void _calibratorNoMatchOutsideToleranceTest();
    void _calibratorSkipInvalidTriggersTest();
    void _calibratorSkipInvalidCoordinatesTest();
    void _calibratorUnmatchedImagesTest();
    void _calibratorTimeOffsetTest();
    void _calibratorDuplicatePreventionTest();
    void _calibratorClosestMatchTest();
};
