#pragma once

#include "UnitTest.h"

class PowerCalibrationStateMachineTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testStateCreation();
    void _testStartEscCalibration();
    void _testStartBusConfig();
    void _testConnectBatteryMessage();
    void _testBatteryConnectedMessage();
    void _testCalibrationSuccess();
    void _testCalibrationFailed();
    void _testDisconnectBattery();
    void _testWarningMessages();
    void _testFullEscCalibrationWorkflow();
    void _testStop();
};
