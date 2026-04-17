#pragma once

#include "StateMachineTest.h"

class WaitForSignalStateTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    void _testWaitForSignalState();
    void _testWaitForSignalStateTimeout();
    void _testCompletedSignal();
    void _testTimedOutSignal();
};
