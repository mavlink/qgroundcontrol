#pragma once

#include "StateMachineTest.h"

class AsyncFunctionStateTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    void _testAsyncFunctionState();
    void _testAsyncFunctionStateTimeout();
    void _testErrorTransition();
    UT_PARAMETERIZED_TEST(_testSetupFunctionEndsState);
    UT_PARAMETERIZED_TEST(_testConnectToCompletion);
    void _testEveryCompletionConnectionEndsWithWait();
};
