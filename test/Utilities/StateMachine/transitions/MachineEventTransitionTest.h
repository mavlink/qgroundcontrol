#pragma once

#include "StateMachineTest.h"

class MachineEventTransitionTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    UT_PARAMETERIZED_TEST(_testMachineEventTransition);
    void _testMachineEventTransitionWithGuard();
    void _testDelayedEventCancel();
    void _testEventPriority();
};
