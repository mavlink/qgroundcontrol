#pragma once

#include "StateMachineTest.h"

class WaitStateBaseTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    void _testTimeoutEmission();
    UT_PARAMETERIZED_TEST(_testCompleteAndFail);
    UT_PARAMETERIZED_TEST(_testMachineStopEndsWait);
    void _testMachineStopAfterExitEndsNothing();
    void _testRestartKeepsOneCompletionConnection();
    void _testNoTimeoutWhenZero();
    void _testTimeoutCancelledOnComplete();
};
