#pragma once

#include "UnitTest.h"

class ActuatorTestingStateMachineTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testStateCreation();
    void _testActivateDeactivate();
    void _testSetChannelTo();
    void _testStopControl();
    void _testStopAllControl();
    void _testActuatorStateTracking();
    void _testHasActiveActuators();
    void _testResetStates();
};
