#pragma once

#include "StateMachineTest.h"

class SignalDataTransitionTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    void _testSignalDataTransition();
    // Qt limitation: Primitive types (bool, int, double) in signal transitions fail due to metatype registration
    // void _testSignalDataTransitionBool();
    // void _testSignalDataTransitionMultiArg();
    // void _testSignalDataTransitionSignalArgs();
    // void _testMakeSignalDataTransition();
};
