#pragma once

#include "UnitTest.h"

class MotorAssignmentStateMachineTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testStateCreation();
    void _testInitializeValidConfig();
    void _testInitializeInvalidConfig();
    void _testStartAssignment();
    void _testSelectMotor();
    void _testCompleteAssignment();
    void _testAbortAssignment();
    void _testStateTransitions();
    void _testIsActive();
};
