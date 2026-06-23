#pragma once

#include "StateMachineTest.h"

class ParallelStateTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    void _testParallelState();
    void _testParallelStateEmpty();
};
