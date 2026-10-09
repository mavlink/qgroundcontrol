#include "NTRIPManagerTest.h"

#include <limits>
#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>
#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "GPSCorrectionManager.h"
#include "ManualScheduler.h"
#include "NTRIP/Support/MockNTRIPTransport.h"
#include "NTRIP/Support/NTRIPTestHelpers.h"
#include "NTRIP/Support/ScriptedNTRIPCaster.h"
#include "NTRIPManager.h"
#include "Protocols/Support/ProtocolTestPackets.h"
#include "QGCNetworkAvailabilityMonitor.h"
#include "RTCMFramer.h"
#include "Support/GPSTestHelpers.h"

using namespace GPSTest;

namespace {
constexpr std::chrono::milliseconds SettingsDebounce{250};

void initialize(NTRIPManager& manager, const NTRIPManager::Configuration& configuration = mockCasterConfiguration())
{
    manager.setConfiguration(configuration);
    manager.init();
}

class FakeNetworkMonitor : public QGCNetworkAvailabilityMonitor
{
public:
    explicit FakeNetworkMonitor(bool hasNetwork)
        : QGCNetworkAvailabilityMonitor(hasNetwork, nullptr)
    {}

    void setHasNetwork(bool hasNetwork) { _setAvailable(hasNetwork); }
};
}  // namespace

void NTRIPManagerTest::_fail(MockNTRIPTransport* transport, const QString& detail, NTRIPError code,
                             std::chrono::milliseconds retryAfter)
{
    expectLogMessage("GPS.NTRIPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("NTRIP error:.*") + QRegularExpression::escape(detail)));
    transport->simulateError(code, detail, retryAfter);
    deliverQueuedCalls();
    verifyExpectedLogMessage();
}

void NTRIPManagerTest::_stopFromIdleIsNoop()
{
    NTRIPManager manager;
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);
    QCOMPARE(manager.connectionStatusText(), NTRIPManager::tr("Disconnected"));
    QCOMPARE(manager.property("connectionStatusText").toString(), NTRIPManager::tr("Disconnected"));
    manager._stop();
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);
}

void NTRIPManagerTest::_transportFactory()
{
    NTRIPManager manager;
    QList<QObject*> parents;
    QString requestedHost;
    MockNTRIPTransport* transport = nullptr;
    manager.setTransportFactory([&](const NTRIPManager::Configuration& configuration, QObject* parent) {
        parents.append(parent);
        requestedHost = configuration.connection.host;
        transport = new MockNTRIPTransport(parent);
        return transport;
    });
    initialize(manager);
    QCOMPARE(parents.size(), 1);
    QCOMPARE(parents.first(), &manager);
    QCOMPARE(requestedHost, mockCasterConfiguration().connection.host);
    QVERIFY(transport);
    QCOMPARE(transport->startCount, 1);
}

void NTRIPManagerTest::_stopCancelsDeferredSettings_data()
{
    QTest::addColumn<bool>("shutdown");
    QTest::newRow("restartable-stop") << false;
    QTest::newRow("permanent-shutdown") << true;
}

void NTRIPManagerTest::_stopCancelsDeferredSettings()
{
    QFETCH(bool, shutdown);
    ManualScheduler scheduler;
    NTRIPManager manager(nullptr, &scheduler);
    auto* first = injectMockTransport(manager, true);
    initialize(manager);
    QCOMPARE(first->startCount, 1);
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Connected);
    auto changed = manager.configuration();
    changed.connection.mountpoint = QStringLiteral("CHANGED");
    manager.setConfiguration(changed);
    auto* replacement = injectMockTransport(manager, true);
    if (shutdown) {
        manager.shutdown();
    } else {
        manager._stop();
    }
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);
    QVERIFY(scheduler.advanceBy(SettingsDebounce));
    QCOMPARE(replacement->startCount, 0);
    if (shutdown) {
        changed.connection.mountpoint = QStringLiteral("AFTER_SHUTDOWN");
        manager.setConfiguration(changed);
        QVERIFY(scheduler.advanceBy(SettingsDebounce));
        QCOMPARE(replacement->startCount, 0);
    }
    manager._start();
    QCOMPARE(replacement->startCount, shutdown ? 0 : 1);
}

void NTRIPManagerTest::_newSessionRetryBudget_data()
{
    QTest::addColumn<int>("action");
    QTest::newRow("stop-start") << 0;
    QTest::newRow("disable-enable") << 1;
    QTest::newRow("automatic-reconnect") << 2;
    QTest::newRow("qml-retry-enables-connection") << 3;
}

void NTRIPManagerTest::_newSessionRetryBudget()
{
    QFETCH(int, action);
    ManualScheduler scheduler;
    NTRIPManager manager(nullptr, &scheduler);
    auto* transport = injectMockTransport(manager);
    manager.setConfiguration(mockCasterConfiguration());
    manager.init();

    const auto failWithExpectedDelay = [&](std::chrono::milliseconds expectedDelay) {
        _fail(transport, QStringLiteral("retry budget"));
        QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Reconnecting);
        QVERIFY(manager.statusMessage().contains(QStringLiteral("in %1s").arg(expectedDelay.count() / 1000)));
        transport = injectMockTransport(manager);
        if (expectedDelay > std::chrono::milliseconds(1)) {
            QVERIFY(scheduler.advanceBy(expectedDelay - std::chrono::milliseconds(1)));
            QCOMPARE(transport->startCount, 0);
        }
        QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
        QCOMPARE(transport->startCount, 1);
        QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Connecting);
    };

    // Spend part of the budget; only an automatic reconnect keeps backing off from here.
    failWithExpectedDelay(std::chrono::seconds(1));
    failWithExpectedDelay(std::chrono::seconds(2));
    if (action == 0) {
        manager._stop();
        transport = injectMockTransport(manager);
        manager._start();
    } else if (action == 1) {
        manager.setConfiguration(mockCasterConfiguration(false));
        QVERIFY(scheduler.advanceBy(SettingsDebounce));
        transport = injectMockTransport(manager);
        manager.setConfiguration(mockCasterConfiguration(true));
        QVERIFY(scheduler.advanceBy(SettingsDebounce));
    } else if (action == 2) {
        failWithExpectedDelay(std::chrono::seconds(4));
    } else {
        _fail(transport, QStringLiteral("authentication"), NTRIPError::AuthFailed);
        QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Error);
        manager.setConfiguration(mockCasterConfiguration(false));
        transport = injectMockTransport(manager);
        QVERIFY(manager.metaObject()->indexOfMethod("retryNTRIP()") >= 0);
        QSignalSpy enableRequested(&manager, &NTRIPManager::enableRequested);
        manager.retryNTRIP();
        QCOMPARE(enableRequested.count(), 1);
    }
    QCOMPARE(transport->startCount, 1);
    QVERIFY(manager.configuration().enabled);
    const auto expectedDelay = action == 2 ? std::chrono::seconds(8) : std::chrono::seconds(1);
    _fail(transport, QStringLiteral("retry budget"));
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Reconnecting);
    QVERIFY(manager.statusMessage().contains(QStringLiteral("in %1s").arg(expectedDelay.count())));
    auto* retry = injectMockTransport(manager);
    QVERIFY(scheduler.advanceBy(expectedDelay - std::chrono::milliseconds(1)));
    QCOMPARE(retry->startCount, 0);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QCOMPARE(retry->startCount, 1);
}

void NTRIPManagerTest::_statusCallbackStopsTransition()
{
    NTRIPManager manager;
    manager.setConfiguration(mockCasterConfiguration());
    auto* transport = injectMockTransport(manager);
    QList<NTRIPManager::ConnectionStatus> observed;
    connect(&manager, &NTRIPManager::connectionStatusChanged, this, [&]() {
        observed.append(manager.connectionStatus());
        if (manager.connectionStatus() == NTRIPManager::ConnectionStatus::Connecting) {
            manager._stop();
        }
    });
    manager._start();
    // Observers see the settled state after the transport has started, never a half-entered state.
    QCOMPARE(observed, (QList<NTRIPManager::ConnectionStatus>{NTRIPManager::ConnectionStatus::Connecting,
                                                              NTRIPManager::ConnectionStatus::Disconnected}));
    QCOMPARE(transport->startCount, 1);
    QCOMPARE(transport->stopCount, 1);
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);
}

void NTRIPManagerTest::_connectedCallbackStopsTransition()
{
    NTRIPManager manager;
    auto* transport = injectMockTransport(manager);
    initialize(manager);
    connect(&manager, &NTRIPManager::connectionStatusChanged, this, [&]() {
        if (manager.connectionStatus() == NTRIPManager::ConnectionStatus::Connected) {
            manager._stop();
        }
    });
    transport->simulateConnect();
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);
    QCOMPARE(transport->stopCount, 1);
    QVERIFY(manager.ggaSource().isEmpty());
}

void NTRIPManagerTest::_plaintextCredentialWarningIsVisibleState_data()
{
    QTest::addColumn<bool>("tls");
    QTest::newRow("plaintext") << false;
    QTest::newRow("tls") << true;
}

void NTRIPManagerTest::_plaintextCredentialWarningIsVisibleState()
{
    QFETCH(bool, tls);
    NTRIPManager mgr;
    auto configuration = mockCasterConfiguration();
    configuration.connection.username = QStringLiteral("user");
    configuration.connection.useTls = tls;
    auto* transport = injectMockTransport(mgr);
    initialize(mgr, configuration);
    QSignalSpy warningSpy(&mgr, &NTRIPManager::securityWarningChanged);

    // Only a stream that is sending its credentials in the clear warns.
    QVERIFY(mgr.securityWarning().isEmpty());
    transport->simulateConnect();
    QCOMPARE(warningSpy.count(), tls ? 0 : 1);
    QCOMPARE(mgr.securityWarning().contains(QStringLiteral("without TLS")), !tls);

    mgr._stop();
    QCOMPARE(warningSpy.count(), tls ? 0 : 2);
    QVERIFY(mgr.securityWarning().isEmpty());
}

void NTRIPManagerTest::_certificatePinWriteBackKeepsConnection()
{
    ManualScheduler scheduler;
    NTRIPManager manager(nullptr, &scheduler);
    auto configuration = mockCasterConfiguration();
    configuration.connection.useTls = true;
    configuration.connection.allowSelfSignedCerts = true;
    auto* transport = injectMockTransport(manager, true);
    initialize(manager, configuration);
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Connected);
    auto* replacement = injectMockTransport(manager, true);
    QSignalSpy pins(&manager, &NTRIPManager::certificatePinChanged);

    const QString pin = QStringLiteral("caster.example.com:2101|") + QString(64, QLatin1Char('a'));
    emit transport->certificatePinned(pin);
    QCOMPARE(pins.size(), 1);
    QCOMPARE(pins.first().first().toString(), pin);

    // The settings store the pin and apply it back; the connection that trusted the certificate stays up.
    configuration.connection.pinnedCertificate = pin;
    manager.setConfiguration(configuration);
    QVERIFY(scheduler.advanceBy(SettingsDebounce));
    QCOMPARE(pins.size(), 1);
    QCOMPARE(transport->stopCount, 0);
    QCOMPARE(replacement->startCount, 0);
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Connected);
    manager._stop();
}

void NTRIPManagerTest::_optingOutForgetsCertificatePin_data()
{
    QTest::addColumn<bool>("allowSelfSigned");
    QTest::newRow("opted-in-keeps-pin") << true;
    QTest::newRow("opted-out-forgets-pin") << false;
}

void NTRIPManagerTest::_optingOutForgetsCertificatePin()
{
    QFETCH(bool, allowSelfSigned);
    NTRIPManager manager;
    QSignalSpy pins(&manager, &NTRIPManager::certificatePinChanged);
    auto configuration = mockCasterConfiguration(false);
    configuration.connection.allowSelfSignedCerts = allowSelfSigned;
    configuration.connection.pinnedCertificate =
        QStringLiteral("caster.example.com:2101|") + QString(64, QLatin1Char('a'));
    manager.setConfiguration(configuration);
    QCOMPARE(pins.size(), allowSelfSigned ? 0 : 1);
    if (!allowSelfSigned) {
        QVERIFY(pins.first().first().toString().isEmpty());
    }
    QCOMPARE(manager.configuration().connection.pinnedCertificate.isEmpty(), !allowSelfSigned);
}

// ---------------------------------------------------------------------------
// Reconnect backoff (migrated from NTRIPReconnectPolicyTest)
// ---------------------------------------------------------------------------

void NTRIPManagerTest::_reconnectGivesUpAfterAttemptCeiling()
{
    ManualScheduler scheduler;
    NTRIPManager manager(nullptr, &scheduler);
    auto* transport = injectMockTransport(manager);
    initialize(manager);

    constexpr int ATTEMPTS = 100;
    for (int attempt = 1; attempt <= ATTEMPTS; ++attempt) {
        _fail(transport, QStringLiteral("refused"));
        QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Reconnecting);
        transport = injectMockTransport(manager);
        QVERIFY(scheduler.advanceBy(std::chrono::seconds(30)));
        QCOMPARE(transport->startCount, 1);
    }
    _fail(transport, QStringLiteral("last refusal"));
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Error);
    QCOMPARE(manager.statusMessage(),
             QStringLiteral("Gave up after %1 reconnect attempts: last refusal").arg(ATTEMPTS));
}

void NTRIPManagerTest::_reconnectCancelStopsTimer()
{
    ManualScheduler scheduler;
    NTRIPManager mgr(nullptr, &scheduler);
    auto* transport = injectMockTransport(mgr);
    initialize(mgr);
    _fail(transport, QStringLiteral("cancel"));
    auto* retry = injectMockTransport(mgr);
    mgr._stop();
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(1)));
    QCOMPARE(retry->startCount, 0);
}

void NTRIPManagerTest::_reconnectBackoffResetsOnCorrections()
{
    ManualScheduler scheduler;
    NTRIPManager mgr(nullptr, &scheduler);
    auto* transport = injectMockTransport(mgr, true);
    initialize(mgr);
    const auto dropAfterHandshake = [&](int expectedSeconds) {
        QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Connected);
        _fail(transport, QStringLiteral("caster closed"), NTRIPError::ServerDisconnected);
        QVERIFY(mgr.statusMessage().contains(QStringLiteral("%1s").arg(expectedSeconds)));
        transport = injectMockTransport(mgr, true);
        QVERIFY(scheduler.advanceBy(std::chrono::seconds(expectedSeconds)));
        QCOMPARE(transport->startCount, 1);
    };

    // A caster that accepts each request and then drops it keeps backing off.
    for (const int seconds : {1, 2, 4}) {
        dropAfterHandshake(seconds);
    }
    transport->simulateRtcmData(GPSTest::rtcmMessage(1005), 1005, scheduler.nowMs());
    deliverQueuedCalls();
    dropAfterHandshake(1);
}

void NTRIPManagerTest::_reconnectWaitsForNetworkAtFailure()
{
    ManualScheduler scheduler;
    FakeNetworkMonitor network(false);
    NTRIPManager manager(nullptr, &scheduler, {.network = &network});
    auto* transport = injectMockTransport(manager);
    initialize(manager);
    QCOMPARE(transport->startCount, 1);

    constexpr int OfflineFailures = 105;
    for (int i = 0; i < OfflineFailures; ++i) {
        _fail(transport, QStringLiteral("offline"));
        QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Reconnecting);
        QCOMPARE(manager.statusMessage(), QStringLiteral("Waiting for network"));
        auto* blockedRetry = injectMockTransport(manager);
        QVERIFY(scheduler.advanceBy(std::chrono::minutes(5)));
        QCOMPARE(blockedRetry->startCount, 0);
        network.setHasNetwork(true);
        QCOMPARE(blockedRetry->startCount, 1);
        QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Connecting);
        network.setHasNetwork(false);
        transport = blockedRetry;
    }

    network.setHasNetwork(true);
    _fail(transport, QStringLiteral("retry budget"));
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Reconnecting);
    QVERIFY(manager.statusMessage().contains(QStringLiteral("1s")));
    auto* retry = injectMockTransport(manager);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(999)));
    QCOMPARE(retry->startCount, 0);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QCOMPARE(retry->startCount, 1);
}

void NTRIPManagerTest::_reconnectWaitsWhenNetworkLostDuringBackoff()
{
    ManualScheduler scheduler;
    FakeNetworkMonitor network(true);
    NTRIPManager manager(nullptr, &scheduler, {.network = &network});
    auto* transport = injectMockTransport(manager);
    initialize(manager);
    _fail(transport, QStringLiteral("backoff"));
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Reconnecting);
    auto* retry = injectMockTransport(manager);
    network.setHasNetwork(false);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(1)));
    QCOMPARE(retry->startCount, 0);
    QCOMPARE(manager.statusMessage(), QStringLiteral("Waiting for network"));
    network.setHasNetwork(true);
    QCOMPARE(retry->startCount, 1);
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Connecting);
    _fail(retry, QStringLiteral("after wait"));
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Reconnecting);
    QVERIFY(manager.statusMessage().contains(QStringLiteral("1s")));
}

void NTRIPManagerTest::_loopbackCasterBypassesNetworkGate_data()
{
    QTest::addColumn<QString>("host");
    QTest::newRow("localhost") << QStringLiteral("localhost");
    QTest::newRow("ipv4-loopback") << QStringLiteral("127.1.2.3");
    QTest::newRow("ipv6-loopback") << QStringLiteral("::1");
}

void NTRIPManagerTest::_loopbackCasterBypassesNetworkGate()
{
    QFETCH(QString, host);
    ManualScheduler scheduler;
    FakeNetworkMonitor network(false);
    NTRIPManager manager(nullptr, &scheduler, {.network = &network});
    auto configuration = mockCasterConfiguration();
    configuration.connection.host = host;
    auto* transport = injectMockTransport(manager);
    initialize(manager, configuration);
    _fail(transport, QStringLiteral("loopback"));
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Reconnecting);
    QVERIFY(!manager.statusMessage().contains(QStringLiteral("Waiting for network")));
    auto* retry = injectMockTransport(manager);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(1)));
    QCOMPARE(retry->startCount, 1);
}

void NTRIPManagerTest::_waitingStatusObserverCanStopManager()
{
    ManualScheduler scheduler;
    FakeNetworkMonitor network(false);
    NTRIPManager manager(nullptr, &scheduler, {.network = &network});
    auto* transport = injectMockTransport(manager);
    initialize(manager);
    bool stoppedFromStatus = false;
    connect(&manager, &NTRIPManager::statusMessageChanged, this, [&]() {
        if (manager.statusMessage() == QStringLiteral("Waiting for network")) {
            stoppedFromStatus = true;
            manager._stop();
        }
    });

    _fail(transport, QStringLiteral("observer"));
    QVERIFY(stoppedFromStatus);
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);
    auto* retry = injectMockTransport(manager);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(1)));
    QCOMPARE(retry->startCount, 0);
}

void NTRIPManagerTest::_duplicateTransportErrorsScheduleOneRetry()
{
    ManualScheduler scheduler;
    NTRIPManager mgr(nullptr, &scheduler);
    auto* transport = injectMockTransport(mgr);
    initialize(mgr);
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Connecting);
    expectLogMessage("GPS.NTRIPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("NTRIP error:.*first failure")));
    transport->simulateError(NTRIPError::SocketError, QStringLiteral("first failure"));
    transport->simulateError(NTRIPError::ServerDisconnected, QStringLiteral("duplicate failure"));
    deliverQueuedCalls();
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Reconnecting);
    QVERIFY(mgr.statusMessage().contains(QStringLiteral("first failure")));
    QVERIFY(!mgr.statusMessage().contains(QStringLiteral("duplicate failure")));
    verifyExpectedLogMessage();
    auto* retry = injectMockTransport(mgr);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(1)));
    QCOMPARE(retry->startCount, 1);
}

void NTRIPManagerTest::_retiredTransportErrorCannotAffectNewSession()
{
    ManualScheduler scheduler;
    NTRIPManager mgr(nullptr, &scheduler);
    auto* first = injectMockTransport(mgr);
    initialize(mgr);
    first->simulateError(NTRIPError::HttpError, QStringLiteral("retired failure"), std::chrono::seconds{300});
    mgr._stop();
    auto* second = injectMockTransport(mgr);
    mgr._start();
    deliverQueuedCalls();
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Connecting);
    QCOMPARE(second->startCount, 1);
    auto* unexpected = injectMockTransport(mgr);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(300)));
    QCOMPARE(unexpected->startCount, 0);
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Connecting);
}

void NTRIPManagerTest::_retryPolicy_data()
{
    QTest::addColumn<qint64>("retryAfterMs");
    QTest::addColumn<int>("attempts");
    QTest::addColumn<bool>("enabled");
    QTest::addColumn<NTRIPError>("code");
    QTest::addColumn<int>("expectedDelayMs");
    QTest::newRow("initial-backoff") << qint64(0) << 0 << true << NTRIPError::HttpError << 1000;
    QTest::newRow("exponential-backoff") << qint64(0) << 4 << true << NTRIPError::HttpError << 16000;
    QTest::newRow("backoff-cap") << qint64(0) << 6 << true << NTRIPError::HttpError << 30000;
    QTest::newRow("hint") << qint64(17000) << 0 << true << NTRIPError::HttpError << 17000;
    QTest::newRow("backoff-dominates") << qint64(1000) << 4 << true << NTRIPError::HttpError << 16000;
    QTest::newRow("negative-hint") << qint64(-1) << 0 << true << NTRIPError::HttpError << 1000;
    QTest::newRow("cap") << std::numeric_limits<qint64>::max() << 0 << true << NTRIPError::HttpError << 300000;
    QTest::newRow("disabled") << qint64(0) << 0 << false << NTRIPError::HttpError << 0;
    QTest::newRow("disabled-with-hint") << qint64(17000) << 0 << false << NTRIPError::HttpError << 0;
    QTest::newRow("authentication") << qint64(0) << 0 << true << NTRIPError::AuthFailed << 0;
    QTest::newRow("authentication-with-hint") << qint64(17000) << 0 << true << NTRIPError::AuthFailed << 0;
    QTest::newRow("invalid-config") << qint64(0) << 0 << true << NTRIPError::InvalidConfig << 0;
    QTest::newRow("invalid-config-with-hint") << qint64(17000) << 0 << true << NTRIPError::InvalidConfig << 0;
    QTest::newRow("tls-certificate") << qint64(0) << 0 << true << NTRIPError::SslError << 0;
    QTest::newRow("request-rejected") << qint64(17000) << 0 << true << NTRIPError::RequestRejected << 0;
}

void NTRIPManagerTest::_retryPolicy()
{
    QFETCH(qint64, retryAfterMs);
    QFETCH(int, attempts);
    QFETCH(bool, enabled);
    QFETCH(NTRIPError, code);
    QFETCH(int, expectedDelayMs);
    ManualScheduler scheduler;
    NTRIPManager manager(nullptr, &scheduler);
    manager.setConfiguration(mockCasterConfiguration(enabled));
    auto* transport = injectMockTransport(manager);
    manager._start();

    for (int attempt = 0; attempt < attempts; ++attempt) {
        const auto warmupDelay = std::chrono::milliseconds(1000 * (1 << attempt));
        _fail(transport, QStringLiteral("warmup"));
        transport = injectMockTransport(manager);
        QVERIFY(scheduler.advanceBy(warmupDelay));
        QCOMPARE(transport->startCount, 1);
    }

    _fail(transport, QStringLiteral("retry test"), code, std::chrono::milliseconds{retryAfterMs});
    QCOMPARE(manager.connectionStatus(),
             expectedDelayMs ? NTRIPManager::ConnectionStatus::Reconnecting : NTRIPManager::ConnectionStatus::Error);
    if (expectedDelayMs) {
        QVERIFY(manager.statusMessage().contains(QString::number(expectedDelayMs / 1000)));
        auto* retry = injectMockTransport(manager);
        QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(expectedDelayMs) - std::chrono::milliseconds(1)));
        QCOMPARE(retry->startCount, 0);
        QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
        QCOMPARE(retry->startCount, 1);
    }
    manager._stop();
    manager.setConfiguration(mockCasterConfiguration(true));
    auto* replacement = injectMockTransport(manager, true);
    manager._start();
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Connected);
    _fail(replacement, QStringLiteral("fresh failure"));
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Reconnecting);
    QVERIFY(manager.statusMessage().contains(QStringLiteral("1s")));
}

void NTRIPManagerTest::_httpRetryAfterReachesManager()
{
    // Compressed error bodies and authentication failures are NTRIPHttpCodecTest's and _retryPolicy's.
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    ManualScheduler scheduler;
    NTRIPManager manager(nullptr, &scheduler);
    auto configuration = mockCasterConfiguration();
    configuration.connection = caster.connectionConfig();
    manager.setConfiguration(configuration);
    manager._start();
    auto* connection = caster.waitForConnection(TestTimeout::shortMs());
    QVERIFY(connection && connection->peer);
    QVERIFY(connection->waitForRequest(TestTimeout::shortMs()).startsWith("GET /TEST HTTP/1.1"));
    expectLogMessage("GPS.NTRIPManager", QtWarningMsg, QRegularExpression(QStringLiteral("NTRIP error:.*503")));
    const QByteArray response = "HTTP/1.1 503 Error\r\nRetry-After: 17\r\nContent-Length: 0\r\n\r\n";
    QCOMPARE(connection->write(response), response.size());
    QTRY_COMPARE_WITH_TIMEOUT(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Reconnecting,
                              TestTimeout::shortMs());
    verifyExpectedLogMessage();
    auto* retry = injectMockTransport(manager);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(17) - std::chrono::milliseconds(1)));
    QCOMPARE(retry->startCount, 0);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QCOMPARE(retry->startCount, 1);
}

void NTRIPManagerTest::_retryPublicationSuperseded()
{
    ManualScheduler scheduler;
    NTRIPManager manager(nullptr, &scheduler);
    manager.setConfiguration(mockCasterConfiguration());
    auto* first = injectMockTransport(manager, true);
    manager._start();
    auto* replacement = injectMockTransport(manager);
    bool replaced = false;
    // An observer of the Reconnecting status restarts the manager; the retry already scheduled must not fire.
    connect(&manager, &NTRIPManager::connectionStatusChanged, this, [&]() {
        if (manager.connectionStatus() != NTRIPManager::ConnectionStatus::Reconnecting ||
            std::exchange(replaced, true)) {
            return;
        }
        manager._stop();
        injectNextTransport(manager, replacement);
        manager._start();
    });
    _fail(first, QStringLiteral("superseded"), NTRIPError::HttpError, std::chrono::seconds(300));
    QVERIFY(replaced);
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Connecting);
    QCOMPARE(replacement->startCount, 1);
    auto* unexpected = injectMockTransport(manager);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(300)));
    QCOMPARE(unexpected->startCount, 0);
}

void NTRIPManagerTest::_missingMountpointDoesNotStartTransport()
{
    NTRIPManager mgr;
    auto configuration = mockCasterConfiguration();
    configuration.connection.mountpoint.clear();
    mgr.setConfiguration(configuration);
    auto* transport = injectMockTransport(mgr, true);
    expectLogMessage("GPS.NTRIPManager", QtWarningMsg, QRegularExpression(QStringLiteral("Select a mountpoint")));
    mgr._start();
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Error);
    QCOMPARE(transport->startCount, 0);
    QVERIFY(mgr.statusMessage().contains(QStringLiteral("mountpoint")));
    verifyExpectedLogMessage();
}

void NTRIPManagerTest::_correctionIngressKeepsSessionAndIdentity()
{
    GPSCorrectionManager corrections;
    QList<QByteArray> routed;
    captureVehicleFrames(corrections, routed);
    NTRIPManager mgr(nullptr, nullptr, {.corrections = &corrections});
    auto configuration = mockCasterConfiguration(false);
    configuration.connection.username = QStringLiteral("private-user");
    configuration.connection.password = QStringLiteral("private-password");
    configuration.connection.useTls = true;
    mgr.setConfiguration(configuration);
    QCOMPARE(mgr.metaObject()->indexOfProperty("rtcmMavlink"), -1);
    auto* first = injectMockTransport(mgr);
    QSignalSpy observed(first, &NTRIPTransport::correctionFrameReceived);
    mgr.init();
    QCOMPARE(first->startCount, 0);
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);
    configuration.enabled = true;
    mgr.setConfiguration(configuration);
    QTRY_COMPARE_WITH_TIMEOUT(first->startCount, 1, TestTimeout::shortMs());
    const QByteArray frame = GPSTest::rtcmMessage(1005);
    const qint64 receivedAtMs = GPSTest::nowMs() - 10;
    first->simulateRtcmData(frame, 1005, receivedAtMs);
    QCOMPARE(observed.size(), 1);
    const auto result = qvariant_cast<RTCMDecodedFrame>(observed[0][0]);
    QCOMPARE(result.data, frame);
    QCOMPARE(result.messageId, 1005);
    QCOMPARE(result.receivedAtMs, receivedAtMs);
    QVERIFY(result.valid && !result.filtered);
    QVERIFY(routed.isEmpty());
    deliverQueuedCalls();
    QCOMPARE(routed.size(), 1);
    QCOMPARE(routed[0], frame);
    QCOMPARE(corrections.vehicleBytesSubmitted(), quint64(frame.size()));
    // The stream is named by the caster endpoint, without credentials.
    QTRY_COMPARE_WITH_TIMEOUT(corrections.selectedStream().instanceId,
                              QStringLiteral("ntrips://caster.example.com:2101/TEST"), TestTimeout::shortMs());
    QCOMPARE(corrections.selectedStream().source, static_cast<int>(GPSCorrectionSettings::Ntrip));

    first->simulateRtcmData(frame, 1005);
    mgr._stop();
    auto* second = injectMockTransport(mgr);
    mgr._start();
    deliverQueuedCalls();
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Connecting);
    QCOMPARE(routed.size(), 1);
    QCOMPARE(corrections.vehicleBytesSubmitted(), quint64(frame.size()));
    const qint64 expiredAgeMs = GPSCorrectionSelector::FRESHNESS_TIMEOUT.count() + 6000;
    const qint64 expiredAtMs = GPSTest::nowMs() - expiredAgeMs;
    second->simulateRtcmData(frame, 1005, expiredAtMs);
    QCOMPARE(mgr.connectionStats()->messagesReceived(), quint32(0));
    deliverQueuedCalls();
    QCOMPARE(routed.size(), 1);
    QCOMPARE(corrections.vehicleBytesSubmitted(), quint64(frame.size()));
    QCOMPARE(mgr.connectionStats()->messagesReceived(), quint32(1));
    QCOMPARE(mgr.connectionStats()->bytesReceived(), quint64(frame.size()));
    QVERIFY(mgr.connectionStats()->correctionAgeSec() >= expiredAgeMs / 1000.0);
    QVERIFY(mgr.connectionStats()->dataStale());

    // How older frames age the statistics is NTRIPConnectionStatsTest's.
    second->simulateRtcmData(frame, 1005);
    deliverQueuedCalls();
    QCOMPARE(routed.size(), 2);
    QCOMPARE(routed[1], frame);
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Connected);
    QCOMPARE(corrections.vehicleBytesSubmitted(), quint64(2 * frame.size()));
    QCOMPARE(mgr.connectionStats()->messagesReceived(), quint32(2));
    QVERIFY(mgr.connectionStats()->correctionAgeSec() < 1.0);
    QVERIFY(!mgr.connectionStats()->dataStale());
}

void NTRIPManagerTest::_factChangesReconfigureTransport_data()
{
    QTest::addColumn<QString>("setting");
    QTest::newRow("cold-whitelist") << QStringLiteral("whitelist");
    QTest::newRow("hot-mountpoint") << QStringLiteral("mountpoint");
}

void NTRIPManagerTest::_ggaSettingsUseInjectedProviders()
{
    using Source = NTRIPGgaReporter::PositionSource;
    ManualScheduler scheduler;
    NTRIPManager manager(nullptr, &scheduler);
    auto configuration = mockCasterConfiguration();
    configuration.gga = {Source::VehicleGPS, std::chrono::seconds{60}};
    manager.setConfiguration(configuration);
    manager.setGgaPositionProvider(Source::VehicleGPS, []() { return ggaObservation(QGeoCoordinate(47, 8, 500)); });
    manager.setGgaPositionProvider(Source::GCSPosition, []() { return ggaObservation(QGeoCoordinate(48, 9, 600)); });
    auto* transport = injectMockTransport(manager, true);
    manager.init();
    QCOMPARE(manager.ggaSource(), NTRIPGgaReporter::tr("Vehicle GPS"));
    QCOMPARE(transport->sentNmea.size(), 1);
    QSignalSpy transitions(&manager, &NTRIPManager::connectionStatusChanged);

    expectLogMessage("GPS.NTRIPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Inject GGA position providers before initializing NTRIP")));
    manager.setGgaPositionProvider(Source::GCSPosition, []() { return std::optional<GPSObservation>{}; });
    verifyExpectedLogMessage();
    configuration.gga = {Source::GCSPosition, std::chrono::seconds{1}};
    manager.setConfiguration(configuration);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds{1}));
    QCOMPARE(manager.ggaSource(), NTRIPGgaReporter::tr("GCS Position"));
    QVERIFY(transport->sentNmea.size() >= 2);
    QCOMPARE(transport->startCount, 1);
    QCOMPARE(transport->stopCount, 0);
    QVERIFY(transitions.isEmpty());
    manager._stop();
}

void NTRIPManagerTest::_factChangesReconfigureTransport()
{
    QFETCH(QString, setting);
    ManualScheduler scheduler;
    GPSCorrectionManager corrections(nullptr, &scheduler);
    NTRIPManager mgr(nullptr, &scheduler, {.corrections = &corrections});
    auto configuration = mockCasterConfiguration();
    configuration.filter.whitelist = QStringLiteral("1005");
    mgr.setConfiguration(configuration);
    auto* first = injectMockTransport(mgr, true);
    int stops = 0;
    first->onStop = [&]() { ++stops; };
    mgr.init();
    QCOMPARE(first->startCount, 1);
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Connected);
    QList<QByteArray> routed;
    captureVehicleFrames(corrections, routed);
    first->simulateRtcmData(GPSTest::rtcmMessage(1005), 1005, scheduler.nowMs());
    deliverQueuedCalls();
    QCOMPARE(routed.size(), 1);

    auto* second = injectMockTransport(mgr, true);
    const bool reconnect = setting == QStringLiteral("mountpoint");
    auto changed = configuration;
    if (reconnect) {
        changed.connection.mountpoint = QStringLiteral("REPLACEMENT");
    } else {
        changed.filter.whitelist = QStringLiteral("1077");
    }
    mgr.setConfiguration(changed);
    QVERIFY(scheduler.advanceBy(SettingsDebounce));
    QCOMPARE(second->startCount, reconnect ? 1 : 0);
    QCOMPARE(stops, reconnect ? 1 : 0);
    auto* active = reconnect ? second : first;
    QCOMPARE(active->startCount, 1);
    QCOMPARE(active->stopCount, 0);
    if (setting == QStringLiteral("whitelist")) {
        QCOMPARE(active->lastWhitelist, QVector<int>{1077});
        active->simulateRtcmData(GPSTest::rtcmMessage(1005), 1005, scheduler.nowMs());
    }
    const auto frame = GPSTest::rtcmMessage(1077);
    active->simulateRtcmData(frame, 1077, scheduler.nowMs() - 10);
    deliverQueuedCalls();
    QCOMPARE(routed.size(), 2);
    QCOMPARE(routed[1], frame);
    // Only a reconnect names a new stream.
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(100)));
    QCOMPARE(corrections.selectedStream().instanceId,
             reconnect ? QStringLiteral("ntrip://caster.example.com:2101/REPLACEMENT")
                       : QStringLiteral("ntrip://caster.example.com:2101/TEST"));
    changed.enabled = false;
    mgr.setConfiguration(changed);
    mgr.setConfiguration(changed);
    QVERIFY(scheduler.advanceBy(SettingsDebounce));
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(100)));
    QCOMPARE(corrections.state(), GPSCorrectionManager::State::Inactive);
}

void NTRIPManagerTest::_transportDiagnosticsReachManager()
{
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    GPSCorrectionManager corrections;
    NTRIPManager mgr(nullptr, nullptr, {.corrections = &corrections});
    auto configuration = mockCasterConfiguration();
    configuration.connection = caster.connectionConfig();
    configuration.filter.whitelist = QStringLiteral("1005");
    mgr.setConfiguration(configuration);
    QList<QByteArray> routed;
    captureVehicleFrames(corrections, routed);
    mgr._start();
    auto* connection = caster.waitForConnection(TestTimeout::shortMs());
    QVERIFY(connection && connection->peer);
    QVERIFY(connection->waitForRequest(TestTimeout::shortMs()).startsWith("GET /TEST HTTP/1.1"));
    const auto accepted = GPSTest::rtcmMessage(1005);
    const auto filtered = GPSTest::rtcmMessage(1077);
    auto rejected = GPSTest::rtcmMessage(1087);
    rejected.back() ^= 1;
    expectLogMessage("GPS.NTRIPHttpTransport", QtWarningMsg, QRegularExpression(QStringLiteral("Invalid RTCM frame")));
    const QByteArray response = okChunkedResponse(accepted + filtered + rejected, false);
    QCOMPARE(connection->write(response), response.size());
    QTRY_COMPARE_WITH_TIMEOUT(mgr.connectionStats()->messagesReceived(), quint32(1), TestTimeout::shortMs());
    verifyExpectedLogMessage();
    deliverQueuedCalls();
    // Only the whitelisted valid frame reaches vehicles; the filtered and the corrupt one do not.
    QCOMPARE(routed, QList<QByteArray>{accepted});
    mgr._stop();
}

UT_REGISTER_TEST_LIGHTWEIGHT(NTRIPManagerTest, TestLabel::Unit)
