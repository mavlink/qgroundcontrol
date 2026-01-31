#pragma once

#include "UnitTest.h"

class MotorAssignmentTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testCreation();
    void _testInitAssignment();
    void _testStart();
    void _testSelectMotor();
    void _testAbort();
    void _testActiveProperty();
    void _testMessageProperty();
};
