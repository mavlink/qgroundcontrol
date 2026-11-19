#pragma once

#include "StateMachineTest.h"

class AdditionalTransitionsCoverageTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    UT_PARAMETERIZED_TEST(_testGuardedTransitionGuardDecides);
    void _testGuardedTransitionTargetSetLater();
    UT_PARAMETERIZED_TEST(_testRetryTransitionRetriesThenAdvances);
    void _testRetryTransitionWaitRearmedBeforeRetryAction();
    UT_PARAMETERIZED_TEST(_testRetrySetsAsyncStateUpAgain);
    void _testRetryTransitionCountStartsAgainOnEntry();
};
