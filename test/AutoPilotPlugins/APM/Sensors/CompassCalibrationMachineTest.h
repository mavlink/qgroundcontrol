#pragma once

#include "UnitTest.h"

class CompassCalibrationMachineTest : public UnitTest
{
    Q_OBJECT

public:
    CompassCalibrationMachineTest() = default;

private slots:
    void _testStateTransitions();
    void _testProgressTracking();
    void _testCancelCalibration();
    void _testAllCompassesComplete();
    void _testPartialFailure();
};
