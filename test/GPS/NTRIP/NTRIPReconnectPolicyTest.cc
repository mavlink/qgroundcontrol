#include "NTRIPReconnectPolicyTest.h"
#include "NTRIPReconnectPolicy.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void NTRIPReconnectPolicyTest::testInitialState()
{
    NTRIPReconnectPolicy policy;
    QCOMPARE(policy.attempts(), 0);
    QVERIFY(!policy.isPending());
    QCOMPARE(policy.nextBackoffMs(), NTRIPReconnectPolicy::kMinReconnectMs);
}

void NTRIPReconnectPolicyTest::testExponentialBackoff()
{
    NTRIPReconnectPolicy policy;

    QCOMPARE(policy.nextBackoffMs(), 1000);

    policy.scheduleReconnect();
    QCOMPARE(policy.attempts(), 1);
    QCOMPARE(policy.nextBackoffMs(), 2000);
    policy.cancel();

    policy.scheduleReconnect();
    QCOMPARE(policy.attempts(), 2);
    QCOMPARE(policy.nextBackoffMs(), 4000);
    policy.cancel();

    policy.scheduleReconnect();
    QCOMPARE(policy.attempts(), 3);
    QCOMPARE(policy.nextBackoffMs(), 8000);
    policy.cancel();

    policy.scheduleReconnect();
    QCOMPARE(policy.attempts(), 4);
    QCOMPARE(policy.nextBackoffMs(), 16000);
    policy.cancel();
}

void NTRIPReconnectPolicyTest::testMaxBackoff()
{
    NTRIPReconnectPolicy policy;

    for (int i = 0; i < 20; ++i) {
        policy.scheduleReconnect();
        policy.cancel();
    }

    QVERIFY(policy.nextBackoffMs() <= NTRIPReconnectPolicy::kMaxReconnectMs);
}

void NTRIPReconnectPolicyTest::testCancelStopsTimer()
{
    NTRIPReconnectPolicy policy;
    policy.scheduleReconnect();
    QVERIFY(policy.isPending());

    policy.cancel();
    QVERIFY(!policy.isPending());
}

void NTRIPReconnectPolicyTest::testResetAttempts()
{
    NTRIPReconnectPolicy policy;
    policy.scheduleReconnect();
    policy.cancel();
    policy.scheduleReconnect();
    policy.cancel();
    QCOMPARE(policy.attempts(), 2);

    policy.resetAttempts();
    QCOMPARE(policy.attempts(), 0);
    QCOMPARE(policy.nextBackoffMs(), NTRIPReconnectPolicy::kMinReconnectMs);
}

void NTRIPReconnectPolicyTest::testReconnectSignal()
{
    NTRIPReconnectPolicy policy;
    QSignalSpy spy(&policy, &NTRIPReconnectPolicy::reconnectRequested);

    policy.scheduleReconnect();
    QVERIFY(spy.wait(2000));
    QCOMPARE(spy.count(), 1);
    QVERIFY(!policy.isPending());
}

UT_REGISTER_TEST(NTRIPReconnectPolicyTest, TestLabel::Unit)
