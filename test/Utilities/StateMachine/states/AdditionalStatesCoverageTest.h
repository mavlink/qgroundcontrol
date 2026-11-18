#pragma once

#include "StateMachineTest.h"

class AdditionalStatesCoverageTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    void _testStatesConstructWithoutVehicle();
    UT_PARAMETERIZED_TEST(_testVehicleStatesFailWithoutVehicle);
    void _testFunctionState();
    void _testFunctionStateChain();
    void _testDelayState();
    UT_PARAMETERIZED_TEST(_testDelayStateMachineStopEndsDelay);
    void _testDelayStateRestartStartsDelayOver();
    void _testQGCStateEntryExitCallbacks();
    void _testQGCStateOnEnterOnLeaveVirtuals();
    void _testQGCFinalState();
    UT_PARAMETERIZED_TEST(_testWaitForMavlinkMessageCompletesOnFirstMatch);
};
