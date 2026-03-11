#pragma once

#include "UnitTest.h"

/// Tests for RunGuard (single-instance lock mechanism)
class RunGuardTest : public UnitTest
{
    Q_OBJECT

private slots:
    // Basic lock lifecycle
    void _testInitialState();
    void _testTryToRunSucceeds();
    void _testReleaseUnlocks();
    void _testDestructorReleases();

    // Multiple guards with same key
    void _testSecondGuardBlocked();
    void _testSecondGuardSucceedsAfterRelease();

    // isAnotherRunning behavior
    void _testIsAnotherRunningNoLock();
    void _testIsAnotherRunningWithLock();

    // Different keys don't interfere
    void _testDifferentKeysIndependent();
};
