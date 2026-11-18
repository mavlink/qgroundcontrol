#pragma once

#include "StateMachineTest.h"

class SkippableAsyncStateTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    void _testSkippableAsyncStateExecute();
    UT_PARAMETERIZED_TEST(_testSkippableAsyncStateSkip);
    void _testSkippedBeforeMachineStops();
    void _testSkippableAsyncStateTimeout();
    UT_PARAMETERIZED_TEST(_testConnectToCompletion);
};
