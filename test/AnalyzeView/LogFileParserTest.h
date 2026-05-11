#pragma once

#include "UnitTest.h"

class LogFileParserTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _parseULogNumericTopicTest();
    void _parseULogParameterTest();
    void _parseULogWarningEventTest();
    void _parseULogModeSegmentsTest();
    void _parseULogDropoutTest();
    void _parseULogInvalidFileTest();
    void _parseDataFlashRegressionTest();
    void _parseUnsupportedExtensionTest();
    void _fieldSamplesFilteredComprehensiveTest();
    void _gpsPathULogVehicleGlobalPositionTest();
    void _gpsPathULogVehicleGpsPositionLatDegTest();
    void _gpsPathAPMDataFlashPOSTest();
};
