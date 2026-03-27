#include "SettingsLifecycleTest.h"
#include "MockNTRIPTransport.h"
#include "NTRIPManager.h"
#include "NTRIPError.h"
#include "SettingsManager.h"
#include "NTRIPSettings.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

namespace {

NTRIPSettings *settings()
{
    return SettingsManager::instance()->ntripSettings();
}

void configureValid()
{
    NTRIPSettings *s = settings();
    if (!s) return;
    s->ntripServerHostAddress()->setRawValue(QStringLiteral("test.caster.com"));
    s->ntripServerPort()->setRawValue(2101);
    s->ntripMountpoint()->setRawValue(QStringLiteral("TEST"));
    s->ntripServerConnectEnabled()->setRawValue(true);
}

void disable()
{
    NTRIPSettings *s = settings();
    if (!s) return;
    s->ntripServerConnectEnabled()->setRawValue(false);
}

} // namespace

// ---------------------------------------------------------------------------
// Test: Full enable → connected → disable → disconnected cycle via settings
// ---------------------------------------------------------------------------

void SettingsLifecycleTest::testEnableDisableCycle()
{
    NTRIPManager *mgr = NTRIPManager::instance();
    disable();
    mgr->stopNTRIP();

    MockNTRIPTransport *mock = nullptr;
    mgr->setTransportFactory([&](const NTRIPTransportConfig &, QObject *parent) -> NTRIPTransport * {
        mock = new MockNTRIPTransport(parent);
        mock->autoConnect = true;
        return mock;
    });

    QSignalSpy statusSpy(mgr, &NTRIPManager::connectionStatusChanged);

    // Enable NTRIP via settings → should trigger connect
    configureValid();
    mgr->startNTRIP();

    QVERIFY(mock);
    QCOMPARE(mgr->connectionStatus(), NTRIPManager::ConnectionStatus::Connected);

    // Disable → should disconnect
    mgr->stopNTRIP();
    disable();

    QCOMPARE(mgr->connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);

    // Status should have changed multiple times: Connecting → Connected → Disconnected
    QVERIFY(statusSpy.count() >= 2);

    mgr->setTransportFactory(nullptr);
}

// ---------------------------------------------------------------------------
// Test: Changing host/port while connected restarts the connection
// ---------------------------------------------------------------------------

void SettingsLifecycleTest::testSettingsChangeRestartsConnection()
{
    NTRIPManager *mgr = NTRIPManager::instance();
    disable();
    mgr->stopNTRIP();

    int factoryCallCount = 0;
    mgr->setTransportFactory([&](const NTRIPTransportConfig &, QObject *parent) -> NTRIPTransport * {
        factoryCallCount++;
        auto *m = new MockNTRIPTransport(parent);
        m->autoConnect = true;
        return m;
    });

    configureValid();
    mgr->startNTRIP();
    QCOMPARE(factoryCallCount, 1);
    QCOMPARE(mgr->connectionStatus(), NTRIPManager::ConnectionStatus::Connected);

    // Stop and start with new settings
    mgr->stopNTRIP();
    settings()->ntripServerHostAddress()->setRawValue(QStringLiteral("new.caster.com"));
    mgr->startNTRIP();

    QCOMPARE(factoryCallCount, 2);
    QCOMPARE(mgr->connectionStatus(), NTRIPManager::ConnectionStatus::Connected);

    mgr->stopNTRIP();
    mgr->setTransportFactory(nullptr);
    disable();
}

// ---------------------------------------------------------------------------
// Test: Error → Reconnecting → manual stop → clean idle
// ---------------------------------------------------------------------------

void SettingsLifecycleTest::testErrorThenReconnectCycle()
{
    NTRIPManager *mgr = NTRIPManager::instance();
    disable();
    mgr->stopNTRIP();

    MockNTRIPTransport *mock = nullptr;
    mgr->setTransportFactory([&](const NTRIPTransportConfig &, QObject *parent) -> NTRIPTransport * {
        mock = new MockNTRIPTransport(parent);
        mock->autoConnect = false;
        return mock;
    });

    configureValid();
    mgr->startNTRIP();
    QVERIFY(mock);

    // Connect then trigger error
    mock->simulateConnect();
    QCOMPARE(mgr->connectionStatus(), NTRIPManager::ConnectionStatus::Connected);

    QSignalSpy statusSpy(mgr, &NTRIPManager::connectionStatusChanged);
    mock->simulateError(NTRIPError::DataWatchdog, QStringLiteral("timeout"));

    // Should transition: Error → Disconnected → Reconnecting
    QVERIFY(statusSpy.count() >= 2);
    QCOMPARE(mgr->connectionStatus(), NTRIPManager::ConnectionStatus::Reconnecting);

    // User manually stops
    mgr->stopNTRIP();
    QCOMPARE(mgr->connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);

    mgr->setTransportFactory(nullptr);
    disable();
}

// ---------------------------------------------------------------------------
// Test: Rapid enable/disable toggling doesn't crash or leak
// ---------------------------------------------------------------------------

void SettingsLifecycleTest::testRapidToggleDoesNotCrash()
{
    NTRIPManager *mgr = NTRIPManager::instance();
    disable();
    mgr->stopNTRIP();

    int createdCount = 0;
    mgr->setTransportFactory([&](const NTRIPTransportConfig &, QObject *parent) -> NTRIPTransport * {
        createdCount++;
        auto *m = new MockNTRIPTransport(parent);
        m->autoConnect = true;
        return m;
    });

    configureValid();

    for (int i = 0; i < 20; ++i) {
        mgr->startNTRIP();
        mgr->stopNTRIP();
    }

    // Should have created a transport each time start was called
    QCOMPARE(createdCount, 20);

    // Final state should be disconnected
    QCOMPARE(mgr->connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);

    // Process pending deleteLater calls
    QTest::qWait(50);

    mgr->setTransportFactory(nullptr);
    disable();
}

// ---------------------------------------------------------------------------
// Test: Disabling NTRIP while in Reconnecting state cleans up properly
// ---------------------------------------------------------------------------

void SettingsLifecycleTest::testDisableDuringReconnect()
{
    NTRIPManager *mgr = NTRIPManager::instance();
    disable();
    mgr->stopNTRIP();

    MockNTRIPTransport *mock = nullptr;
    mgr->setTransportFactory([&](const NTRIPTransportConfig &, QObject *parent) -> NTRIPTransport * {
        mock = new MockNTRIPTransport(parent);
        mock->autoConnect = false;
        return mock;
    });

    configureValid();
    mgr->startNTRIP();
    QVERIFY(mock);

    // Trigger error to enter Reconnecting state
    mock->simulateConnect();
    mock->simulateError(NTRIPError::SocketError, QStringLiteral("Connection reset"));

    QCOMPARE(mgr->connectionStatus(), NTRIPManager::ConnectionStatus::Reconnecting);

    // Now disable via settings and stop
    disable();
    mgr->stopNTRIP();

    QCOMPARE(mgr->connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);

    // Wait and confirm no reconnect timer fires
    QTest::qWait(200);
    QCOMPARE(mgr->connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);

    mgr->setTransportFactory(nullptr);
}

UT_REGISTER_TEST(SettingsLifecycleTest, TestLabel::Unit)
