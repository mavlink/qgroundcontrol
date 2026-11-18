#pragma once

#include "StateMachineTest.h"

class QGCAbstractStateTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    UT_PARAMETERIZED_TEST(_testEntryAndExitHooks);
    void _testStateName();
    void _testStateNameInQGCStateMachine();
    UT_PARAMETERIZED_TEST(_testSignalStopsMachine);
    void _testEventHandler();
};
