#include "RunGuardTest.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QUuid>

#include "RunGuard.h"
#include "UnitTest.h"

// Generate unique keys for each test to avoid interference
static QString uniqueKey()
{
    return QStringLiteral("RunGuardTest_") + QUuid::createUuid().toString(QUuid::WithoutBraces);
}

void RunGuardTest::init()
{
    UnitTest::init();

    // On XDG platforms the lock lives in the runtime directory: use a private one (0700, as Qt requires),
    // which also keeps Qt from warning when the environment has none.
    _runtimeDir = std::make_unique<QTemporaryDir>();
    QVERIFY(_runtimeDir->isValid());
    _runtimeDirEnv.emplace("XDG_RUNTIME_DIR");
    qputenv("XDG_RUNTIME_DIR", QFile::encodeName(_runtimeDir->path()));
}

void RunGuardTest::cleanup()
{
    _runtimeDirEnv.reset();
    _runtimeDir.reset();

    UnitTest::cleanup();
}

QStringList RunGuardTest::_lockFilesInRuntimeDir() const
{
    return QDir(_runtimeDir->path()).entryList({QStringLiteral("qgc-*.lock")}, QDir::Files);
}

void RunGuardTest::_testInitialState()
{
    RunGuard guard(uniqueKey());
    // Initially should not be locked
    QVERIFY(!guard.isLocked());
}

void RunGuardTest::_testTryToRunSucceeds()
{
    RunGuard guard(uniqueKey());
    // First tryToRun should succeed
    QVERIFY(guard.tryToRun());
    QVERIFY(guard.isLocked());
}

void RunGuardTest::_testReleaseUnlocks()
{
    RunGuard guard(uniqueKey());
    QVERIFY(guard.tryToRun());
    QVERIFY(guard.isLocked());
    guard.release();
    QVERIFY(!guard.isLocked());
}

void RunGuardTest::_testDestructorReleases()
{
    const QString key = uniqueKey();
    {
        RunGuard guard(key);
        QVERIFY(guard.tryToRun());
        QVERIFY(guard.isLocked());
        // guard goes out of scope here
    }
    // After destruction, a new guard should be able to acquire the lock
    RunGuard guard2(key);
    QVERIFY(guard2.tryToRun());
    QVERIFY(guard2.isLocked());
}

void RunGuardTest::_testSecondGuardBlocked()
{
    const QString key = uniqueKey();
    RunGuard guard1(key);
    QVERIFY(guard1.tryToRun());
    QVERIFY(guard1.isLocked());
    // Second guard with same key should fail to acquire
    RunGuard guard2(key);
    QVERIFY(!guard2.tryToRun());
    QVERIFY(!guard2.isLocked());
    // First guard should still be locked
    QVERIFY(guard1.isLocked());
}

void RunGuardTest::_testSecondGuardSucceedsAfterRelease()
{
    const QString key = uniqueKey();
    RunGuard guard1(key);
    QVERIFY(guard1.tryToRun());
    RunGuard guard2(key);
    QVERIFY(!guard2.tryToRun());
    // Release first guard
    guard1.release();
    QVERIFY(!guard1.isLocked());
    // Now second guard should succeed
    QVERIFY(guard2.tryToRun());
    QVERIFY(guard2.isLocked());
}

void RunGuardTest::_testIsAnotherRunningNoLock()
{
    RunGuard guard(uniqueKey());
    // No lock held, so no other instance is running
    QVERIFY(!guard.isAnotherRunning());
}

void RunGuardTest::_testIsAnotherRunningWithLock()
{
    const QString key = uniqueKey();
    RunGuard guard1(key);
    QVERIFY(guard1.tryToRun());
    // From guard1's perspective, no other is running (we hold the lock)
    QVERIFY(!guard1.isAnotherRunning());
    // From guard2's perspective, another is running
    RunGuard guard2(key);
    QVERIFY(guard2.isAnotherRunning());
}

void RunGuardTest::_testDifferentKeysIndependent()
{
    RunGuard guard1(uniqueKey());
    RunGuard guard2(uniqueKey());
    // Both should succeed since they have different keys
    QVERIFY(guard1.tryToRun());
    QVERIFY(guard2.tryToRun());
    QVERIFY(guard1.isLocked());
    QVERIFY(guard2.isLocked());
}

void RunGuardTest::_testLockFileInRuntimeDir()
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
    RunGuard guard(uniqueKey());
    QVERIFY(_lockFilesInRuntimeDir().isEmpty());
    QVERIFY(guard.tryToRun());
    QCOMPARE(_lockFilesInRuntimeDir().size(), 1);
    guard.release();
    QVERIFY(_lockFilesInRuntimeDir().isEmpty());
#else
    QSKIP("The temporary directory, where the lock is kept, is already per-user on this platform");
#endif
}

void RunGuardTest::_testLockFileErrorDoesNotBlock()
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
    RunGuard guard(uniqueKey());

    // Without its directory the lock file cannot be created, which is not "another instance is running"
    QVERIFY(_runtimeDir->remove());

    expectLogMessage("Utilities.RunGuard", QtWarningMsg, QRegularExpression(QStringLiteral("lock file")));
    QVERIFY(guard.tryToRun());
    verifyExpectedLogMessage();
    QVERIFY(!guard.isLocked());

    expectLogMessage("Utilities.RunGuard", QtWarningMsg, QRegularExpression(QStringLiteral("lock file")));
    QVERIFY(!guard.isAnotherRunning());
    verifyExpectedLogMessage();
#else
    QSKIP("Needs a lock directory that the test can remove");
#endif
}

UT_REGISTER_TEST(RunGuardTest, TestLabel::Unit, TestLabel::Utilities)
