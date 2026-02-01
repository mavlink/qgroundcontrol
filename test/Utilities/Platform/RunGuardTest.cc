#include "RunGuardTest.h"

#include <QtCore/QUuid>

#include "RunGuard.h"
#include "UnitTest.h"

// Generate unique keys for each test to avoid interference
static QString uniqueKey()
{
    return QStringLiteral("RunGuardTest_") + QUuid::createUuid().toString(QUuid::WithoutBraces);
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
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
#endif

UT_REGISTER_TEST(RunGuardTest, TestLabel::Unit, TestLabel::Utilities)
