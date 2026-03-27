#include "NTRIPManagerTest.h"
#include "NTRIPManager.h"

#include <QtTest/QTest>

void NTRIPManagerTest::testInitialStateIsDisconnected()
{
    NTRIPManager* mgr = NTRIPManager::instance();
    QVERIFY(mgr != nullptr);

    // Whatever singleton construction order produced, the public-facing
    // connection state machine must start in Disconnected. A different value
    // means the constructor raced with startNTRIP() — the bug the init()
    // refactor was meant to prevent.
    QCOMPARE(mgr->connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);
}

void NTRIPManagerTest::testStopFromIdleIsNoop()
{
    NTRIPManager* mgr = NTRIPManager::instance();
    QVERIFY(mgr != nullptr);

    // Calling stopNTRIP() while idle must not crash or emit spurious state
    // transitions; the operation state machine should early-out.
    mgr->stopNTRIP();
    QCOMPARE(mgr->connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);
}

UT_REGISTER_TEST(NTRIPManagerTest, TestLabel::Unit)
