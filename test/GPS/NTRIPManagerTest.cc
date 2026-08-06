#include "NTRIPManagerTest.h"

#include <QtCore/QChronoTimer>
#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "NTRIPManager.h"

void NTRIPManagerTest::cleanup()
{
    // Tests share the NTRIPManager singleton; leave it Disconnected so order
    // cannot leak state between cases.
    NTRIPManager::instance()->stopNTRIP();
    UnitTest::cleanup();
}

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

void NTRIPManagerTest::testPlaintextCredentialWarningIsVisibleState()
{
    NTRIPManager mgr;
    QSignalSpy warningSpy(&mgr, &NTRIPManager::securityWarningChanged);

    ignoreLogMessage("GPS.NTRIPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Credentials sent without TLS encryption")));

    QVERIFY(mgr.securityWarning().isEmpty());
    mgr._onPlaintextCredentialsWarning();

    QCOMPARE(warningSpy.count(), 1);
    QVERIFY(mgr.securityWarning().contains(QStringLiteral("without TLS")));
}

void NTRIPManagerTest::testErrorStateStopsUdpForwarder()
{
    NTRIPManager mgr;

    NTRIPTransportConfig config;
    config.udpForwardEnabled = true;
    config.udpTargetAddress = QStringLiteral("127.0.0.1");
    config.udpTargetPort = 2101;

    mgr._applyUdpForwarderConfig(config);
    QVERIFY(mgr._udpForwarder.isEnabled());

    mgr._enterState(NTRIPManager::ConnectionStatus::Error, QStringLiteral("bad config"));
    QVERIFY(!mgr._udpForwarder.isEnabled());
}

// ---------------------------------------------------------------------------
// Reconnect backoff (migrated from NTRIPReconnectPolicyTest)
// ---------------------------------------------------------------------------

void NTRIPManagerTest::testReconnectInitialBackoff()
{
    NTRIPManager mgr;
    QCOMPARE(mgr._reconnectAttempts, 0);
    QVERIFY(!mgr._reconnectTimer.isActive());
    QCOMPARE(mgr._reconnectBackoffMs(), NTRIPManager::kMinReconnectMs);
}

void NTRIPManagerTest::testReconnectExponentialBackoff()
{
    NTRIPManager mgr;

    QCOMPARE(mgr._reconnectBackoffMs(), 1000);

    mgr._scheduleReconnect();
    QCOMPARE(mgr._reconnectAttempts, 1);
    QCOMPARE(mgr._reconnectBackoffMs(), 2000);
    mgr._cancelReconnect();

    mgr._scheduleReconnect();
    QCOMPARE(mgr._reconnectAttempts, 2);
    QCOMPARE(mgr._reconnectBackoffMs(), 4000);
    mgr._cancelReconnect();

    mgr._scheduleReconnect();
    QCOMPARE(mgr._reconnectAttempts, 3);
    QCOMPARE(mgr._reconnectBackoffMs(), 8000);
    mgr._cancelReconnect();

    mgr._scheduleReconnect();
    QCOMPARE(mgr._reconnectAttempts, 4);
    QCOMPARE(mgr._reconnectBackoffMs(), 16000);
    mgr._cancelReconnect();
}

void NTRIPManagerTest::testReconnectMaxBackoff()
{
    NTRIPManager mgr;
    for (int i = 0; i < 20; ++i) {
        mgr._scheduleReconnect();
        mgr._cancelReconnect();
    }
    QVERIFY(mgr._reconnectBackoffMs() <= NTRIPManager::kMaxReconnectMs);
}

void NTRIPManagerTest::testReconnectCancelStopsTimer()
{
    NTRIPManager mgr;
    mgr._scheduleReconnect();
    QVERIFY(mgr._reconnectTimer.isActive());
    mgr._cancelReconnect();
    QVERIFY(!mgr._reconnectTimer.isActive());
}

void NTRIPManagerTest::testReconnectResetAttempts()
{
    NTRIPManager mgr;
    mgr._scheduleReconnect();
    mgr._cancelReconnect();
    mgr._scheduleReconnect();
    mgr._cancelReconnect();
    QCOMPARE(mgr._reconnectAttempts, 2);

    mgr._resetReconnectAttempts();
    QCOMPARE(mgr._reconnectAttempts, 0);
    QCOMPARE(mgr._reconnectBackoffMs(), NTRIPManager::kMinReconnectMs);
}

void NTRIPManagerTest::testReconnectSignalFires()
{
    NTRIPManager mgr;
    // The timer callback dispatches ReconnectDue; a fresh manager is Disconnected
    // where ReconnectDue has no transition (no-op), so observe the timer instead.
    mgr._scheduleReconnect();
    QVERIFY(mgr._reconnectTimer.isActive());
    // Replace the production backoff (kMinReconnectMs) with a short interval so the
    // single-shot fire is observed well within the wait timeout on loaded CI.
    using namespace std::chrono_literals;
    mgr._reconnectTimer.setInterval(50ms);
    mgr._reconnectTimer.start();
    QSignalSpy spy(&mgr._reconnectTimer, &QChronoTimer::timeout);
    QVERIFY(spy.wait(2000));
    QCOMPARE(spy.count(), 1);
    QVERIFY(!mgr._reconnectTimer.isActive());
}

UT_REGISTER_TEST(NTRIPManagerTest, TestLabel::Unit)
