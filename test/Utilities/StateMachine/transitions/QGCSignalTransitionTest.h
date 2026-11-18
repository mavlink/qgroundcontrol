#pragma once

#include "StateMachineTest.h"

class QGCSignalTransitionTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    void _testTransitionMachineAccessor();
    void _testTransitionMachineAccessorAbstract();
    UT_PARAMETERIZED_TEST(_testSignalForms);
};
