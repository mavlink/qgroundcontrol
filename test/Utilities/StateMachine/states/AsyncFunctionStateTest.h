#pragma once

#include "UnitTest.h"

class AsyncFunctionStateTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testAsyncFunctionState();
    void _testAsyncFunctionStateTimeout();
    void _testErrorTransition();
};
