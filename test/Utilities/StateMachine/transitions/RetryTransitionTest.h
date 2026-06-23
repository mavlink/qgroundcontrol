#pragma once

#include "StateMachineTest.h"

class RetryTransitionTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    void _testRetryActionCalled();
    void _testTransitionAfterMaxRetries();
    void _testRetryCountResets();
    void _testMultipleRetries();
    void _testZeroRetries();
    void _testWaitRearmedBeforeRetryAction();
};
