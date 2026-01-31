#pragma once

#include "UnitTest.h"

class QGCStateTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testEntryExitCallbacks();
    void _testOnEnterOnLeaveVirtuals();
};
