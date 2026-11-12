#pragma once

#include "UnitTest.h"

class AutoSuspendGuardTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testSuspendsForItsLifetime();
    void _testMovedFromGuardLeavesFlagSuspended();
    void _testMoveAssignmentReleasesPreviousFlag();
};
