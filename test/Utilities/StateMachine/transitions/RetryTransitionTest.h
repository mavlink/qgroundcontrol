#pragma once

#include "StateMachineTest.h"

class RetryTransitionTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    UT_PARAMETERIZED_TEST(_testRetriesThenAdvances);
    void _testWaitRearmedBeforeRetryAction();
};
