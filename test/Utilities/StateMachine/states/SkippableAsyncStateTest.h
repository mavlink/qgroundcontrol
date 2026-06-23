#pragma once

#include "StateMachineTest.h"

class SkippableAsyncStateTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    void _testSkippableAsyncStateExecute();
    void _testSkippableAsyncStateSkip();
    void _testSkippableAsyncStateTimeout();
    void _testSkippableAsyncStateWithSkipAction();
};
