#pragma once

#include "StateMachineTest.h"

class QGCStateTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    void _testStateName();
    void _testStateNameInQGCStateMachine();
    UT_PARAMETERIZED_TEST(_testSignalStopsMachine);
};
