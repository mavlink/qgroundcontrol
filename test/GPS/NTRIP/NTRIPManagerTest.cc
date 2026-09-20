#include "NTRIPManagerTest.h"

#include <limits>
#include <utility>

#include <QtCore/QChronoTimer>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QNetworkDatagram>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QUdpSocket>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "Fixtures/RAIIFixtures.h"
#include "GPSCorrectionManager.h"
#include "GpsTestHelpers.h"
#include "MockNTRIPTransport.h"
#include "MonotonicClock.h"
#include "NTRIPManager.h"
#include "NTRIPSettings.h"
#include "RTCMDecodedFrame.h"
#include "SettingsManager.h"

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

void NTRIPManagerTest::testStatusCallbackStopsTransition()
{
    NTRIPManager manager;
    auto* transport = new MockNTRIPTransport(&manager);
    manager.setTransportForTest(transport);
    connect(&manager, &NTRIPManager::connectionStatusChanged, this, [&]() {
        if (manager.connectionStatus() == NTRIPManager::ConnectionStatus::Connecting) {
            manager.stopNTRIP();
        }
    });
    manager.startNTRIP();
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);
    QCOMPARE(transport->startCount, 0);
    QVERIFY(!manager._transport);
}

void NTRIPManagerTest::testCasterCallbackStopsTransition()
{
    NTRIPManager manager;
    auto* transport = new MockNTRIPTransport(&manager);
    manager._transport = transport;
    manager._connectionStatus = NTRIPManager::ConnectionStatus::Connecting;
    connect(&manager, &NTRIPManager::casterStatusChanged, this, [&]() { manager.stopNTRIP(); });
    manager._dispatch(NTRIPManager::Event::TransportConnected);
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);
    QCOMPARE(transport->stopCount, 1);
    QVERIFY(manager.ggaSource().isEmpty());
    QVERIFY(!manager._transport);
}

void NTRIPManagerTest::testPlaintextCredentialWarningIsVisibleState()
{
    NTRIPManager mgr;
    QSignalSpy warningSpy(&mgr, &NTRIPManager::securityWarningChanged);

    QVERIFY(mgr.securityWarning().isEmpty());
    mgr._onPlaintextCredentialsWarning();

    QCOMPARE(warningSpy.count(), 1);
    QVERIFY(mgr.securityWarning().contains(QStringLiteral("without TLS")));

    mgr._onPlaintextCredentialsWarning();
    QCOMPARE(warningSpy.count(), 1);
}

void NTRIPManagerTest::testTerminalStateStopsUdpForwarder_data()
{
    QTest::addColumn<NTRIPManager::ConnectionStatus>("status");
    QTest::newRow("disconnected") << NTRIPManager::ConnectionStatus::Disconnected;
    QTest::newRow("error") << NTRIPManager::ConnectionStatus::Error;
}

void NTRIPManagerTest::testTerminalStateStopsUdpForwarder()
{
    QFETCH(NTRIPManager::ConnectionStatus, status);
    GPSCorrectionManager corrections;
    NTRIPManager mgr;
    mgr.setCorrectionManager(&corrections);
    QUdpSocket listener;
    QVERIFY(listener.bind(QHostAddress(QHostAddress::LocalHost), 0));

    NTRIPUdpForwardConfig config;
    config.enabled = true;
    config.address = QStringLiteral("127.0.0.1");
    config.port = listener.localPort();

    mgr._applyUdpForwarderConfig(config);
    auto source = corrections.registerSource(GPSCorrectionSource::Ntrip);
    const auto frame = GpsTestHelpers::buildRtcmFrame(1005);
    corrections.acceptIngress(source.token().event(frame, GPSCorrectionFrame::monotonicNowMs(), 1005, true));
    QTRY_VERIFY_WITH_TIMEOUT(listener.hasPendingDatagrams(), TestTimeout::shortMs());
    QCOMPARE(listener.receiveDatagram().data(), frame);

    mgr._enterState(status, QStringLiteral("session ended"));
    QCOMPARE(mgr.connectionStatus(), status);
    corrections.acceptIngress(source.token().event(frame, GPSCorrectionFrame::monotonicNowMs(), 1005, true));
    QVERIFY(!listener.waitForReadyRead(TestTimeout::shortMs()));
    QVERIFY(!listener.hasPendingDatagrams());
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

void NTRIPManagerTest::testDuplicateTransportErrorsScheduleOneRetry()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->ntripSettings();
    saved.setFactValue(settings->ntripServerHostAddress(), QStringLiteral("caster.example.com"));
    saved.setFactValue(settings->ntripMountpoint(), QStringLiteral("TEST"));
    saved.setFactValue(settings->ntripServerConnectEnabled(), true);
    NTRIPManager mgr;
    mgr._settings = settings;
    auto* transport = new MockNTRIPTransport(&mgr);
    transport->autoConnect = false;
    mgr.setTransportForTest(transport);
    mgr.startNTRIP();
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Connecting);
    expectLogMessage("GPS.NTRIPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("NTRIP error:.*first failure")));
    transport->simulateError(NTRIPError::SocketError, QStringLiteral("first failure"));
    transport->simulateError(NTRIPError::ServerDisconnected, QStringLiteral("duplicate failure"));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Reconnecting);
    QCOMPARE(mgr._reconnectAttempts, 1);
    QCOMPARE(mgr._reconnectTimer.interval(), std::chrono::milliseconds(1000));
    QVERIFY(mgr.statusMessage().contains(QStringLiteral("first failure")));
    QVERIFY(!mgr.statusMessage().contains(QStringLiteral("duplicate failure")));
    verifyExpectedLogMessage();
}

void NTRIPManagerTest::testRetiredTransportErrorCannotAffectNewSession()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->ntripSettings();
    saved.setFactValue(settings->ntripServerHostAddress(), QStringLiteral("caster.example.com"));
    saved.setFactValue(settings->ntripMountpoint(), QStringLiteral("TEST"));
    saved.setFactValue(settings->ntripServerConnectEnabled(), true);
    NTRIPManager mgr;
    mgr._settings = settings;
    auto* first = new MockNTRIPTransport(&mgr);
    first->autoConnect = false;
    mgr.setTransportForTest(first);
    mgr.startNTRIP();
    first->simulateError(NTRIPError::HttpError, QStringLiteral("retired failure"), std::chrono::seconds{300});
    mgr.stopNTRIP();
    auto* second = new MockNTRIPTransport(&mgr);
    second->autoConnect = false;
    mgr.setTransportForTest(second);
    mgr.startNTRIP();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Connecting);
    QCOMPARE(mgr._transport.data(), second);
    QCOMPARE(mgr._reconnectAttempts, 0);
    QVERIFY(!mgr._reconnectTimer.isActive());
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
}

void NTRIPManagerTest::testRetryPolicy()
{
    QFETCH(qint64, retryAfterMs);
    QFETCH(int, attempts);
    QFETCH(bool, enabled);
    QFETCH(NTRIPError, code);
    QFETCH(int, expectedDelayMs);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->ntripSettings();
    saved.setFactValue(settings->ntripServerHostAddress(), QStringLiteral("caster.example.com"));
    saved.setFactValue(settings->ntripMountpoint(), QStringLiteral("TEST"));
    saved.setFactValue(settings->ntripServerConnectEnabled(), enabled);
    NTRIPManager manager;
    manager._settings = settings;
    auto* transport = new MockNTRIPTransport(&manager);
    transport->autoConnect = false;
    manager.setTransportForTest(transport);
    manager.startNTRIP();
    manager._reconnectAttempts = attempts;
    expectLogMessage("GPS.NTRIPManager", QtWarningMsg, QRegularExpression(QStringLiteral("NTRIP error:.*retry test")));
    transport->simulateError(code, QStringLiteral("retry test"), std::chrono::milliseconds{retryAfterMs});
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    verifyExpectedLogMessage();
    QCOMPARE(manager.connectionStatus(),
             expectedDelayMs ? NTRIPManager::ConnectionStatus::Reconnecting : NTRIPManager::ConnectionStatus::Error);
    QCOMPARE(manager._reconnectTimer.isActive(), expectedDelayMs != 0);
    if (expectedDelayMs) {
        QCOMPARE(manager._reconnectTimer.interval(), std::chrono::milliseconds(expectedDelayMs));
        QVERIFY(manager.statusMessage().contains(QString::number(expectedDelayMs / 1000)));
    }
    manager.stopNTRIP();
    QVERIFY(!manager._reconnectTimer.isActive());
    settings->ntripServerConnectEnabled()->setRawValue(true);
    auto* replacement = new MockNTRIPTransport(&manager);
    manager.setTransportForTest(replacement);
    manager.startNTRIP();
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Connected);
    expectLogMessage("GPS.NTRIPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("NTRIP error:.*fresh failure")));
    replacement->simulateError(NTRIPError::SocketError, QStringLiteral("fresh failure"));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    verifyExpectedLogMessage();
    QCOMPARE(manager._reconnectTimer.interval(), std::chrono::milliseconds(1000));
    QCOMPARE(manager._reconnectAttempts, 1);
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
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->ntripSettings();
    saved.setFactValue(settings->ntripServerHostAddress(), QStringLiteral("127.0.0.1"));
    saved.setFactValue(settings->ntripServerPort(), server.serverPort());
    saved.setFactValue(settings->ntripMountpoint(), QStringLiteral("TEST"));
    saved.setFactValue(settings->ntripUsername(), QString());
    saved.setFactValue(settings->ntripPassword(), QString());
    saved.setFactValue(settings->ntripUseTls(), false);
    saved.setFactValue(settings->ntripServerConnectEnabled(), true);
    NTRIPManager manager;
    manager._settings = settings;
    manager.startNTRIP();
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::shortMs());
    QTcpSocket* peer = server.nextPendingConnection();
    QVERIFY(peer);
    QTRY_VERIFY_WITH_TIMEOUT(peer->bytesAvailable() > 0, TestTimeout::shortMs());
    peer->readAll();
    expectLogMessage("GPS.NTRIPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("NTRIP error:.*%1").arg(status)));
    const QByteArray body = compressed ? QByteArray::fromHex("1f8b080000000000000303000000000000000000") : QByteArray();
    const QByteArray encoding = compressed ? QByteArray("Content-Encoding: gzip\r\n") : QByteArray();
    const QByteArray response = "HTTP/1.1 " + QByteArray::number(status) + " Error\r\nRetry-After: 17\r\n" + encoding +
                                "Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body;
    QCOMPARE(peer->write(response), response.size());
    const auto expectedState =
        status == 401 ? NTRIPManager::ConnectionStatus::Error : NTRIPManager::ConnectionStatus::Reconnecting;
    QTRY_COMPARE_WITH_TIMEOUT(manager.connectionStatus(), expectedState, TestTimeout::shortMs());
    verifyExpectedLogMessage();
    QCOMPARE(manager._reconnectTimer.isActive(), status != 401);
    QCOMPARE(manager._reconnectAttempts, status == 401 ? 0 : 1);
    if (status != 401) {
        QCOMPARE(manager._reconnectTimer.interval(), std::chrono::seconds(17));
    }
}

void NTRIPManagerTest::testRetryPublicationSuperseded_data()
{
    QTest::addColumn<int>("phase");
    QTest::newRow("caster-status") << 0;
    QTest::newRow("reconnecting-status") << 1;
    QTest::newRow("transport-stop") << 2;
}

void NTRIPManagerTest::testRetryPublicationSuperseded()
{
    QFETCH(int, phase);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->ntripSettings();
    saved.setFactValue(settings->ntripServerHostAddress(), QStringLiteral("caster.example.com"));
    saved.setFactValue(settings->ntripMountpoint(), QStringLiteral("TEST"));
    saved.setFactValue(settings->ntripServerConnectEnabled(), true);
    NTRIPManager manager;
    manager._settings = settings;
    auto* first = new MockNTRIPTransport(&manager);
    manager.setTransportForTest(first);
    manager.startNTRIP();
    auto* replacement = new MockNTRIPTransport(&manager);
    replacement->autoConnect = false;
    bool replaced = false;
    const auto restart = [&]() {
        if (std::exchange(replaced, true)) {
            return;
        }
        manager.stopNTRIP();
        manager.setTransportForTest(replacement);
        manager.startNTRIP();
    };
    if (phase == 0) {
        connect(&manager, &NTRIPManager::casterStatusChanged, this, restart);
    } else if (phase == 1) {
        connect(&manager, &NTRIPManager::connectionStatusChanged, this, [&]() {
            if (manager.connectionStatus() == NTRIPManager::ConnectionStatus::Reconnecting) {
                restart();
            }
        });
    } else {
        first->onStop = restart;
    }
    expectLogMessage("GPS.NTRIPManager", QtWarningMsg, QRegularExpression(QStringLiteral("NTRIP error:.*superseded")));
    first->simulateError(NTRIPError::HttpError, QStringLiteral("superseded"), std::chrono::seconds(300));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    verifyExpectedLogMessage();
    QVERIFY(replaced);
    QCOMPARE(manager._transport.data(), replacement);
    QCOMPARE(manager.connectionStatus(), NTRIPManager::ConnectionStatus::Connecting);
    QVERIFY(!manager._reconnectTimer.isActive());
    QCOMPARE(manager._reconnectAttempts, 0);
}

void NTRIPManagerTest::testMissingMountpointDoesNotStartTransport()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->ntripSettings();
    saved.setFactValue(settings->ntripServerHostAddress(), QStringLiteral("caster.example.com"));
    saved.setFactValue(settings->ntripMountpoint(), QString());
    NTRIPManager mgr;
    mgr._settings = settings;
    auto* transport = new MockNTRIPTransport(&mgr);
    mgr.setTransportForTest(transport);
    expectLogMessage("GPS.NTRIPManager", QtWarningMsg, QRegularExpression(QStringLiteral("Select a mountpoint")));
    mgr.startNTRIP();
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Error);
    QCOMPARE(transport->startCount, 0);
    QVERIFY(!mgr._transport);
    QVERIFY(mgr.statusMessage().contains(QStringLiteral("mountpoint")));
    verifyExpectedLogMessage();
}

void NTRIPManagerTest::testCorrectionIngressKeepsSessionAndIdentity()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->ntripSettings();
    saved.setFactValue(settings->ntripServerConnectEnabled(), false);
    saved.setFactValue(settings->ntripServerHostAddress(), QStringLiteral("caster.example.com"));
    saved.setFactValue(settings->ntripServerPort(), 2101);
    saved.setFactValue(settings->ntripMountpoint(), QStringLiteral("TEST"));
    saved.setFactValue(settings->ntripUsername(), QStringLiteral("private-user"));
    saved.setFactValue(settings->ntripPassword(), QStringLiteral("private-password"));
    saved.setFactValue(settings->ntripUseTls(), true);
    saved.setFactValue(settings->ntripUdpForwardEnabled(), false);
    GPSCorrectionManager corrections;
    corrections.rtcmMavlink()->setOutputProvider([]() {
        return QList<RTCMMavlink::Output>{{QStringLiteral("test"), 1, [](const GpsRtcmPacket&) { return true; }}};
    });
    NTRIPManager mgr;
    mgr.setCorrectionManager(&corrections);
    QCOMPARE(mgr.metaObject()->indexOfProperty("rtcmMavlink"), -1);
    QSignalSpy routed(&corrections, &GPSCorrectionManager::correctionRouted);
    auto* first = new MockNTRIPTransport(&mgr);
    first->autoConnect = false;
    QSignalSpy observed(first, &NTRIPTransport::correctionFrameReceived);
    mgr.setTransportForTest(first);
    mgr.init();
    QCOMPARE(first->startCount, 0);
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);
    settings->ntripServerConnectEnabled()->setRawValue(true);
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

void NTRIPManagerTest::testSettingsProduceExplicitConfiguration()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->ntripSettings();
    saved.setFactValue(settings->ntripServerConnectEnabled(), false);

    const NTRIPConfiguration expected{
        .connection = {.host = QStringLiteral("caster.example.com"),
                       .port = 443,
                       .username = QStringLiteral("user"),
                       .password = QStringLiteral("pass"),
                       .mountpoint = QStringLiteral("MOUNT"),
                       .useTls = true,
                       .allowSelfSignedCerts = true},
        .filter = {.whitelist = QStringLiteral("1005,1077")},
        .udpForward = {.enabled = true, .address = QStringLiteral("127.0.0.1"), .port = 3001}};
    saved.setFactValue(settings->ntripServerHostAddress(), expected.connection.host);
    saved.setFactValue(settings->ntripServerPort(), expected.connection.port);
    saved.setFactValue(settings->ntripUsername(), expected.connection.username);
    saved.setFactValue(settings->ntripPassword(), expected.connection.password);
    saved.setFactValue(settings->ntripMountpoint(), expected.connection.mountpoint);
    saved.setFactValue(settings->ntripUseTls(), expected.connection.useTls);
    saved.setFactValue(settings->ntripAllowSelfSignedCerts(), expected.connection.allowSelfSignedCerts);
    saved.setFactValue(settings->ntripWhitelist(), expected.filter.whitelist);
    saved.setFactValue(settings->ntripUdpForwardEnabled(), expected.udpForward.enabled);
    saved.setFactValue(settings->ntripUdpTargetAddress(), expected.udpForward.address);
    saved.setFactValue(settings->ntripUdpTargetPort(), expected.udpForward.port);

    NTRIPManager manager;
    manager._settings = settings;
    QCOMPARE(manager._configFromSettings(), expected);
}

void NTRIPManagerTest::testFactChangesReconfigureTransport_data()
{
    QTest::addColumn<QString>("setting");
    QTest::newRow("cold-whitelist") << QStringLiteral("whitelist");
    QTest::newRow("warm-udp-output") << QStringLiteral("udp");
    QTest::newRow("hot-mountpoint") << QStringLiteral("mountpoint");
}

void NTRIPManagerTest::testGgaSettingsUseInjectedProviders()
{
    using Source = NTRIPGgaProvider::PositionSource;
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->ntripSettings();
    saved.setFactValue(settings->ntripServerHostAddress(), QStringLiteral("caster.example.com"));
    saved.setFactValue(settings->ntripMountpoint(), QStringLiteral("TEST"));
    saved.setFactValue(settings->ntripServerConnectEnabled(), true);
    saved.setFactValue(settings->ntripGgaPositionSource(), static_cast<int>(Source::VehicleGPS));
    saved.setFactValue(settings->ntripGgaIntervalSec(), 60);
    NTRIPManager manager;
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

    expectLogMessage("GPS.NTRIPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Inject GGA position providers before initializing NTRIP")));
    manager.setGgaPositionProvider(Source::GCSPosition, []() { return PositionResult{}; });
    verifyExpectedLogMessage();
    settings->ntripGgaPositionSource()->setRawValue(static_cast<int>(Source::GCSPosition));
    settings->ntripGgaIntervalSec()->setRawValue(1);
    QTRY_COMPARE_WITH_TIMEOUT(manager.ggaSource(), QStringLiteral("Injected GCS"), TestTimeout::mediumMs());
    QVERIFY(transport->sentNmea.size() >= 2);
    QCOMPARE(transport->startCount, 1);
    QCOMPARE(transport->stopCount, 0);
    QVERIFY(transitions.isEmpty());
    manager.stopNTRIP();
}

void NTRIPManagerTest::testFactChangesReconfigureTransport()
{
    QFETCH(QString, setting);
    QUdpSocket listener;
    QVERIFY(listener.bind(QHostAddress(QHostAddress::LocalHost), 0));
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->ntripSettings();
    saved.setFactValue(settings->ntripServerConnectEnabled(), true);
    saved.setFactValue(settings->ntripServerHostAddress(), QStringLiteral("caster.example.com"));
    saved.setFactValue(settings->ntripServerPort(), 2101);
    saved.setFactValue(settings->ntripMountpoint(), QStringLiteral("TEST"));
    saved.setFactValue(settings->ntripUsername(), QString());
    saved.setFactValue(settings->ntripPassword(), QString());
    saved.setFactValue(settings->ntripUseTls(), false);
    saved.setFactValue(settings->ntripWhitelist(), QStringLiteral("1005"));
    saved.setFactValue(settings->ntripUdpForwardEnabled(), false);
    saved.setFactValue(settings->ntripUdpTargetAddress(), QStringLiteral("127.0.0.1"));
    saved.setFactValue(settings->ntripUdpTargetPort(), listener.localPort());
    GPSCorrectionManager corrections;
    NTRIPManager mgr;
    mgr.setCorrectionManager(&corrections);
    auto* first = new MockNTRIPTransport(&mgr);
    int stops = 0;
    first->onStop = [&]() { ++stops; };
    mgr.setTransportForTest(first);
    mgr.init();
    QCOMPARE(first->startCount, 1);
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Connected);
    QSignalSpy routed(&corrections, &GPSCorrectionManager::correctionRouted);
    first->simulateRtcmData(GpsTestHelpers::buildRtcmFrame(1005), 1005);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QCOMPARE(routed.size(), 1);
    const auto original = qvariant_cast<GPSCorrectionFrame>(routed[0][0]);

    auto* second = new MockNTRIPTransport(&mgr);
    mgr.setTransportForTest(second);
    QSignalSpy settingsApplied(&mgr._settingsDebounceTimer, &QChronoTimer::timeout);
    const bool reconnect = setting == QStringLiteral("mountpoint");
    if (reconnect) {
        settings->ntripMountpoint()->setRawValue(QStringLiteral("REPLACEMENT"));
    } else if (setting == QStringLiteral("whitelist")) {
        settings->ntripWhitelist()->setRawValue(QStringLiteral("1077"));
    } else {
        settings->ntripUdpForwardEnabled()->setRawValue(true);
    }
    QVERIFY_SIGNAL_WAIT(settingsApplied, TestTimeout::shortMs());
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
    if (setting == QStringLiteral("udp")) {
        QTRY_VERIFY_WITH_TIMEOUT(listener.hasPendingDatagrams(), TestTimeout::shortMs());
        QCOMPARE(listener.receiveDatagram().data(), frame);
        QVERIFY(!listener.hasPendingDatagrams());
    }

    settingsApplied.clear();
    settings->ntripServerConnectEnabled()->setRawValue(false);
    QVERIFY_SIGNAL_WAIT(settingsApplied, TestTimeout::shortMs());
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);
    QVERIFY(corrections.sourceInstances().isEmpty());
}

void NTRIPManagerTest::testNtripOnlyUdpForwardingBypassesSelectionOnce()
{
    QUdpSocket listener;
    QVERIFY(listener.bind(QHostAddress(QHostAddress::LocalHost), 0));
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->ntripSettings();
    saved.setFactValue(settings->ntripServerHostAddress(), QStringLiteral("caster.example.com"));
    saved.setFactValue(settings->ntripMountpoint(), QStringLiteral("TEST"));
    saved.setFactValue(settings->ntripUdpForwardEnabled(), true);
    saved.setFactValue(settings->ntripUdpTargetAddress(), QStringLiteral("127.0.0.1"));
    saved.setFactValue(settings->ntripUdpTargetPort(), listener.localPort());
    GPSCorrectionManager corrections;
    corrections.applyRoutingConfiguration(
        {GPSCorrectionManager::RoutingPolicy::Manual, GPSCorrectionSource::LocalReceiver, {}});
    auto local = corrections.registerSource(GPSCorrectionSource::LocalReceiver, QStringLiteral("serial:test"));
    auto udp = corrections.registerSource(GPSCorrectionSource::Udp);
    NTRIPManager mgr;
    mgr.setCorrectionManager(&corrections);
    mgr._settings = settings;
    auto* transport = new MockNTRIPTransport(&mgr);
    mgr.setTransportForTest(transport);
    mgr.startNTRIP();
    QSignalSpy routed(&corrections, &GPSCorrectionManager::correctionRouted);
    const auto localFrame = GpsTestHelpers::buildRtcmFrame(1005);
    const auto udpFrame = GpsTestHelpers::buildRtcmFrame(1077);
    const auto ntripFrame = GpsTestHelpers::buildRtcmFrame(1087);
    const qint64 now = GPSCorrectionFrame::monotonicNowMs();
    corrections.acceptIngress(local.token().event(localFrame, now, 1005, true));
    corrections.acceptIngress(udp.token().event(udpFrame, now, 1077, true));
    transport->simulateRtcmData(ntripFrame, 1087);
    QTRY_VERIFY_WITH_TIMEOUT(listener.hasPendingDatagrams(), TestTimeout::shortMs());
    QCOMPARE(listener.receiveDatagram().data(), ntripFrame);
    QCOMPARE(routed.size(), 1);
    QCOMPARE(qvariant_cast<GPSCorrectionFrame>(routed[0][0]).source, GPSCorrectionSource::LocalReceiver);
    QCOMPARE(corrections.rtcmMavlink()->totalBytesSent(), quint64(localFrame.size()));
    const auto ntripStats = corrections.sources()[static_cast<int>(GPSCorrectionSource::Ntrip)].toMap();
    QCOMPARE(ntripStats.value(QStringLiteral("receivedFrames")).toULongLong(), 1);
    QCOMPARE(ntripStats.value(QStringLiteral("selectedFrames")).toULongLong(), 0);
    QVariantMap forwarding;
    for (const auto& destination : corrections.destinations()) {
        const auto stats = destination.toMap();
        if (stats.value(QStringLiteral("destinationId")).toString() == QStringLiteral("ntripUdp")) {
            forwarding = stats;
        }
    }
    QCOMPARE(forwarding.value(QStringLiteral("queuedBytes")).toULongLong(), quint64(ntripFrame.size()));
    QCOMPARE(forwarding.value(QStringLiteral("queuedFrames")).toULongLong(), 1);
    QVERIFY(!listener.waitForReadyRead(TestTimeout::shortMs()));
    QVERIFY(!listener.hasPendingDatagrams());
}

void NTRIPManagerTest::testTransportDiagnosticsReachManager()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->ntripSettings();
    saved.setFactValue(settings->ntripServerHostAddress(), QStringLiteral("127.0.0.1"));
    saved.setFactValue(settings->ntripServerPort(), server.serverPort());
    saved.setFactValue(settings->ntripMountpoint(), QStringLiteral("TEST"));
    saved.setFactValue(settings->ntripUsername(), QString());
    saved.setFactValue(settings->ntripPassword(), QString());
    saved.setFactValue(settings->ntripUseTls(), false);
    saved.setFactValue(settings->ntripWhitelist(), QStringLiteral("1005"));
    saved.setFactValue(settings->ntripUdpForwardEnabled(), false);
    GPSCorrectionManager corrections;
    NTRIPManager mgr;
    mgr.setCorrectionManager(&corrections);
    mgr._settings = settings;
    QSignalSpy routed(&corrections, &GPSCorrectionManager::correctionRouted);
    mgr.startNTRIP();
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::shortMs());
    QTcpSocket* peer = server.nextPendingConnection();
    QVERIFY(peer);
    QTRY_VERIFY_WITH_TIMEOUT(peer->bytesAvailable() > 0, TestTimeout::shortMs());
    const auto accepted = GpsTestHelpers::buildRtcmFrame(1005);
    const auto filtered = GpsTestHelpers::buildRtcmFrame(1077);
    auto rejected = GpsTestHelpers::buildRtcmFrame(1087);
    rejected.back() ^= 1;
    expectLogMessage("GPS.NTRIPHttpTransport", QtWarningMsg, QRegularExpression(QStringLiteral("Invalid RTCM frame")));
    const QByteArray body = accepted + filtered + rejected;
    const QByteArray response = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n" +
                                QByteArray::number(body.size(), 16) + "\r\n" + body + "\r\n";
    QCOMPARE(peer->write(response), response.size());
    QTRY_COMPARE_WITH_TIMEOUT(corrections.sources()[static_cast<int>(GPSCorrectionSource::Ntrip)]
                                  .toMap()
                                  .value(QStringLiteral("receivedFrames"))
                                  .toULongLong(),
                              3, TestTimeout::shortMs());
    verifyExpectedLogMessage();
    const auto stats = corrections.sources()[static_cast<int>(GPSCorrectionSource::Ntrip)].toMap();
    QCOMPARE(stats.value(QStringLiteral("validatedFrames")).toULongLong(), 2);
    QCOMPARE(stats.value(QStringLiteral("filteredFrames")).toULongLong(), 2);
    QCOMPARE(routed.size(), 1);
    QCOMPARE(corrections.rtcmMavlink()->totalBytesSent(), quint64(accepted.size()));
    mgr.stopNTRIP();
}
