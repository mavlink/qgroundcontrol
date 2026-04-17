#pragma once

#include "StateMachineTest.h"

class NamedEventTransitionTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    void _testNamedEventTransition();
    void _testNamedEventTransitionWithGuard();
};
