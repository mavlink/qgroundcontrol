#pragma once

#include "StateMachineTest.h"

class QGCAbstractStateTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    void _testEntryCallback();
    void _testExitCallback();
    void _testBothCallbacks();
    void _testOnEnterOverride();
    void _testOnLeaveOverride();
    void _testStateName();
    void _testAdvanceSignal();
    void _testErrorSignal();
    void _testEventHandler();
};
