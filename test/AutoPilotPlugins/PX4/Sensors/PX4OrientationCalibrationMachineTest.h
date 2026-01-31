#pragma once

#include "UnitTest.h"

class PX4OrientationCalibrationMachineTest : public UnitTest
{
    Q_OBJECT

public:
    PX4OrientationCalibrationMachineTest() = default;

private slots:
    void _testStateTransitions();
    void _testSideDetection();
    void _testSideCompletion();
    void _testMagCalibrationRotate();
    void _testCancelCalibration();
    void _testCalibrationComplete();
    void _testCalibrationFailed();
};
