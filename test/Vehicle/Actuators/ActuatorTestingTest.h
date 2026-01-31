#pragma once

#include "UnitTest.h"

class ActuatorTestingTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testCreation();
    void _testUpdateFunctions();
    void _testSetActive();
    void _testSetChannelTo();
    void _testStopControl();
    void _testAllMotorsActuator();
};
