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

/// The casters a manager's connection attempts go to, in order, each attempt a mock transport.
class Attempts
{
public:
    explicit Attempts(NTRIPManager& manager, bool autoConnect = false)
    {
        manager.setTransportFactory(
            [this, autoConnect](const NTRIPManager::Configuration& configuration, QObject* parent) {
                hosts.append(configuration.connection.host);
                latest = new MockNTRIPTransport(parent);
                latest->autoConnect = autoConnect;
                return latest;
            });
    }

    QStringList hosts;
    QPointer<MockNTRIPTransport> latest;
};

const QString kBackupHost = QStringLiteral("backup.example.com");

/// A saved caster at @a host, which differs from mockCasterConfiguration()'s.
NTRIPManager::SavedCaster savedCaster(const QString& name, const QString& host = kBackupHost)
{
    NTRIPManager::SavedCaster caster;
    caster.name = name;
    caster.connection = mockCasterConfiguration().connection;
    caster.connection.host = host;
    return caster;
}

/// mockCasterConfiguration() with @a casters saved and failover on.
NTRIPManager::Configuration failoverConfiguration(const QList<NTRIPManager::SavedCaster>& casters)
{
    NTRIPManager::Configuration configuration = mockCasterConfiguration();
    configuration.failover = true;
    configuration.savedCasters = casters;
    return configuration;
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

bool NTRIPManagerTest::_failAttempts(NTRIPManager& manager, ManualScheduler& scheduler,
                                     QPointer<MockNTRIPTransport>& latest, int times)
{
    for (int attempt = 0; attempt < times; ++attempt) {
        const MockNTRIPTransport* const failed = latest;
        _fail(latest, QStringLiteral("unreachable"));
        // No delay between attempts is longer.
        if (manager.connectionStatus() != NTRIPManager::ConnectionStatus::Reconnecting ||
            !scheduler.advanceBy(std::chrono::seconds(30)) || latest == failed) {
            return false;
        }
    }
    return true;
}

void NTRIPManagerTest::_stopFromIdleIsNoop()
{
    NTRIPManager manager;
    QSignalSpy transitions(&manager, &NTRIPManager::connectionStatusChanged);
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);
    // A connection that is off has nothing to stop, when it is applied or at shutdown.
    manager.init();
    manager.shutdown();
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);
    QVERIFY(transitions.isEmpty());
}

void NTRIPManagerTest::_statusMessageNamesStateWithoutDetail()
{
    // The panel shows the message alone, so it is never empty: not before the first connection, and not after a
    // failure that gives no reason.
    NTRIPManager manager;
    QCOMPARE(manager.property("statusMessage").toString(), NTRIPManager::tr("Disconnected"));
    auto* transport = injectMockTransport(manager);
    initialize(manager);
    QSignalSpy messages(&manager, &NTRIPManager::statusMessageChanged);
    _fail(transport, QString(), NTRIPError::AuthFailed);
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Error);
    QCOMPARE(manager.statusMessage(), NTRIPManager::tr("Error"));
    QCOMPARE(messages.size(), 1);
}

void NTRIPManagerTest::_canConnectNeedsCasterHost()
{
    NTRIPManager manager;
    QSignalSpy changes(&manager, &NTRIPManager::canConnectChanged);
    // The panels read the property.
    QVERIFY(!manager.property("canConnect").toBool());
    auto configuration = mockCasterConfiguration(false);
    manager.setConfiguration(configuration);
    QVERIFY(manager.property("canConnect").toBool());
    QCOMPARE(changes.size(), 1);

    // The host alone decides: the mountpoint browser needs a caster before there is a mountpoint.
    configuration.connection.mountpoint.clear();
    manager.setConfiguration(configuration);
    QVERIFY(manager.canConnect());
    QCOMPARE(changes.size(), 1);

    configuration.connection.host.clear();
    manager.setConfiguration(configuration);
    QVERIFY(!manager.canConnect());
    QCOMPARE(changes.size(), 2);
}

void NTRIPManagerTest::_statusKeepsPlaceMarkers_data()
{
    QTest::addColumn<QString>("host");
    // A link-local IPv6 address with a numeric zone, as Windows writes it.
    QTest::newRow("zone-index") << QStringLiteral("fe80::1%2");
    QTest::newRow("first-marker") << QStringLiteral("a%1b");
}

void NTRIPManagerTest::_statusKeepsPlaceMarkers()
{
    QFETCH(QString, host);
    ManualScheduler scheduler;
    NTRIPManager manager(nullptr, &scheduler);
    auto configuration = mockCasterConfiguration();
    configuration.connection.host = host;
    auto* transport = injectMockTransport(manager);
    initialize(manager, configuration);
    QCOMPARE(manager.statusMessage(), QStringLiteral("Connecting to %1:2101...").arg(host));
    // What a caster answers is text of its own as well.
    const QString detail = QStringLiteral("HTTP 503: %1 busy %2");
    _fail(transport, detail);
    QCOMPARE(manager.statusMessage(), QStringLiteral("Reconnecting in 1s: ") + detail);
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
    QTest::newRow("connection-turned-off") << false;
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
        QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);
    } else {
        // Turned off before the changed mountpoint was applied.
        changed.enabled = false;
        manager.setConfiguration(changed);
    }
    QVERIFY(scheduler.advanceBy(SettingsDebounce));
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);
    QCOMPARE(replacement->startCount, 0);
    // Only a manager that was not shut down connects again.
    changed.enabled = true;
    changed.connection.mountpoint = QStringLiteral("AFTER_STOP");
    manager.setConfiguration(changed);
    QVERIFY(scheduler.advanceBy(SettingsDebounce));
    QCOMPARE(replacement->startCount, shutdown ? 0 : 1);
}

void NTRIPManagerTest::_newSessionRetryBudget_data()
{
    QTest::addColumn<int>("action");
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
    if (action == 1) {
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
        QVERIFY(manager.metaObject()->indexOfMethod("connectCaster()") >= 0);
        QSignalSpy enabledRequested(&manager, &NTRIPManager::enabledRequested);
        manager.connectCaster();
        QCOMPARE(enabledRequested.count(), 1);
        QVERIFY(enabledRequested.constFirst().constFirst().toBool());
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
            manager.shutdown();
        }
    });
    manager.init();
    // Observers see the settled state after the transport has started, never a half-entered state.
    QCOMPARE(observed, (QList<NTRIPManager::ConnectionStatus>{NTRIPManager::ConnectionStatus::Connecting,
                                                              NTRIPManager::ConnectionStatus::Disconnected}));
    QCOMPARE(transport->startCount, 1);
    QCOMPARE(transport->stopCount, 1);
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);
}

void NTRIPManagerTest::_connectedCallbackStopsTransition_data()
{
    QTest::addColumn<bool>("position");
    QTest::newRow("no-position") << false;
    // The first position report publishes the connected state while the reporter is still starting.
    QTest::newRow("position-reported-at-once") << true;
}

void NTRIPManagerTest::_connectedCallbackStopsTransition()
{
    QFETCH(bool, position);
    NTRIPManager manager;
    if (position) {
        manager.setGgaPositionProvider(NTRIPGgaReporter::PositionSource::VehicleGPS,
                                       []() { return ggaObservation(QGeoCoordinate(47, 8, 500)); });
    }
    auto* transport = injectMockTransport(manager);
    initialize(manager);
    connect(&manager, &NTRIPManager::connectionStatusChanged, this, [&]() {
        if (manager.connectionStatus() == NTRIPManager::ConnectionStatus::Connected) {
            manager.shutdown();
        }
    });
    transport->simulateConnect();
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);
    QCOMPARE(manager.statusMessage(), NTRIPManager::tr("Disconnected"));
    QCOMPARE(transport->stopCount, 1);
    QCOMPARE(transport->sentNmea.size(), position ? 1 : 0);
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

    mgr.shutdown();
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
    manager.shutdown();
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
    _fail(transport, QStringLiteral("last refusal %1"));
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Error);
    QCOMPARE(manager.statusMessage(),
             QStringLiteral("Gave up after %1 reconnect attempts: ").arg(ATTEMPTS) + QStringLiteral("last refusal %1"));
}

void NTRIPManagerTest::_reconnectCancelStopsTimer()
{
    ManualScheduler scheduler;
    NTRIPManager mgr(nullptr, &scheduler);
    auto* transport = injectMockTransport(mgr);
    initialize(mgr);
    _fail(transport, QStringLiteral("cancel"));
    auto* retry = injectMockTransport(mgr);
    // The setting is applied after its debounce, which is shorter than the first reconnect delay.
    mgr.setConfiguration(mockCasterConfiguration(false));
    QVERIFY(scheduler.advanceBy(SettingsDebounce));
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
            manager.shutdown();
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
    // A changed mountpoint replaces the transport before its queued failure is delivered.
    auto* second = injectMockTransport(mgr);
    auto changed = mgr.configuration();
    changed.connection.mountpoint = QStringLiteral("OTHER");
    mgr.setConfiguration(changed);
    QVERIFY(scheduler.advanceBy(SettingsDebounce));
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
    auto* transport = injectMockTransport(manager);
    initialize(manager);
    if (!enabled) {
        // Turned off while the attempt runs: the setting applies after its debounce, and the failure comes first.
        manager.setConfiguration(mockCasterConfiguration(false));
    }

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
    // Turning the connection off and on starts over with the shortest delay.
    manager.setConfiguration(mockCasterConfiguration(false));
    QVERIFY(scheduler.advanceBy(SettingsDebounce));
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);
    auto* replacement = injectMockTransport(manager, true);
    manager.setConfiguration(mockCasterConfiguration(true));
    QVERIFY(scheduler.advanceBy(SettingsDebounce));
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Connected);
    _fail(replacement, QStringLiteral("fresh failure"));
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Reconnecting);
    QVERIFY(manager.statusMessage().contains(QStringLiteral("1s")));
}

void NTRIPManagerTest::_unusableMountpointRetriedOnlyAfterCorrections_data()
{
    QTest::addColumn<NTRIPError>("code");
    // The caster answers with its source table, or with data that holds no RTCM message.
    QTest::newRow("source-table") << NTRIPError::InvalidMountpoint;
    QTest::newRow("other-format") << NTRIPError::InvalidStream;
}

void NTRIPManagerTest::_unusableMountpointRetriedOnlyAfterCorrections()
{
    QFETCH(NTRIPError, code);
    const QString detail = QStringLiteral("unusable mountpoint");
    ManualScheduler scheduler;
    NTRIPManager manager(nullptr, &scheduler);
    auto* transport = injectMockTransport(manager, true);
    initialize(manager);

    // Before any corrections the mountpoint is wrong, and asking again only counts against the client at the caster.
    _fail(transport, detail, code);
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Error);
    QCOMPARE(manager.statusMessage(), detail);

    // A mountpoint that has served corrections has only gone offline.
    transport = injectMockTransport(manager, true);
    manager.connectCaster();
    transport->simulateRtcmData(GPSTest::rtcmMessage(1005), 1005, scheduler.nowMs());
    deliverQueuedCalls();
    _fail(transport, detail, code);
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Reconnecting);
    transport = injectMockTransport(manager, true);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(1)));
    QCOMPARE(transport->startCount, 1);
    _fail(transport, detail, code);
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Reconnecting);

    // Another mountpoint has served nothing yet.
    auto changed = manager.configuration();
    changed.connection.mountpoint = QStringLiteral("OTHER");
    transport = injectMockTransport(manager, true);
    manager.setConfiguration(changed);
    QVERIFY(scheduler.advanceBy(SettingsDebounce));
    QCOMPARE(transport->startCount, 1);
    _fail(transport, detail, code);
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Error);
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
    initialize(manager, configuration);
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

void NTRIPManagerTest::_unrepeatableAnswerWaitsForUser_data()
{
    QTest::addColumn<QByteArray>("response");
    QTest::addColumn<QString>("logged");
    QTest::addColumn<QString>("message");
    QTest::newRow("redirect") << QByteArray(
                                     "HTTP/1.1 301 Moved Permanently\r\nLocation: https://caster.example:2102/TEST\r\n"
                                     "Content-Length: 0\r\n\r\n")
                              << QStringLiteral("301")
                              << QStringLiteral("redirected the request to https://caster.example:2102/TEST");
    // A mountpoint the caster does not have, before it served any corrections.
    QTest::newRow("not-found") << QByteArray("HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n")
                               << QStringLiteral("404") << QStringLiteral("no such mountpoint");
}

void NTRIPManagerTest::_unrepeatableAnswerWaitsForUser()
{
    QFETCH(QByteArray, response);
    QFETCH(QString, logged);
    QFETCH(QString, message);
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    ManualScheduler scheduler;
    NTRIPManager manager(nullptr, &scheduler);
    auto configuration = mockCasterConfiguration();
    configuration.connection = caster.connectionConfig();
    initialize(manager, configuration);
    auto* connection = caster.waitForConnection(TestTimeout::shortMs());
    QVERIFY(connection && connection->peer);
    QVERIFY(connection->waitForRequest(TestTimeout::shortMs()).startsWith("GET /TEST HTTP/1.1"));
    expectLogMessage("GPS.NTRIPManager", QtWarningMsg, QRegularExpression(QStringLiteral("NTRIP error:.*") + logged));
    QCOMPARE(connection->write(response), response.size());
    // The request is not repeated: the caster would answer it the same way every time.
    QTRY_VERIFY_WITH_TIMEOUT(manager.connectionStatus() != NTRIPManager::ConnectionStatus::Connecting,
                             TestTimeout::shortMs());
    verifyExpectedLogMessage();
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Error);
    QVERIFY2(manager.statusMessage().contains(message), qPrintable(manager.statusMessage()));
    auto* retry = injectMockTransport(manager);
    QVERIFY(scheduler.advanceBy(NTRIPFailure::MAX_RETRY_AFTER));
    QCOMPARE(retry->startCount, 0);
}

void NTRIPManagerTest::_version1CasterIsRemembered()
{
    using Status = NTRIPManager::ConnectionStatus;
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    ManualScheduler scheduler;
    NTRIPManager manager(nullptr, &scheduler);
    auto configuration = mockCasterConfiguration();
    configuration.connection = caster.connectionConfig();
    QList<Status> observed;
    connect(&manager, &NTRIPManager::connectionStatusChanged, this,
            [&]() { observed.append(manager.connectionStatus()); });
    QSignalSpy casters(&manager, &NTRIPManager::version1CastersChanged);
    const QRegularExpression failure(QStringLiteral("NTRIP error:"));

    // The caster closes the request and serves its repeat in NTRIP version 1 form: one attempt, and not a failed one.
    expectLogMessage("GPS.NTRIP.NTRIPHttpSession", QtWarningMsg, repeatedInVersion1());
    initialize(manager, configuration);
    auto* connection = caster.waitForVersion1Request(TestTimeout::shortMs());
    QVERIFY(connection && connection->peer);
    verifyExpectedLogMessage();
    const QByteArray stream = "ICY 200 OK\r\n" + GPSTest::rtcmMessage(1005);
    QCOMPARE(connection->write(stream), stream.size());
    QTRY_COMPARE_WITH_TIMEOUT(manager.connectionStats()->messagesReceived(), quint32(1), TestTimeout::shortMs());
    QCOMPARE(observed, (QList<Status>{Status::Connecting, Status::Connected}));
    // The settings are given the caster to keep for the next run.
    QCOMPARE(casters.size(), 1);
    QCOMPARE(casters.constLast().constFirst().toString(), configuration.connection.url(false).toString());
    QCOMPARE(manager.configuration().version1Casters, configuration.connection.url(false).toString());

    // The mountpoint browser now asks this caster in version 1 form at once.
    manager.fetchMountpoints();
    auto* fetch = caster.waitForConnection(TestTimeout::shortMs());
    QVERIFY(fetch && fetch->peer);
    QVERIFY(fetch->waitForRequest(TestTimeout::shortMs()).startsWith("GET / HTTP/1.0\r\n"));
    manager.sourceTableController()->cancel();

    // So does a reconnect.
    expectLogMessage("GPS.NTRIPManager", QtWarningMsg, failure);
    connection->disconnectFromHost();
    QTRY_COMPARE_WITH_TIMEOUT(manager.connectionStatus(), Status::Reconnecting, TestTimeout::shortMs());
    verifyExpectedLogMessage();
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(1)));
    connection = caster.waitForConnection(TestTimeout::shortMs());
    QVERIFY(connection && connection->peer);
    QVERIFY(connection->waitForRequest(TestTimeout::shortMs()).startsWith("GET /TEST HTTP/1.0\r\n"));

    // A version 1 request the caster closes without an answer is not repeated, and the next attempt starts over.
    expectLogMessage("GPS.NTRIPManager", QtWarningMsg, failure);
    connection->disconnectFromHost();
    QTRY_COMPARE_WITH_TIMEOUT(manager.connectionStatus(), Status::Reconnecting, TestTimeout::shortMs());
    verifyExpectedLogMessage();
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(2)));
    connection = caster.waitForConnection(TestTimeout::shortMs());
    QVERIFY(connection && connection->peer);
    QVERIFY(connection->waitForRequest(TestTimeout::shortMs()).startsWith("GET /TEST HTTP/1.1\r\n"));
    QCOMPARE(casters.size(), 2);
    QVERIFY(casters.constLast().constFirst().toString().isEmpty());
    manager.shutdown();
}

void NTRIPManagerTest::_savedVersion1CasterIsAskedInThatFormAtOnce()
{
    using Status = NTRIPManager::ConnectionStatus;
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    ManualScheduler scheduler;
    NTRIPManager manager(nullptr, &scheduler);
    auto configuration = mockCasterConfiguration();
    configuration.connection = caster.connectionConfig();
    // As an earlier run reported the caster, and the settings kept it.
    configuration.version1Casters = configuration.connection.url(false).toString();
    QSignalSpy casters(&manager, &NTRIPManager::version1CastersChanged);

    initialize(manager, configuration);
    auto* connection = caster.waitForConnection(TestTimeout::shortMs());
    QVERIFY(connection && connection->peer);
    QVERIFY(connection->waitForRequest(TestTimeout::shortMs()).startsWith("GET /TEST HTTP/1.0\r\n"));
    QVERIFY(casters.isEmpty());

    // The caster no longer takes that form, and refuses it instead of closing the connection: it is forgotten, so
    // that the settings do not keep it for ever.
    expectLogMessage("GPS.NTRIPManager", QtWarningMsg, QRegularExpression(QStringLiteral("NTRIP error:")));
    const QByteArray refusal = "HTTP/1.1 400 Bad Request\r\n\r\n";
    QCOMPARE(connection->write(refusal), refusal.size());
    connection->disconnectFromHost();
    QTRY_COMPARE_WITH_TIMEOUT(manager.connectionStatus(), Status::Error, TestTimeout::shortMs());
    verifyExpectedLogMessage();
    QCOMPARE(casters.size(), 1);
    QVERIFY(casters.constFirst().constFirst().toString().isEmpty());
    QVERIFY(manager.configuration().version1Casters.isEmpty());

    // The next attempt asks in version 2 form again.
    manager.connectCaster();
    connection = caster.waitForConnection(TestTimeout::shortMs());
    QVERIFY(connection && connection->peer);
    QVERIFY(connection->waitForRequest(TestTimeout::shortMs()).startsWith("GET /TEST HTTP/1.1\r\n"));
    manager.shutdown();
}

void NTRIPManagerTest::_retryPublicationSuperseded()
{
    ManualScheduler scheduler;
    NTRIPManager manager(nullptr, &scheduler);
    auto* first = injectMockTransport(manager, true);
    initialize(manager);
    auto* replacement = injectMockTransport(manager);
    bool replaced = false;
    // Changed settings take effect while the Reconnecting status is announced, which connects anew at once; the
    // retry already scheduled must not fire.
    connect(&manager, &NTRIPManager::connectionStatusChanged, this, [&]() {
        if (manager.connectionStatus() != NTRIPManager::ConnectionStatus::Reconnecting ||
            std::exchange(replaced, true)) {
            return;
        }
        injectNextTransport(manager, replacement);
        auto changed = manager.configuration();
        changed.connection.mountpoint = QStringLiteral("OTHER");
        manager.setConfiguration(changed);
        QVERIFY(scheduler.advanceBy(SettingsDebounce));
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
    mgr.init();
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
    QVERIFY(result.valid);
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
    // On the wall clock this test runs on, every public restart waits in the event loop, which would deliver the
    // queued frame to the session that sent it.
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
    manager.shutdown();
}

void NTRIPManagerTest::_connectedMessageSaysWhatTheStreamWaitsFor_data()
{
    QTest::addColumn<bool>("positionArrives");
    QTest::newRow("position-arrives") << true;
    // A mountpoint of a single base needs no position.
    QTest::newRow("corrections-arrive") << false;
}

void NTRIPManagerTest::_connectedMessageSaysWhatTheStreamWaitsFor()
{
    QFETCH(bool, positionArrives);
    ManualScheduler scheduler;
    NTRIPManager manager(nullptr, &scheduler);
    std::optional<GPSObservation> position;
    manager.setGgaPositionProvider(NTRIPGgaReporter::PositionSource::VehicleGPS, [&position]() { return position; });
    auto* transport = injectMockTransport(manager, true);
    initialize(manager);

    // A network RTK (VRS) mountpoint sends nothing until it is told a position, so the silence is explained.
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Connected);
    QCOMPARE(manager.statusMessage(), NTRIPManager::tr("Connected, waiting for a position to report"));
    QVERIFY(transport->sentNmea.isEmpty());

    QSignalSpy messages(&manager, &NTRIPManager::statusMessageChanged);
    if (positionArrives) {
        // It may then take a minute or more over its first corrections.
        position = ggaObservation(QGeoCoordinate(47, 8, 500));
        QVERIFY(scheduler.advanceBy(NTRIPGgaReporter::FAST_RETRY_INTERVAL));
        QCOMPARE(transport->sentNmea.size(), 1);
        QCOMPARE(manager.statusMessage(), NTRIPManager::tr("Connected, waiting for corrections"));
        QCOMPARE(messages.size(), 1);
        messages.clear();
    }
    transport->simulateRtcmData(GPSTest::rtcmMessage(1005), 1005, scheduler.nowMs());
    deliverQueuedCalls();
    QCOMPARE(manager.statusMessage(), NTRIPManager::tr("Connected"));
    QCOMPARE(messages.size(), 1);
    manager.shutdown();
}

void NTRIPManagerTest::_mountpointChangeReconnects()
{
    ManualScheduler scheduler;
    GPSCorrectionManager corrections(nullptr, &scheduler);
    NTRIPManager mgr(nullptr, &scheduler, {.corrections = &corrections});
    const auto configuration = mockCasterConfiguration();
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
    auto changed = configuration;
    changed.connection.mountpoint = QStringLiteral("REPLACEMENT");
    mgr.setConfiguration(changed);
    QVERIFY(scheduler.advanceBy(SettingsDebounce));
    QCOMPARE(second->startCount, 1);
    QCOMPARE(stops, 1);
    QCOMPARE(second->stopCount, 0);
    const auto frame = GPSTest::rtcmMessage(1077);
    second->simulateRtcmData(frame, 1077, scheduler.nowMs() - 10);
    deliverQueuedCalls();
    QCOMPARE(routed.size(), 2);
    QCOMPARE(routed[1], frame);
    // The reconnect names a new stream.
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(100)));
    QCOMPARE(corrections.selectedStream().instanceId, QStringLiteral("ntrip://caster.example.com:2101/REPLACEMENT"));
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
    mgr.setConfiguration(configuration);
    corrections.setConfiguration({.udpInput = {}, .udpOutput = {}, .messageFilter = QStringLiteral("1005")});
    QList<QByteArray> routed;
    captureVehicleFrames(corrections, routed);
    mgr.init();
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
    // The connection counts every valid frame the caster sends, whatever the routing's message filter takes of them.
    QTRY_COMPARE_WITH_TIMEOUT(mgr.connectionStats()->messagesReceived(), quint32(2), TestTimeout::shortMs());
    verifyExpectedLogMessage();
    deliverQueuedCalls();
    QCOMPARE(mgr.connectionStats()->bytesReceived(), quint64(accepted.size() + filtered.size()));
    // Only the valid frame the filter takes reaches vehicles; the filtered and the corrupt one do not.
    QCOMPARE(routed, QList<QByteArray>{accepted});
    mgr.shutdown();
}

void NTRIPManagerTest::_failoverGoesOnAfterThreeFailures_data()
{
    QTest::addColumn<bool>("failover");
    QTest::newRow("on") << true;
    // Saved casters alone change nothing: the entered caster is retried as before.
    QTest::newRow("off") << false;
}

void NTRIPManagerTest::_failoverGoesOnAfterThreeFailures()
{
    using Status = NTRIPManager::ConnectionStatus;
    QFETCH(bool, failover);
    const QString entered = mockCasterConfiguration().connection.host;
    const QString suffix = QStringLiteral(" (saved caster Backup)");
    ManualScheduler scheduler;
    NTRIPManager manager(nullptr, &scheduler);
    Attempts attempts(manager);
    auto configuration = failoverConfiguration({savedCaster(QStringLiteral("Backup"))});
    configuration.failover = failover;
    initialize(manager, configuration);

    // The delays between attempts are those of a single caster, whichever caster the next attempt goes to.
    for (const int seconds : {1, 2, 4}) {
        QVERIFY(!manager.statusMessage().endsWith(suffix));
        _fail(attempts.latest, QStringLiteral("unreachable"));
        QCOMPARE(manager.connectionStatus(), Status::Reconnecting);
        QVERIFY(manager.statusMessage().contains(QStringLiteral("in %1s: unreachable").arg(seconds)));
        QVERIFY(scheduler.advanceBy(std::chrono::seconds(seconds) - std::chrono::milliseconds(1)));
        QCOMPARE(manager.connectionStatus(), Status::Reconnecting);
        QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
        QCOMPARE(manager.connectionStatus(), Status::Connecting);
    }
    // Three failed attempts without corrections leave the entered caster for the next saved one.
    const QString fourth = failover ? kBackupHost : entered;
    QCOMPARE(attempts.hosts, (QStringList{entered, entered, entered, fourth}));
    // The status says so for as long as the caster in use is not the one entered.
    QCOMPARE(manager.statusMessage().endsWith(suffix), failover);
    QVERIFY(manager.statusMessage().contains(fourth));
    attempts.latest->simulateRtcmData(GPSTest::rtcmMessage(1005), 1005, scheduler.nowMs());
    deliverQueuedCalls();
    QCOMPARE(manager.connectionStatus(), Status::Connected);
    QCOMPARE(manager.statusMessage(),
             failover ? NTRIPManager::tr("Connected") + suffix : NTRIPManager::tr("Connected"));

    // A caster that served is kept: it has three attempts again before the next one is tried.
    _fail(attempts.latest, QStringLiteral("dropped"), NTRIPError::ServerDisconnected);
    QCOMPARE(manager.statusMessage().endsWith(suffix), failover);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(1)));
    QCOMPARE(attempts.hosts.last(), fourth);
    manager.shutdown();
}

void NTRIPManagerTest::_failoverLeavesAWrongLoginToTheUser()
{
    ManualScheduler scheduler;
    NTRIPManager manager(nullptr, &scheduler);
    Attempts attempts(manager);
    initialize(manager, failoverConfiguration({savedCaster(QStringLiteral("Backup"))}));
    // What is wrong with the caster that is entered is for the user to see, not to be hidden by another caster.
    _fail(attempts.latest, QStringLiteral("check username and password"), NTRIPError::AuthFailed);
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Error);
    QCOMPARE(manager.statusMessage(), QStringLiteral("check username and password"));
    QVERIFY(scheduler.advanceBy(NTRIPFailure::MAX_RETRY_AFTER));
    QCOMPARE(attempts.hosts.size(), 1);
}

void NTRIPManagerTest::_failoverLeavesOutASavedCasterThatRefuses()
{
    using Status = NTRIPManager::ConnectionStatus;
    const QString entered = mockCasterConfiguration().connection.host;
    const QString third = QStringLiteral("third.example.com");
    ManualScheduler scheduler;
    NTRIPManager manager(nullptr, &scheduler);
    Attempts attempts(manager);
    initialize(manager, failoverConfiguration(
                            {savedCaster(QStringLiteral("Backup")), savedCaster(QStringLiteral("Third"), third)}));
    QVERIFY(_failAttempts(manager, scheduler, attempts.latest, 3));
    QCOMPARE(attempts.hosts.last(), kBackupHost);

    // A saved caster that refuses the login would refuse it again: the next one is tried, and the connection goes
    // on instead of ending with an error about a caster that is not the one entered.
    expectLogMessage("GPS.NTRIPManager", QtWarningMsg, QRegularExpression(QStringLiteral("NTRIP error:")));
    expectLogMessage("GPS.NTRIPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Leaving out saved caster \"Backup\"")));
    attempts.latest->simulateError(NTRIPError::AuthFailed, QStringLiteral("check username and password"));
    deliverQueuedCalls();
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QCOMPARE(manager.connectionStatus(), Status::Reconnecting);
    QVERIFY(manager.statusMessage().endsWith(QStringLiteral("check username and password (saved caster Third)")));
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(30)));
    QCOMPARE(attempts.hosts.last(), third);

    // After the last saved caster comes the entered one again, and then the saved casters without the one left out.
    QVERIFY(_failAttempts(manager, scheduler, attempts.latest, 3));
    QCOMPARE(attempts.hosts.last(), entered);
    QVERIFY(!manager.statusMessage().contains(QStringLiteral("saved caster")));
    QVERIFY(_failAttempts(manager, scheduler, attempts.latest, 3));
    QCOMPARE(attempts.hosts.last(), third);
    QCOMPARE(attempts.hosts.count(kBackupHost), 1);
    manager.shutdown();
}

void NTRIPManagerTest::_failoverTakesOnlyCastersItCanUse_data()
{
    QTest::addColumn<NTRIPManager::SavedCaster>("caster");
    QTest::addColumn<bool>("used");
    const auto changed = [](auto change) {
        NTRIPManager::SavedCaster caster = savedCaster(QStringLiteral("Backup"));
        change(caster.connection);
        return caster;
    };
    QTest::newRow("another-host") << changed([](NTRIPConnectionConfig&) {}) << true;
    // The caster that is entered, saved: only its trusted certificate may differ.
    QTest::newRow("the-entered-caster") << changed([](NTRIPConnectionConfig& connection) {
        connection.host = mockCasterConfiguration().connection.host;
        connection.pinnedCertificate = QStringLiteral("caster.example.com:2101|") + QString(64, u'a');
    }) << false;
    QTest::newRow("another-mountpoint-of-the-entered-caster") << changed([](NTRIPConnectionConfig& connection) {
        connection.host = mockCasterConfiguration().connection.host;
        connection.mountpoint = QStringLiteral("OTHER");
    }) << true;
    QTest::newRow("no-mountpoint") << changed([](NTRIPConnectionConfig& connection) { connection.mountpoint.clear(); })
                                   << false;
    // A self-signed certificate is accepted only against the one trusted before.
    QTest::newRow("self-signed-never-trusted") << changed([](NTRIPConnectionConfig& connection) {
        connection.useTls = true;
        connection.allowSelfSignedCerts = true;
    }) << false;
    QTest::newRow("self-signed-trusted") << changed([](NTRIPConnectionConfig& connection) {
        connection.useTls = true;
        connection.allowSelfSignedCerts = true;
        connection.pinnedCertificate = connection.endpoint() + QLatin1Char('|') + QString(64, u'a');
    }) << true;
}

void NTRIPManagerTest::_failoverTakesOnlyCastersItCanUse()
{
    QFETCH(NTRIPManager::SavedCaster, caster);
    QFETCH(bool, used);
    ManualScheduler scheduler;
    NTRIPManager manager(nullptr, &scheduler);
    QList<NTRIPConnectionConfig> attempted;
    QPointer<MockNTRIPTransport> latest;
    manager.setTransportFactory([&](const NTRIPManager::Configuration& configuration, QObject* parent) {
        attempted.append(configuration.connection);
        latest = new MockNTRIPTransport(parent);
        return latest.data();
    });
    const auto configuration = failoverConfiguration({caster});
    initialize(manager, configuration);
    QVERIFY(_failAttempts(manager, scheduler, latest, 3));
    QCOMPARE(attempted.size(), 4);
    QCOMPARE(attempted.last(), used ? caster.connection : configuration.connection);
    QCOMPARE(manager.statusMessage().endsWith(QStringLiteral("(saved caster Backup)")), used);
    manager.shutdown();
}

void NTRIPManagerTest::_failoverDropsAnotherCastersRetryAfter()
{
    ManualScheduler scheduler;
    NTRIPManager manager(nullptr, &scheduler);
    Attempts attempts(manager);
    initialize(manager, failoverConfiguration({savedCaster(QStringLiteral("Backup"))}));
    QVERIFY(_failAttempts(manager, scheduler, attempts.latest, 2));
    // The entered caster asks to be left alone for five minutes, which says nothing of the next caster.
    _fail(attempts.latest, QStringLiteral("busy"), NTRIPError::HttpError, std::chrono::seconds(300));
    QVERIFY(manager.statusMessage().contains(QStringLiteral("in 4s: busy")));
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(4)));
    QCOMPARE(attempts.hosts.last(), kBackupHost);
    // On the caster that asks, it still holds.
    _fail(attempts.latest, QStringLiteral("busy too"), NTRIPError::HttpError, std::chrono::seconds(300));
    QVERIFY(manager.statusMessage().contains(QStringLiteral("in 300s: busy too")));
    manager.shutdown();
}

void NTRIPManagerTest::_failoverStartsAgainFromTheEnteredCaster_data()
{
    QTest::addColumn<int>("action");
    QTest::newRow("disconnect-and-connect") << 0;
    QTest::newRow("saved-casters-change") << 1;
    QTest::newRow("failover-turned-off") << 2;
    QTest::newRow("mountpoint-change") << 3;
}

void NTRIPManagerTest::_failoverStartsAgainFromTheEnteredCaster()
{
    using Status = NTRIPManager::ConnectionStatus;
    QFETCH(int, action);
    const QString entered = mockCasterConfiguration().connection.host;
    ManualScheduler scheduler;
    NTRIPManager manager(nullptr, &scheduler);
    Attempts attempts(manager, true);
    auto configuration = failoverConfiguration({savedCaster(QStringLiteral("Backup"))});
    initialize(manager, configuration);
    QCOMPARE(manager.connectionStatus(), Status::Connected);

    // While the entered caster is in use, the saved casters can change without a word to the stream.
    configuration.savedCasters.append(savedCaster(QStringLiteral("Third"), QStringLiteral("third.example.com")));
    manager.setConfiguration(configuration);
    QVERIFY(scheduler.advanceBy(SettingsDebounce));
    QCOMPARE(attempts.hosts, QStringList{entered});

    QVERIFY(_failAttempts(manager, scheduler, attempts.latest, 3));
    QCOMPARE(attempts.hosts.last(), kBackupHost);
    QCOMPARE(manager.connectionStatus(), Status::Connected);
    QVERIFY(manager.statusMessage().endsWith(QStringLiteral("(saved caster Backup)")));
    // The certificate setting is the entered caster's: nothing a saved caster's connection reports is stored in it.
    QSignalSpy pins(&manager, &NTRIPManager::certificatePinChanged);
    emit attempts.latest->certificatePinned(QStringLiteral("backup.example.com:2101|") + QString(64, u'b'));
    QVERIFY(pins.isEmpty());
    QVERIFY(manager.configuration().connection.pinnedCertificate.isEmpty());

    const qsizetype before = attempts.hosts.size();
    switch (action) {
        case 0:
            configuration.enabled = false;
            manager.setConfiguration(configuration);
            QVERIFY(scheduler.advanceBy(SettingsDebounce));
            QCOMPARE(manager.statusMessage(), NTRIPManager::tr("Disconnected"));
            configuration.enabled = true;
            break;
        case 1:
            configuration.savedCasters.removeLast();
            break;
        case 2:
            configuration.failover = false;
            break;
        default:
            configuration.connection.mountpoint = QStringLiteral("OTHER");
            break;
    }
    manager.setConfiguration(configuration);
    QVERIFY(scheduler.advanceBy(SettingsDebounce));
    QCOMPARE(attempts.hosts.size(), before + 1);
    QCOMPARE(attempts.hosts.last(), entered);
    QCOMPARE(manager.connectionStatus(), Status::Connected);
    QVERIFY(!manager.statusMessage().contains(QStringLiteral("saved caster")));
    manager.shutdown();
}

UT_REGISTER_TEST_LIGHTWEIGHT(NTRIPManagerTest, TestLabel::Unit)
