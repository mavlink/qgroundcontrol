#pragma once

#include "UnitTest.h"

class PX4SensorCalibrationStateMachineTest : public UnitTest
{
    Q_OBJECT

public:
    PX4SensorCalibrationStateMachineTest() = default;

private slots:
    void _testInitialState();
    void _testCompassCalibration();
    void _testGyroCalibration();
    void _testAccelCalibration();
    void _testLevelCalibration();
    void _testAirspeedCalibration();
    void _testCancelCalibration();
    void _testTextMessageHandling();
    void _testProgressParsing();
    void _testFirmwareVersionCheck();
};
