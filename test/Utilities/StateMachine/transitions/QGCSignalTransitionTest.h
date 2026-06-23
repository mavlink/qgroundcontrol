#pragma once

#include "StateMachineTest.h"

class QGCSignalTransitionTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    void _testTransitionMachineAccessor();
    void _testTransitionMachineAccessorAbstract();
};
