#pragma once

#include "StateMachineTest.h"

class AdditionalStatesCoverageTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    void _testStatesConstructWithoutVehicle();
    UT_PARAMETERIZED_TEST(_testVehicleStatesFailWithoutVehicle);
    UT_PARAMETERIZED_TEST(_testDelayStateMachineStopEndsDelay);
    void _testDelayStateRestartStartsDelayOver();
};
