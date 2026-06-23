#pragma once

#include "StateMachineTest.h"

class MachineEventTransitionTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    void _testMachineEventTransition();
    void _testDelayedEvent();
    void _testDelayedEventCancel();
    void _testEventPriority();
};
