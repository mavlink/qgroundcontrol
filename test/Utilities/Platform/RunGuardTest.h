#pragma once

#include <memory>
#include <optional>

#include <QtCore/QTemporaryDir>

#include "Fixtures/RAIIFixtures.h"
#include "UnitTest.h"

/// Tests for RunGuard (single-instance lock mechanism)
class RunGuardTest : public UnitTest
{
    Q_OBJECT

protected slots:
    void init() override;
    void cleanup() override;

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

    // Lock file location and errors
    void _testLockFileInRuntimeDir();
    void _testLockFileErrorDoesNotBlock();

private:
    QStringList _lockFilesInRuntimeDir() const;

    std::unique_ptr<QTemporaryDir> _runtimeDir;
    std::optional<TestFixtures::EnvVarFixture> _runtimeDirEnv;
};
