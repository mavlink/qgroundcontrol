#pragma once

#include "StateMachineTest.h"

class TimeoutTransitionTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    void _testTimeoutFires();
    void _testTimeoutCancelledOnExit();
    void _testTimeoutRestartsOnReentry();
    void _testMultipleTimeoutTransitions();
};
