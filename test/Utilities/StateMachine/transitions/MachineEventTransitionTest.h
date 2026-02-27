#pragma once

#include "UnitTest.h"

class MachineEventTransitionTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testMachineEventTransition();
    void _testDelayedEvent();
    void _testDelayedEventCancel();
    void _testEventPriority();
};
