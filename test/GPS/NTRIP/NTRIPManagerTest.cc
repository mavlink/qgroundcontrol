#include "NTRIPManagerTest.h"

#include <limits>
#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>
#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "GPSCorrectionManager.h"
#include "GpsTestHelpers.h"
#include "ManualScheduler.h"
#include "MockNTRIPTransport.h"
#include "MonotonicClock.h"
#include "NTRIPManager.h"
#include "RTCMDecodedFrame.h"
#include "ScriptedNtripCaster.h"

namespace {
constexpr std::chrono::milliseconds SettingsDebounce{250};

NTRIPManager::Configuration testConfiguration(bool enabled = true)
{
    NTRIPManager::Configuration configuration;
    configuration.enabled = enabled;
    configuration.stream.connection.host = QStringLiteral("caster.example.com");
    configuration.stream.connection.port = 2101;
    configuration.stream.connection.mountpoint = QStringLiteral("TEST");
    return configuration;
}

void initialize(NTRIPManager& manager, const NTRIPManager::Configuration& configuration = testConfiguration())
{
    manager.setConfiguration(configuration);
    manager.init();
}

MockNTRIPTransport* injectTransport(NTRIPManager& manager, bool autoConnect = false)
{
    auto* transport = new MockNTRIPTransport(&manager);
    transport->autoConnect = autoConnect;
    manager.setTransportForTest(transport);
    return transport;
}

void deliverPostedEvents()
{
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
}
}  // namespace

void NTRIPManagerTest::cleanup()
{
    UnitTest::cleanup();
}

void NTRIPManagerTest::testInitialStateIsDisconnected()
{
    NTRIPManager manager;
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);
}

void NTRIPManagerTest::testStopFromIdleIsNoop()
{
    NTRIPManager manager;
    manager.stopNTRIP();
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);
}

void NTRIPManagerTest::testStopCancelsDeferredSettings_data()
{
    QTest::addColumn<bool>("shutdown");
    QTest::newRow("restartable-stop") << false;
    QTest::newRow("permanent-shutdown") << true;
}

void NTRIPManagerTest::testStopCancelsDeferredSettings()
{
    QFETCH(bool, shutdown);
    ManualScheduler scheduler;
    NTRIPManager manager(nullptr, &scheduler);
    auto* first = injectTransport(manager, true);
    initialize(manager);
    QCOMPARE(first->startCount, 1);
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Connected);
    auto changed = manager.configuration();
    changed.stream.connection.mountpoint = QStringLiteral("CHANGED");
    manager.setConfiguration(changed);
    auto* replacement = injectTransport(manager, true);
    if (shutdown) {
        manager.shutdown();
    } else {
        manager.stopNTRIP();
    }
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);
    QVERIFY(scheduler.advanceBy(SettingsDebounce));
    QCOMPARE(replacement->startCount, 0);
    if (shutdown) {
        changed.stream.connection.mountpoint = QStringLiteral("AFTER_SHUTDOWN");
        manager.setConfiguration(changed);
        QVERIFY(scheduler.advanceBy(SettingsDebounce));
        QCOMPARE(replacement->startCount, 0);
    }
    manager.startNTRIP();
    QCOMPARE(replacement->startCount, shutdown ? 0 : 1);
}

void NTRIPManagerTest::testNewSessionRetryBudget_data()
{
    QTest::addColumn<int>("action");
    QTest::newRow("explicit-retry") << 0;
    QTest::newRow("disable-enable") << 1;
    QTest::newRow("automatic-reconnect") << 2;
    QTest::newRow("qml-retry-enables-connection") << 3;
}

void NTRIPManagerTest::testNewSessionRetryBudget()
{
    QFETCH(int, action);
    ManualScheduler scheduler;
    NTRIPManager manager(nullptr, &scheduler);
    auto* transport = injectTransport(manager);
    manager.setConfiguration(testConfiguration());
    manager.init();

    const auto failWithExpectedDelay = [&](std::chrono::milliseconds expectedDelay) {
        expectLogMessage("GPS.NTRIP.NTRIPManager", QtWarningMsg,
                         QRegularExpression(QStringLiteral("NTRIP error:.*retry budget")));
        transport->simulateError(NTRIPError::SocketError, QStringLiteral("retry budget"));
        deliverPostedEvents();
        verifyExpectedLogMessage();
        QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Reconnecting);
        QVERIFY(manager.statusMessage().contains(QString::number(expectedDelay.count() / 1000)));
        transport = injectTransport(manager);
        if (expectedDelay > std::chrono::milliseconds(1)) {
            QVERIFY(scheduler.advanceBy(expectedDelay - std::chrono::milliseconds(1)));
            QCOMPARE(transport->startCount, 0);
        }
        QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
        QCOMPARE(transport->startCount, 1);
        QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Connecting);
    };

    if (action == 1) {
        manager.setConfiguration(testConfiguration(false));
        QVERIFY(scheduler.advanceBy(SettingsDebounce));
        transport = injectTransport(manager);
        manager.setConfiguration(testConfiguration(true));
        QVERIFY(scheduler.advanceBy(SettingsDebounce));
    } else if (action == 2) {
        for (const auto delay : {std::chrono::seconds(1), std::chrono::seconds(2), std::chrono::seconds(4)}) {
            failWithExpectedDelay(delay);
        }
    } else if (action == 3) {
        expectLogMessage("GPS.NTRIP.NTRIPManager", QtWarningMsg,
                         QRegularExpression(QStringLiteral("NTRIP error:.*authentication")));
        transport->simulateError(NTRIPError::AuthFailed, QStringLiteral("authentication"));
        deliverPostedEvents();
        verifyExpectedLogMessage();
        QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Error);
        manager.setConfiguration(testConfiguration(false));
        transport = injectTransport(manager);
        QVERIFY(manager.metaObject()->indexOfMethod("retryNTRIP()") >= 0);
        QSignalSpy enableRequested(&manager, &NTRIPManager::enableRequested);
        manager.retryNTRIP();
        QCOMPARE(enableRequested.count(), 1);
    } else {
        manager.startNTRIP();
    }
    QCOMPARE(transport->startCount, 1);
    QVERIFY(manager.configuration().enabled);
    const auto expectedDelay = action == 2 ? std::chrono::seconds(8) : std::chrono::seconds(1);
    expectLogMessage("GPS.NTRIP.NTRIPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("NTRIP error:.*retry budget")));
    transport->simulateError(NTRIPError::SocketError, QStringLiteral("retry budget"));
    deliverPostedEvents();
    verifyExpectedLogMessage();
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Reconnecting);
    auto* retry = injectTransport(manager);
    QVERIFY(scheduler.advanceBy(expectedDelay - std::chrono::milliseconds(1)));
    QCOMPARE(retry->startCount, 0);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QCOMPARE(retry->startCount, 1);
}

void NTRIPManagerTest::testStatusCallbackStopsTransition()
{
    NTRIPManager manager;
    manager.setConfiguration(testConfiguration());
    auto* transport = new MockNTRIPTransport(&manager);
    transport->autoConnect = false;
    manager.setTransportForTest(transport);
    QList<NTRIPManager::ConnectionStatus> observed;
    connect(&manager, &NTRIPManager::connectionStatusChanged, this, [&]() {
        observed.append(manager.connectionStatus());
        if (manager.connectionStatus() == NTRIPManager::ConnectionStatus::Connecting) {
            manager.stopNTRIP();
        }
    });
    manager.startNTRIP();
    // Observers see the settled state after the transport has started, never a half-entered state.
    QCOMPARE(observed, (QList<NTRIPManager::ConnectionStatus>{NTRIPManager::ConnectionStatus::Connecting,
                                                              NTRIPManager::ConnectionStatus::Disconnected}));
    QCOMPARE(transport->startCount, 1);
    QCOMPARE(transport->stopCount, 1);
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);
}

void NTRIPManagerTest::testConnectedCallbackStopsTransition()
{
    NTRIPManager manager;
    auto* transport = injectTransport(manager);
    initialize(manager);
    connect(&manager, &NTRIPManager::connectionStatusChanged, this, [&]() {
        if (manager.connectionStatus() == NTRIPManager::ConnectionStatus::Connected) {
            manager.stopNTRIP();
        }
    });
    transport->simulateConnect();
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);
    QCOMPARE(transport->stopCount, 1);
    QVERIFY(manager.ggaSource().isEmpty());
}

void NTRIPManagerTest::testPlaintextCredentialWarningIsVisibleState()
{
    NTRIPManager mgr;
    auto* transport = injectTransport(mgr);
    initialize(mgr);
    QSignalSpy warningSpy(&mgr, &NTRIPManager::securityWarningChanged);

    QVERIFY(mgr.securityWarning().isEmpty());
    transport->simulatePlaintextWarning();

    QCOMPARE(warningSpy.count(), 1);
    QVERIFY(mgr.securityWarning().contains(QStringLiteral("without TLS")));

    transport->simulatePlaintextWarning();
    QCOMPARE(warningSpy.count(), 1);
}

// ---------------------------------------------------------------------------
// Reconnect backoff (migrated from NTRIPReconnectPolicyTest)
// ---------------------------------------------------------------------------

void NTRIPManagerTest::testReconnectInitialBackoff()
{
    ManualScheduler scheduler;
    NTRIPManager mgr(nullptr, &scheduler);
    auto* transport = injectTransport(mgr);
    initialize(mgr);
    expectLogMessage("GPS.NTRIP.NTRIPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("NTRIP error:.*initial")));
    transport->simulateError(NTRIPError::SocketError, QStringLiteral("initial"));
    deliverPostedEvents();
    verifyExpectedLogMessage();
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Reconnecting);
    QVERIFY(mgr.statusMessage().contains(QStringLiteral("1s")));
    auto* retry = injectTransport(mgr);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(999)));
    QCOMPARE(retry->startCount, 0);
    QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
    QCOMPARE(retry->startCount, 1);
}

void NTRIPManagerTest::testReconnectExponentialBackoff()
{
    ManualScheduler scheduler;
    NTRIPManager mgr(nullptr, &scheduler);
    auto* transport = injectTransport(mgr);
    initialize(mgr);

    for (const int seconds : {1, 2, 4, 8}) {
        expectLogMessage("GPS.NTRIP.NTRIPManager", QtWarningMsg,
                         QRegularExpression(QStringLiteral("NTRIP error:.*backoff")));
        transport->simulateError(NTRIPError::SocketError, QStringLiteral("backoff"));
        deliverPostedEvents();
        verifyExpectedLogMessage();
        QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Reconnecting);
        QVERIFY(mgr.statusMessage().contains(QStringLiteral("%1s").arg(seconds)));
        transport = injectTransport(mgr);
        QVERIFY(scheduler.advanceBy(std::chrono::seconds(seconds) - std::chrono::milliseconds(1)));
        QCOMPARE(transport->startCount, 0);
        QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
        QCOMPARE(transport->startCount, 1);
        QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Connecting);
    }
}

void NTRIPManagerTest::testReconnectMaxBackoff()
{
    ManualScheduler scheduler;
    NTRIPManager mgr(nullptr, &scheduler);
    auto* transport = injectTransport(mgr);
    initialize(mgr);

    for (const int seconds : {1, 2, 4, 8, 16, 30, 30}) {
        expectLogMessage("GPS.NTRIP.NTRIPManager", QtWarningMsg,
                         QRegularExpression(QStringLiteral("NTRIP error:.*capped")));
        transport->simulateError(NTRIPError::SocketError, QStringLiteral("capped"));
        deliverPostedEvents();
        verifyExpectedLogMessage();
        QVERIFY(mgr.statusMessage().contains(QStringLiteral("%1s").arg(seconds)));
        transport = injectTransport(mgr);
        QVERIFY(scheduler.advanceBy(std::chrono::seconds(seconds)));
        QCOMPARE(transport->startCount, 1);
    }
}

void NTRIPManagerTest::testReconnectCancelStopsTimer()
{
    ManualScheduler scheduler;
    NTRIPManager mgr(nullptr, &scheduler);
    auto* transport = injectTransport(mgr);
    initialize(mgr);
    expectLogMessage("GPS.NTRIP.NTRIPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("NTRIP error:.*cancel")));
    transport->simulateError(NTRIPError::SocketError, QStringLiteral("cancel"));
    deliverPostedEvents();
    verifyExpectedLogMessage();
    auto* retry = injectTransport(mgr);
    mgr.stopNTRIP();
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(1)));
    QCOMPARE(retry->startCount, 0);
}

void NTRIPManagerTest::testReconnectResetAttempts()
{
    ManualScheduler scheduler;
    NTRIPManager mgr(nullptr, &scheduler);
    auto* transport = injectTransport(mgr);
    initialize(mgr);
    for (const int seconds : {1, 2}) {
        expectLogMessage("GPS.NTRIP.NTRIPManager", QtWarningMsg,
                         QRegularExpression(QStringLiteral("NTRIP error:.*reset")));
        transport->simulateError(NTRIPError::SocketError, QStringLiteral("reset"));
        deliverPostedEvents();
        verifyExpectedLogMessage();
        QVERIFY(mgr.statusMessage().contains(QStringLiteral("%1s").arg(seconds)));
        transport = injectTransport(mgr);
        QVERIFY(scheduler.advanceBy(std::chrono::seconds(seconds)));
        QCOMPARE(transport->startCount, 1);
    }

    mgr.stopNTRIP();
    transport = injectTransport(mgr);
    mgr.startNTRIP();
    QCOMPARE(transport->startCount, 1);
    expectLogMessage("GPS.NTRIP.NTRIPManager", QtWarningMsg, QRegularExpression(QStringLiteral("NTRIP error:.*fresh")));
    transport->simulateError(NTRIPError::SocketError, QStringLiteral("fresh"));
    deliverPostedEvents();
    verifyExpectedLogMessage();
    QVERIFY(mgr.statusMessage().contains(QStringLiteral("1s")));
}

void NTRIPManagerTest::testReconnectSignalFires()
{
    ManualScheduler scheduler;
    NTRIPManager mgr(nullptr, &scheduler);
    auto* transport = injectTransport(mgr);
    initialize(mgr);
    expectLogMessage("GPS.NTRIP.NTRIPManager", QtWarningMsg, QRegularExpression(QStringLiteral("NTRIP error:.*timer")));
    transport->simulateError(NTRIPError::SocketError, QStringLiteral("timer"));
    deliverPostedEvents();
    verifyExpectedLogMessage();
    auto* retry = injectTransport(mgr);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(1)));
    QCOMPARE(retry->startCount, 1);
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Connecting);
}

UT_REGISTER_TEST(NTRIPManagerTest, TestLabel::Unit)

void NTRIPManagerTest::testDuplicateTransportErrorsScheduleOneRetry()
{
    ManualScheduler scheduler;
    NTRIPManager mgr(nullptr, &scheduler);
    auto* transport = injectTransport(mgr);
    initialize(mgr);
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Connecting);
    expectLogMessage("GPS.NTRIP.NTRIPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("NTRIP error:.*first failure")));
    transport->simulateError(NTRIPError::SocketError, QStringLiteral("first failure"));
    transport->simulateError(NTRIPError::ServerDisconnected, QStringLiteral("duplicate failure"));
    deliverPostedEvents();
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Reconnecting);
    QVERIFY(mgr.statusMessage().contains(QStringLiteral("first failure")));
    QVERIFY(!mgr.statusMessage().contains(QStringLiteral("duplicate failure")));
    verifyExpectedLogMessage();
    auto* retry = injectTransport(mgr);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(1)));
    QCOMPARE(retry->startCount, 1);
}

void NTRIPManagerTest::testRetiredTransportErrorCannotAffectNewSession()
{
    ManualScheduler scheduler;
    NTRIPManager mgr(nullptr, &scheduler);
    auto* first = injectTransport(mgr);
    initialize(mgr);
    first->simulateError(NTRIPError::HttpError, QStringLiteral("retired failure"), std::chrono::seconds{300});
    mgr.stopNTRIP();
    auto* second = injectTransport(mgr);
    mgr.startNTRIP();
    deliverPostedEvents();
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Connecting);
    QCOMPARE(second->startCount, 1);
    auto* unexpected = injectTransport(mgr);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(300)));
    QCOMPARE(unexpected->startCount, 0);
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Connecting);
}

void NTRIPManagerTest::testRetryPolicy_data()
{
    QTest::addColumn<qint64>("retryAfterMs");
    QTest::addColumn<int>("attempts");
    QTest::addColumn<bool>("enabled");
    QTest::addColumn<NTRIPError>("code");
    QTest::addColumn<int>("expectedDelayMs");
    QTest::newRow("initial-backoff") << qint64(0) << 0 << true << NTRIPError::HttpError << 1000;
    QTest::newRow("exponential-backoff") << qint64(0) << 4 << true << NTRIPError::HttpError << 16000;
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
}

void NTRIPManagerTest::testRetryPolicy()
{
    QFETCH(qint64, retryAfterMs);
    QFETCH(int, attempts);
    QFETCH(bool, enabled);
    QFETCH(NTRIPError, code);
    QFETCH(int, expectedDelayMs);
    ManualScheduler scheduler;
    NTRIPManager manager(nullptr, &scheduler);
    manager.setConfiguration(testConfiguration(enabled));
    auto* transport = injectTransport(manager);
    manager.startNTRIP();

    for (int attempt = 0; attempt < attempts; ++attempt) {
        const auto warmupDelay = std::chrono::milliseconds(1000 * (1 << attempt));
        expectLogMessage("GPS.NTRIP.NTRIPManager", QtWarningMsg,
                         QRegularExpression(QStringLiteral("NTRIP error:.*warmup")));
        transport->simulateError(NTRIPError::SocketError, QStringLiteral("warmup"));
        deliverPostedEvents();
        verifyExpectedLogMessage();
        transport = injectTransport(manager);
        QVERIFY(scheduler.advanceBy(warmupDelay));
        QCOMPARE(transport->startCount, 1);
    }

    expectLogMessage("GPS.NTRIP.NTRIPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("NTRIP error:.*retry test")));
    transport->simulateError(code, QStringLiteral("retry test"), std::chrono::milliseconds{retryAfterMs});
    deliverPostedEvents();
    verifyExpectedLogMessage();
    QCOMPARE(manager.connectionStatus(),
             expectedDelayMs ? NTRIPManager::ConnectionStatus::Reconnecting : NTRIPManager::ConnectionStatus::Error);
    if (expectedDelayMs) {
        QVERIFY(manager.statusMessage().contains(QString::number(expectedDelayMs / 1000)));
        auto* retry = injectTransport(manager);
        QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(expectedDelayMs) - std::chrono::milliseconds(1)));
        QCOMPARE(retry->startCount, 0);
        QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
        QCOMPARE(retry->startCount, 1);
    }
    manager.stopNTRIP();
    manager.setConfiguration(testConfiguration(true));
    auto* replacement = injectTransport(manager, true);
    manager.startNTRIP();
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Connected);
    expectLogMessage("GPS.NTRIP.NTRIPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("NTRIP error:.*fresh failure")));
    replacement->simulateError(NTRIPError::SocketError, QStringLiteral("fresh failure"));
    deliverPostedEvents();
    verifyExpectedLogMessage();
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Reconnecting);
    QVERIFY(manager.statusMessage().contains(QStringLiteral("1s")));
}

void NTRIPManagerTest::testHttpRetryAfterReachesManager_data()
{
    QTest::addColumn<int>("status");
    QTest::addColumn<bool>("compressed");
    QTest::newRow("plain-retry") << 503 << false;
    QTest::newRow("compressed-retry") << 503 << true;
    QTest::newRow("compressed-authentication") << 401 << true;
}

void NTRIPManagerTest::testHttpRetryAfterReachesManager()
{
    QFETCH(int, status);
    QFETCH(bool, compressed);
    ScriptedNtripCaster caster;
    QVERIFY(caster.isListening());
    ManualScheduler scheduler;
    NTRIPManager manager(nullptr, &scheduler);
    auto configuration = testConfiguration();
    configuration.stream.connection.host = QStringLiteral("127.0.0.1");
    configuration.stream.connection.port = caster.port();
    manager.setConfiguration(configuration);
    manager.startNTRIP();
    auto* connection = caster.waitForConnection(TestTimeout::shortMs());
    QVERIFY(connection && connection->peer);
    QVERIFY(connection->waitForRequest(TestTimeout::shortMs()).startsWith("GET /TEST HTTP/1.1"));
    expectLogMessage("GPS.NTRIP.NTRIPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("NTRIP error:.*%1").arg(status)));
    const QByteArray body = compressed ? QByteArray::fromHex("1f8b080000000000000303000000000000000000") : QByteArray();
    const QByteArray encoding = compressed ? QByteArray("Content-Encoding: gzip\r\n") : QByteArray();
    const QByteArray response = "HTTP/1.1 " + QByteArray::number(status) + " Error\r\nRetry-After: 17\r\n" + encoding +
                                "Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body;
    QCOMPARE(connection->write(response), response.size());
    const auto expectedState =
        status == 401 ? NTRIPManager::ConnectionStatus::Error : NTRIPManager::ConnectionStatus::Reconnecting;
    QTRY_COMPARE_WITH_TIMEOUT(manager.connectionStatus(), expectedState, TestTimeout::shortMs());
    verifyExpectedLogMessage();
    if (status != 401) {
        auto* retry = injectTransport(manager);
        QVERIFY(scheduler.advanceBy(std::chrono::seconds(17) - std::chrono::milliseconds(1)));
        QCOMPARE(retry->startCount, 0);
        QVERIFY(scheduler.advanceBy(std::chrono::milliseconds(1)));
        QCOMPARE(retry->startCount, 1);
    } else {
        auto* retry = injectTransport(manager);
        QVERIFY(scheduler.advanceBy(std::chrono::seconds(17)));
        QCOMPARE(retry->startCount, 0);
    }
}

void NTRIPManagerTest::testRetryPublicationSuperseded_data()
{
    QTest::addColumn<int>("phase");
    QTest::newRow("reconnecting-status") << 1;
    QTest::newRow("transport-stop") << 2;
}

void NTRIPManagerTest::testRetryPublicationSuperseded()
{
    QFETCH(int, phase);
    ManualScheduler scheduler;
    NTRIPManager manager(nullptr, &scheduler);
    manager.setConfiguration(testConfiguration());
    auto* first = injectTransport(manager, true);
    manager.startNTRIP();
    auto* replacement = injectTransport(manager);
    bool replaced = false;
    const auto restart = [&]() {
        if (std::exchange(replaced, true)) {
            return;
        }
        manager.stopNTRIP();
        manager.setTransportForTest(replacement);
        manager.startNTRIP();
    };
    if (phase == 1) {
        connect(&manager, &NTRIPManager::connectionStatusChanged, this, [&]() {
            if (manager.connectionStatus() == NTRIPManager::ConnectionStatus::Reconnecting) {
                restart();
            }
        });
    } else {
        first->onStop = restart;
    }
    expectLogMessage("GPS.NTRIP.NTRIPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("NTRIP error:.*superseded")));
    first->simulateError(NTRIPError::HttpError, QStringLiteral("superseded"), std::chrono::seconds(300));
    deliverPostedEvents();
    verifyExpectedLogMessage();
    QVERIFY(replaced);
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Connecting);
    QCOMPARE(replacement->startCount, 1);
    auto* unexpected = injectTransport(manager);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds(300)));
    QCOMPARE(unexpected->startCount, 0);
}

void NTRIPManagerTest::testMissingMountpointDoesNotStartTransport()
{
    NTRIPManager mgr;
    auto configuration = testConfiguration();
    configuration.stream.connection.mountpoint.clear();
    mgr.setConfiguration(configuration);
    auto* transport = new MockNTRIPTransport(&mgr);
    mgr.setTransportForTest(transport);
    expectLogMessage("GPS.NTRIP.NTRIPManager", QtWarningMsg, QRegularExpression(QStringLiteral("Select a mountpoint")));
    mgr.startNTRIP();
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Error);
    QCOMPARE(transport->startCount, 0);
    QVERIFY(mgr.statusMessage().contains(QStringLiteral("mountpoint")));
    verifyExpectedLogMessage();
}

void NTRIPManagerTest::testCorrectionIngressKeepsSessionAndIdentity()
{
    GPSCorrectionManager corrections;
    corrections.rtcmMavlink()->setOutputProvider([]() {
        return QList<RTCMMavlink::Output>{{QStringLiteral("test"), 1, [](const GpsRtcmPacket&) { return true; }}};
    });
    NTRIPManager mgr;
    auto configuration = testConfiguration(false);
    configuration.stream.connection.username = QStringLiteral("private-user");
    configuration.stream.connection.password = QStringLiteral("private-password");
    configuration.stream.connection.useTls = true;
    mgr.setConfiguration(configuration);
    mgr.setCorrectionManager(&corrections);
    QCOMPARE(mgr.metaObject()->indexOfProperty("rtcmMavlink"), -1);
    QSignalSpy routed(&corrections.router(), &GPSCorrectionRouter::frameRouted);
    auto* first = new MockNTRIPTransport(&mgr);
    first->autoConnect = false;
    QSignalSpy observed(first, &NTRIPTransport::correctionFrameReceived);
    mgr.setTransportForTest(first);
    mgr.init();
    QCOMPARE(first->startCount, 0);
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);
    configuration.enabled = true;
    mgr.setConfiguration(configuration);
    QTRY_COMPARE_WITH_TIMEOUT(first->startCount, 1, TestTimeout::shortMs());
    const QByteArray frame = GpsTestHelpers::buildRtcmFrame(1005);
    const qint64 receivedAtMs = GPSCorrectionFrame::monotonicNowMs() - 10;
    first->simulateRtcmData(frame, 1005, receivedAtMs);
    QCOMPARE(observed.size(), 1);
    const auto result = qvariant_cast<RTCMDecodedFrame>(observed[0][0]);
    QCOMPARE(result.data, frame);
    QCOMPARE(result.messageId, 1005);
    QCOMPARE(result.receivedAtMs, receivedAtMs);
    QVERIFY(result.valid && !result.filtered);
    QVERIFY(routed.isEmpty());
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QCOMPARE(routed.size(), 1);
    const auto original = qvariant_cast<GPSCorrectionFrame>(routed[0][0]);
    QCOMPARE(original.source, GPSCorrectionSource::Ntrip);
    QCOMPARE(original.sourceInstance, QStringLiteral("ntrips://caster.example.com:2101/TEST"));
    QCOMPARE(original.receivedAtMs, receivedAtMs);
    QVERIFY(original.validated);
    QCOMPARE(corrections.rtcmMavlink()->totalBytesSent(), quint64(frame.size()));
    QCOMPARE(corrections.rtcmMavlink()->totalBytesSubmitted(), quint64(frame.size()));

    first->simulateRtcmData(frame, 1005);
    mgr.stopNTRIP();
    QVERIFY(corrections.sourceInstances().isEmpty());
    auto* second = new MockNTRIPTransport(&mgr);
    second->autoConnect = false;
    mgr.setTransportForTest(second);
    mgr.startNTRIP();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Connecting);
    QCOMPARE(routed.size(), 1);
    QCOMPARE(corrections.rtcmMavlink()->totalBytesSent(), quint64(frame.size()));
    const qint64 expiredAgeMs = GPSCorrectionRouter::FRESHNESS_TIMEOUT_MS + 6000;
    const qint64 expiredAtMs = static_cast<qint64>(MonotonicClock::nowUs() / 1000) - expiredAgeMs;
    second->simulateRtcmData(frame, 1005, expiredAtMs);
    QCOMPARE(mgr.connectionStats()->messagesReceived(), quint32(0));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QCOMPARE(routed.size(), 1);
    QCOMPARE(corrections.rtcmMavlink()->totalBytesSubmitted(), quint64(frame.size()));
    QCOMPARE(mgr.connectionStats()->messagesReceived(), quint32(1));
    QCOMPARE(mgr.connectionStats()->bytesReceived(), quint64(frame.size()));
    QVERIFY(mgr.connectionStats()->correctionAgeSec() >= expiredAgeMs / 1000.0);
    QVERIFY(mgr.connectionStats()->dataStale());

    second->simulateRtcmData(frame, 1005, expiredAtMs - 1000);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QCOMPARE(mgr.connectionStats()->messagesReceived(), quint32(2));
    QVERIFY(mgr.connectionStats()->dataStale());

    second->simulateRtcmData(frame, 1005);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QCOMPARE(routed.size(), 2);
    const auto replacement = qvariant_cast<GPSCorrectionFrame>(routed[1][0]);
    QVERIFY(replacement.session != original.session);
    QCOMPARE(replacement.sourceInstance, original.sourceInstance);
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Connected);
    QCOMPARE(corrections.rtcmMavlink()->totalBytesSubmitted(), quint64(2 * frame.size()));
    QCOMPARE(mgr.connectionStats()->messagesReceived(), quint32(3));
    QVERIFY(mgr.connectionStats()->correctionAgeSec() < 1.0);
    QVERIFY(!mgr.connectionStats()->dataStale());

    second->simulateRtcmData(frame, 1005, expiredAtMs);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QCOMPARE(routed.size(), 2);
    QCOMPARE(mgr.connectionStats()->messagesReceived(), quint32(4));
    QCOMPARE(mgr.connectionStats()->bytesReceived(), quint64(4 * frame.size()));
    QVERIFY(mgr.connectionStats()->correctionAgeSec() < 1.0);
    QVERIFY(!mgr.connectionStats()->dataStale());
}

void NTRIPManagerTest::testFactChangesReconfigureTransport_data()
{
    QTest::addColumn<QString>("setting");
    QTest::newRow("cold-whitelist") << QStringLiteral("whitelist");
    QTest::newRow("hot-mountpoint") << QStringLiteral("mountpoint");
}

void NTRIPManagerTest::testGgaSettingsUseInjectedProviders()
{
    using Source = NTRIPGgaProvider::PositionSource;
    ManualScheduler scheduler;
    NTRIPManager manager(nullptr, &scheduler);
    auto configuration = testConfiguration();
    configuration.gga = {Source::VehicleGPS, std::chrono::seconds{60}};
    manager.setConfiguration(configuration);
    manager.setGgaPositionProvider(Source::VehicleGPS, []() {
        return PositionResult{QGeoCoordinate(47, 8, 500), QStringLiteral("Injected vehicle"),
                              GPSAltitudeDatum::MeanSeaLevel};
    });
    manager.setGgaPositionProvider(Source::GCSPosition, []() {
        return PositionResult{QGeoCoordinate(48, 9, 600), QStringLiteral("Injected GCS"),
                              GPSAltitudeDatum::MeanSeaLevel};
    });
    auto* transport = new MockNTRIPTransport(&manager);
    manager.setTransportForTest(transport);
    manager.init();
    QCOMPARE(manager.ggaSource(), QStringLiteral("Injected vehicle"));
    QCOMPARE(transport->sentNmea.size(), 1);
    QSignalSpy transitions(&manager, &NTRIPManager::connectionStatusChanged);

    expectLogMessage("GPS.NTRIP.NTRIPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Inject GGA position providers before initializing NTRIP")));
    manager.setGgaPositionProvider(Source::GCSPosition, []() { return PositionResult{}; });
    verifyExpectedLogMessage();
    configuration.gga = {Source::GCSPosition, std::chrono::seconds{1}};
    manager.setConfiguration(configuration);
    QVERIFY(scheduler.advanceBy(std::chrono::seconds{1}));
    QCOMPARE(manager.ggaSource(), QStringLiteral("Injected GCS"));
    QVERIFY(transport->sentNmea.size() >= 2);
    QCOMPARE(transport->startCount, 1);
    QCOMPARE(transport->stopCount, 0);
    QVERIFY(transitions.isEmpty());
    manager.stopNTRIP();
}

void NTRIPManagerTest::testFactChangesReconfigureTransport()
{
    QFETCH(QString, setting);
    ManualScheduler scheduler;
    GPSCorrectionManager corrections(nullptr, &scheduler);
    NTRIPManager mgr(nullptr, &scheduler);
    auto configuration = testConfiguration();
    configuration.stream.filter.whitelist = QStringLiteral("1005");
    mgr.setConfiguration(configuration);
    mgr.setCorrectionManager(&corrections);
    auto* first = new MockNTRIPTransport(&mgr);
    int stops = 0;
    first->onStop = [&]() { ++stops; };
    mgr.setTransportForTest(first);
    mgr.init();
    QCOMPARE(first->startCount, 1);
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Connected);
    QSignalSpy routed(&corrections.router(), &GPSCorrectionRouter::frameRouted);
    first->simulateRtcmData(GpsTestHelpers::buildRtcmFrame(1005), 1005);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QCOMPARE(routed.size(), 1);
    const auto original = qvariant_cast<GPSCorrectionFrame>(routed[0][0]);

    auto* second = new MockNTRIPTransport(&mgr);
    mgr.setTransportForTest(second);
    const bool reconnect = setting == QStringLiteral("mountpoint");
    auto changed = configuration;
    if (reconnect) {
        changed.stream.connection.mountpoint = QStringLiteral("REPLACEMENT");
    } else {
        changed.stream.filter.whitelist = QStringLiteral("1077");
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
        active->simulateRtcmData(GpsTestHelpers::buildRtcmFrame(1005), 1005);
    }
    const auto frame = GpsTestHelpers::buildRtcmFrame(1077);
    const qint64 receivedAtMs = GPSCorrectionFrame::monotonicNowMs() - 10;
    active->simulateRtcmData(frame, 1077, receivedAtMs);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QCOMPARE(routed.size(), 2);
    const auto current = qvariant_cast<GPSCorrectionFrame>(routed[1][0]);
    QCOMPARE(current.session != original.session, reconnect);
    QCOMPARE(current.sourceInstance,
             reconnect ? QStringLiteral("ntrip://caster.example.com:2101/REPLACEMENT") : original.sourceInstance);
    QCOMPARE(current.receivedAtMs, receivedAtMs);
    changed.enabled = false;
    mgr.setConfiguration(changed);
    mgr.setConfiguration(changed);
    QVERIFY(scheduler.advanceBy(SettingsDebounce));
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);
    QVERIFY(corrections.sourceInstances().isEmpty());
}

void NTRIPManagerTest::testTransportDiagnosticsReachManager()
{
    ScriptedNtripCaster caster;
    QVERIFY(caster.isListening());
    GPSCorrectionManager corrections;
    NTRIPManager mgr;
    auto configuration = testConfiguration();
    configuration.stream.connection.host = QStringLiteral("127.0.0.1");
    configuration.stream.connection.port = caster.port();
    configuration.stream.filter.whitelist = QStringLiteral("1005");
    mgr.setConfiguration(configuration);
    mgr.setCorrectionManager(&corrections);
    QSignalSpy routed(&corrections.router(), &GPSCorrectionRouter::frameRouted);
    mgr.startNTRIP();
    auto* connection = caster.waitForConnection(TestTimeout::shortMs());
    QVERIFY(connection && connection->peer);
    QVERIFY(connection->waitForRequest(TestTimeout::shortMs()).startsWith("GET /TEST HTTP/1.1"));
    const auto accepted = GpsTestHelpers::buildRtcmFrame(1005);
    const auto filtered = GpsTestHelpers::buildRtcmFrame(1077);
    auto rejected = GpsTestHelpers::buildRtcmFrame(1087);
    rejected.back() ^= 1;
    expectLogMessage("GPS.NTRIP.NTRIPHttpTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Invalid RTCM frame")));
    const QByteArray body = accepted + filtered + rejected;
    const QByteArray response = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n" +
                                QByteArray::number(body.size(), 16) + "\r\n" + body + "\r\n";
    QCOMPARE(connection->write(response), response.size());
    QTRY_COMPARE_WITH_TIMEOUT(
        corrections.sourceDiagnostics()[static_cast<int>(GPSCorrectionSource::Ntrip)].receivedFrames, 3,
        TestTimeout::shortMs());
    verifyExpectedLogMessage();
    const auto stats = corrections.sourceDiagnostics()[static_cast<int>(GPSCorrectionSource::Ntrip)];
    QCOMPARE(stats.validatedFrames, 2);
    QCOMPARE(stats.selectedFrames, 1);
    QCOMPARE(routed.size(), 1);
    QCOMPARE(corrections.rtcmMavlink()->totalBytesSent(), quint64(accepted.size()));
    mgr.stopNTRIP();
}
