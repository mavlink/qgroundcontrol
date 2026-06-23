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
    void _startTimeAPMFromGwkGmsTest();
    void _startTimeAPMInvalidGwkTest();
    void _startTimePX4FromSensorGpsTest();
    void _startTimePX4ZeroUtcTest();
    void _startTimeClearedOnResetTest();
    void _parseProgressULogTest();
    void _parseProgressDataFlashTest();
    void _startParsingAsyncProgressTest();
    void _clearDuringAsyncParseTest();
};
