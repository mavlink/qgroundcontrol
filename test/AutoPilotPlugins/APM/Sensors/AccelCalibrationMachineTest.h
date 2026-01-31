#pragma once

#include "UnitTest.h"

class AccelCalibrationMachineTest : public UnitTest
{
    Q_OBJECT

public:
    AccelCalibrationMachineTest() = default;

private slots:
    void _testStateTransitions();
    void _testPositionCommands();
    void _testCancelCalibration();
    void _testProgressTracking();
    void _testFailedCalibration();
};
