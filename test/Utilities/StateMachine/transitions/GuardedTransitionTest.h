#pragma once

#include "StateMachineTest.h"

class GuardedTransitionTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    void _testGuardedTransitionAllowed();
    void _testGuardedTransitionBlocked();
};
