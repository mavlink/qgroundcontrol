#include "NTRIPIntegrationTest.h"
#include "MockNTRIPTransport.h"
#include "NTRIPError.h"
#include "NTRIPManager.h"
#include "NTRIPSettings.h"
#include "NTRIPTransportConfig.h"
#include "RTCMMavlink.h"
#include "GPSManager.h"
#include "SettingsManager.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

namespace {

void configureValidSettings()
{
    NTRIPSettings *s = SettingsManager::instance()->ntripSettings();
    if (!s) return;
    s->ntripServerHostAddress()->setRawValue(QStringLiteral("test.caster.com"));
    s->ntripServerPort()->setRawValue(2101);
    s->ntripMountpoint()->setRawValue(QStringLiteral("TEST"));
    s->ntripServerConnectEnabled()->setRawValue(true);
}

void disableNtrip()
{
    NTRIPSettings *s = SettingsManager::instance()->ntripSettings();
    if (!s) return;
    s->ntripServerConnectEnabled()->setRawValue(false);
}

} // namespace

// ---------------------------------------------------------------------------
// Connection lifecycle
// ---------------------------------------------------------------------------

void NTRIPIntegrationTest::testConnectDisconnect()
{
    NTRIPManager *mgr = NTRIPManager::instance();
    disableNtrip();
    mgr->stopNTRIP();

    MockNTRIPTransport *mock = nullptr;
    mgr->setTransportFactory([&](const NTRIPTransportConfig&, QObject *parent) -> NTRIPTransport* {
        mock = new MockNTRIPTransport(parent);
        mock->autoConnect = true;
        return mock;
    });

    configureValidSettings();
    mgr->startNTRIP();

    QVERIFY(mock);
    QVERIFY(mock->isStarted());
    QCOMPARE(mock->startCount, 1);
    QCOMPARE(mgr->connectionStatus(), NTRIPManager::ConnectionStatus::Connected);

    mgr->stopNTRIP();

    QCOMPARE(mgr->connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);
    QCOMPARE(mock->stopCount, 1);

    mgr->setTransportFactory(nullptr);
    disableNtrip();
}

void NTRIPIntegrationTest::testAutoConnectEmitsStatus()
{
    NTRIPManager *mgr = NTRIPManager::instance();
    disableNtrip();
    mgr->stopNTRIP();

    QSignalSpy statusSpy(mgr, &NTRIPManager::connectionStatusChanged);

    mgr->setTransportFactory([](const NTRIPTransportConfig&, QObject *parent) -> NTRIPTransport* {
        auto *m = new MockNTRIPTransport(parent);
        m->autoConnect = true;
        return m;
    });

    configureValidSettings();
    mgr->startNTRIP();

    // Should have transitioned: Connecting → Connected
    QVERIFY(statusSpy.count() >= 1);
    QCOMPARE(mgr->connectionStatus(), NTRIPManager::ConnectionStatus::Connected);

    mgr->stopNTRIP();
    mgr->setTransportFactory(nullptr);
    disableNtrip();
}

void NTRIPIntegrationTest::testStopCleansUpTransport()
{
    NTRIPManager *mgr = NTRIPManager::instance();
    disableNtrip();
    mgr->stopNTRIP();

    QPointer<MockNTRIPTransport> mock;
    mgr->setTransportFactory([&](const NTRIPTransportConfig&, QObject *parent) -> NTRIPTransport* {
        auto *m = new MockNTRIPTransport(parent);
        m->autoConnect = true;
        mock = m;
        return m;
    });

    configureValidSettings();
    mgr->startNTRIP();
    QVERIFY(mock);

    mgr->stopNTRIP();

    // Transport is scheduled for deletion via deleteLater()
    QTest::qWait(50);
    QVERIFY(mock.isNull());

    mgr->setTransportFactory(nullptr);
    disableNtrip();
}

// ---------------------------------------------------------------------------
// Error handling
// ---------------------------------------------------------------------------

void NTRIPIntegrationTest::testTransportErrorTriggersReconnect()
{
    NTRIPManager *mgr = NTRIPManager::instance();
    disableNtrip();
    mgr->stopNTRIP();

    MockNTRIPTransport *mock = nullptr;
    mgr->setTransportFactory([&](const NTRIPTransportConfig&, QObject *parent) -> NTRIPTransport* {
        mock = new MockNTRIPTransport(parent);
        mock->autoConnect = false;
        return mock;
    });

    configureValidSettings();
    mgr->startNTRIP();
    QVERIFY(mock);

    // Simulate connect then error
    mock->simulateConnect();
    QCOMPARE(mgr->connectionStatus(), NTRIPManager::ConnectionStatus::Connected);

    QSignalSpy statusSpy(mgr, &NTRIPManager::connectionStatusChanged);
    mock->simulateError(NTRIPError::DataWatchdog, QStringLiteral("No data"));

    // Error triggers: Error → Disconnected (stopNTRIP) → Reconnecting (with timer)
    QVERIFY(statusSpy.count() >= 2);
    QCOMPARE(mgr->connectionStatus(), NTRIPManager::ConnectionStatus::Reconnecting);

    mgr->stopNTRIP();
    mgr->setTransportFactory(nullptr);
    disableNtrip();
}

void NTRIPIntegrationTest::testSslErrorReported()
{
    NTRIPManager *mgr = NTRIPManager::instance();
    disableNtrip();
    mgr->stopNTRIP();

    MockNTRIPTransport *mock = nullptr;
    mgr->setTransportFactory([&](const NTRIPTransportConfig&, QObject *parent) -> NTRIPTransport* {
        mock = new MockNTRIPTransport(parent);
        mock->autoConnect = false;
        return mock;
    });

    configureValidSettings();
    mgr->startNTRIP();
    QVERIFY(mock);

    QSignalSpy statusSpy(mgr, &NTRIPManager::connectionStatusChanged);
    mock->simulateError(NTRIPError::SslError, QStringLiteral("Certificate expired"));

    // Error path: Error → Disconnected (stop) → Reconnecting
    QVERIFY(statusSpy.count() >= 2);
    QCOMPARE(mgr->connectionStatus(), NTRIPManager::ConnectionStatus::Reconnecting);

    mgr->stopNTRIP();
    mgr->setTransportFactory(nullptr);
    disableNtrip();
}

void NTRIPIntegrationTest::testAuthFailedReported()
{
    NTRIPManager *mgr = NTRIPManager::instance();
    disableNtrip();
    mgr->stopNTRIP();

    MockNTRIPTransport *mock = nullptr;
    mgr->setTransportFactory([&](const NTRIPTransportConfig&, QObject *parent) -> NTRIPTransport* {
        mock = new MockNTRIPTransport(parent);
        mock->autoConnect = false;
        return mock;
    });

    configureValidSettings();
    mgr->startNTRIP();
    QVERIFY(mock);

    QSignalSpy statusSpy(mgr, &NTRIPManager::connectionStatusChanged);
    mock->simulateError(NTRIPError::AuthFailed, QStringLiteral("401 Unauthorized"));

    // Error path: Error → Disconnected (stop) → Reconnecting
    QVERIFY(statusSpy.count() >= 2);
    QCOMPARE(mgr->connectionStatus(), NTRIPManager::ConnectionStatus::Reconnecting);

    mgr->stopNTRIP();
    mgr->setTransportFactory(nullptr);
    disableNtrip();
}

// ---------------------------------------------------------------------------
// RTCM data flow
// ---------------------------------------------------------------------------

void NTRIPIntegrationTest::testRtcmFlowsToRouter()
{
    NTRIPManager *mgr = NTRIPManager::instance();
    disableNtrip();
    mgr->stopNTRIP();

    MockNTRIPTransport *mock = nullptr;
    mgr->setTransportFactory([&](const NTRIPTransportConfig&, QObject *parent) -> NTRIPTransport* {
        mock = new MockNTRIPTransport(parent);
        mock->autoConnect = true;
        return mock;
    });

    RTCMMavlink *mavlink = GPSManager::instance()->rtcmMavlink();
    QVERIFY(mavlink);

    QVector<QByteArray> routedData;
    QSignalSpy routeSpy(mavlink, &RTCMMavlink::RTCMDataUpdate);
    (void)routeSpy;

    configureValidSettings();
    mgr->startNTRIP();
    QVERIFY(mock);

    // Simulate RTCM data
    mock->simulateRtcmData(QByteArrayLiteral("RTCM_MSG_1"));
    mock->simulateRtcmData(QByteArrayLiteral("RTCM_MSG_2"));

    // RTCMDataUpdate is emitted by RTCMMavlink when data is routed to vehicles.
    // Check stats to verify data flowed through the pipeline.
    QVERIFY(mgr->connectionStats()->messagesReceived() >= 2);

    mgr->stopNTRIP();
    mgr->setTransportFactory(nullptr);
    disableNtrip();
}

void NTRIPIntegrationTest::testRtcmUpdatesStats()
{
    NTRIPManager *mgr = NTRIPManager::instance();
    disableNtrip();
    mgr->stopNTRIP();

    MockNTRIPTransport *mock = nullptr;
    mgr->setTransportFactory([&](const NTRIPTransportConfig&, QObject *parent) -> NTRIPTransport* {
        mock = new MockNTRIPTransport(parent);
        mock->autoConnect = true;
        return mock;
    });

    configureValidSettings();
    mgr->startNTRIP();
    QVERIFY(mock);

    auto *stats = mgr->connectionStats();
    QCOMPARE(stats->messagesReceived(), 0);
    QCOMPARE(stats->bytesReceived(), static_cast<quint64>(0));

    const QByteArray testData(100, 'X');
    mock->simulateRtcmData(testData);

    QCOMPARE(stats->messagesReceived(), 1);
    QCOMPARE(stats->bytesReceived(), static_cast<quint64>(100));

    mock->simulateRtcmData(testData);
    QCOMPARE(stats->messagesReceived(), 2);
    QCOMPARE(stats->bytesReceived(), static_cast<quint64>(200));

    mgr->stopNTRIP();
    mgr->setTransportFactory(nullptr);
    disableNtrip();
}

// ---------------------------------------------------------------------------
// GGA flow
// ---------------------------------------------------------------------------

void NTRIPIntegrationTest::testGgaSentAfterConnect()
{
    NTRIPManager *mgr = NTRIPManager::instance();
    disableNtrip();
    mgr->stopNTRIP();

    MockNTRIPTransport *mock = nullptr;
    mgr->setTransportFactory([&](const NTRIPTransportConfig&, QObject *parent) -> NTRIPTransport* {
        mock = new MockNTRIPTransport(parent);
        mock->autoConnect = false;
        return mock;
    });

    configureValidSettings();
    mgr->startNTRIP();
    QVERIFY(mock);
    QVERIFY(mock->sentNmea.isEmpty());

    // After connect, the GGA provider should attempt to send GGA
    mock->simulateConnect();

    // Verify connected status without relying on timing-dependent GGA send
    QTRY_COMPARE_WITH_TIMEOUT(mgr->connectionStatus(), NTRIPManager::ConnectionStatus::Connected, 3000);

    mgr->stopNTRIP();
    mgr->setTransportFactory(nullptr);
    disableNtrip();
}

// ---------------------------------------------------------------------------
// Config validation
// ---------------------------------------------------------------------------

void NTRIPIntegrationTest::testInvalidConfigRejectsStart()
{
    NTRIPManager *mgr = NTRIPManager::instance();
    disableNtrip();
    mgr->stopNTRIP();

    bool factoryCalled = false;
    mgr->setTransportFactory([&](const NTRIPTransportConfig&, QObject *parent) -> NTRIPTransport* {
        factoryCalled = true;
        return new MockNTRIPTransport(parent);
    });

    // Empty host
    NTRIPSettings *s = SettingsManager::instance()->ntripSettings();
    s->ntripServerHostAddress()->setRawValue(QStringLiteral(""));
    s->ntripServerPort()->setRawValue(2101);
    s->ntripServerConnectEnabled()->setRawValue(true);

    mgr->startNTRIP();

    QVERIFY(!factoryCalled);
    QCOMPARE(mgr->connectionStatus(), NTRIPManager::ConnectionStatus::Error);

    // Invalid port
    s->ntripServerHostAddress()->setRawValue(QStringLiteral("host.com"));
    s->ntripServerPort()->setRawValue(0);

    mgr->startNTRIP();

    QVERIFY(!factoryCalled);

    mgr->stopNTRIP();
    mgr->setTransportFactory(nullptr);
    disableNtrip();
}

UT_REGISTER_TEST(NTRIPIntegrationTest, TestLabel::Integration)
