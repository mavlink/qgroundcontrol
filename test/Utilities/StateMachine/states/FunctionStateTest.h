#pragma once

#include "StateMachineTest.h"

class FunctionStateTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    void _testFunctionState();
    void _testFunctionStateChain();
};
