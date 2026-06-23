#pragma once

#include "StateMachineTest.h"

class QGCStateTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    void _testEntryExitCallbacks();
    void _testOnEnterOnLeaveVirtuals();
};
