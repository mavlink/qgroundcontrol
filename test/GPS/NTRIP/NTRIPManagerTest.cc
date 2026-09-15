#include "NTRIPManagerTest.h"

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
    GPSCorrectionManager corrections;
    NTRIPManager mgr;
    mgr.setCorrectionManager(&corrections);
    QUdpSocket listener;
    QVERIFY(listener.bind(QHostAddress(QHostAddress::LocalHost), 0));

    NTRIPTransportConfig config;
    config.udpForwardEnabled = true;
    config.udpTargetAddress = QStringLiteral("127.0.0.1");
    config.udpTargetPort = listener.localPort();

    mgr._applyUdpForwarderConfig(config);
    auto source = corrections.registerSource(GPSCorrectionSource::Ntrip);
    const auto frame = GpsTestHelpers::buildRtcmFrame(1005);
    corrections.acceptIngress(source.token().event(frame, GPSCorrectionFrame::monotonicNowMs(), 1005, true));
    QTRY_VERIFY_WITH_TIMEOUT(listener.hasPendingDatagrams(), TestTimeout::shortMs());
    QCOMPARE(listener.receiveDatagram().data(), frame);

    mgr._enterState(NTRIPManager::ConnectionStatus::Error, QStringLiteral("bad config"));
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
    first->simulateError(NTRIPError::SocketError, QStringLiteral("retired failure"));
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
    QCOMPARE(mgr.rtcmMavlink(), corrections.rtcmMavlink());
    QSignalSpy routed(&corrections, &GPSCorrectionManager::correctionRouted);
    auto* first = new MockNTRIPTransport(&mgr);
    first->autoConnect = false;
    QSignalSpy accepted(first, &NTRIPTransport::RTCMDataUpdate);
    mgr.setTransportForTest(first);
    mgr.init();
    QCOMPARE(first->startCount, 0);
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);
    settings->ntripServerConnectEnabled()->setRawValue(true);
    QTRY_COMPARE_WITH_TIMEOUT(first->startCount, 1, TestTimeout::shortMs());
    const QByteArray frame = GpsTestHelpers::buildRtcmFrame(1005);
    const qint64 receivedAtMs = GPSCorrectionFrame::monotonicNowMs() - 10;
    first->simulateRtcmData(frame, 1005, receivedAtMs);
    QCOMPARE(accepted.size(), 1);
    QCOMPARE(accepted[0][0].toByteArray(), frame);
    QCOMPARE(accepted[0][1].toInt(), 1005);
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
    QTest::newRow("warm-udp-output") << QStringLiteral("udp");
    QTest::newRow("hot-mountpoint") << QStringLiteral("mountpoint");
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
    QSignalSpy stopped(first, &NTRIPTransport::finished);
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
    QCOMPARE(stopped.size(), reconnect ? 1 : 0);
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
    corrections.setSelectedSource(GPSCorrectionSource::LocalReceiver);
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
    const QByteArray response = QByteArrayLiteral("HTTP/1.1 200 OK\r\n\r\n") + accepted + filtered + rejected;
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
