#pragma once

#include "StateMachineTest.h"

class AdditionalTransitionsCoverageTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    void _testMachineEventTransitionAccessors();
    UT_PARAMETERIZED_TEST(_testGuardedTransitionGuardDecides);
    void _testGuardedTransitionTargetSetLater();
    UT_PARAMETERIZED_TEST(_testRetryTransitionRetriesThenAdvances);
    void _testRetryTransitionWaitRearmedBeforeRetryAction();
};
