#include "NTRIPManagerTest.h"

#include <QtCore/QChronoTimer>
#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "Fixtures/RAIIFixtures.h"
#include "GPSCorrectionManager.h"
#include "GpsTestHelpers.h"
#include "MockNTRIPStream.h"
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

    ignoreLogMessage("GPS.NTRIP.NTRIPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Credentials sent without TLS encryption")));

    QVERIFY(mgr.securityWarning().isEmpty());
    mgr._onPlaintextCredentialsWarning();

    QCOMPARE(warningSpy.count(), 1);
    QVERIFY(mgr.securityWarning().contains(QStringLiteral("without TLS")));
}

// ---------------------------------------------------------------------------
// Reconnect backoff (migrated from NTRIPReconnectPolicyTest)
// ---------------------------------------------------------------------------

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
    auto* transport = new MockNTRIPStream(&mgr);
    transport->autoConnect = false;
    mgr.setTransportForTest(transport);
    mgr.startNTRIP();
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Connecting);
    expectLogMessage("GPS.NTRIP.NTRIPManager", QtWarningMsg,
                     QRegularExpression(QStringLiteral("NTRIP error:.*first failure")));
    transport->simulateError(NTRIPError::SocketError, QStringLiteral("first failure"));
    transport->simulateError(NTRIPError::ServerDisconnected, QStringLiteral("duplicate failure"));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Reconnecting);
    QCOMPARE(mgr._session._failedAttempts, 1);
    QCOMPARE(mgr._session.nextRetryDelay(), std::chrono::milliseconds(1000));
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
    auto* first = new MockNTRIPStream(&mgr);
    first->autoConnect = false;
    mgr.setTransportForTest(first);
    mgr.startNTRIP();
    first->simulateError(NTRIPError::SocketError, QStringLiteral("retired failure"));
    mgr.stopNTRIP();
    auto* second = new MockNTRIPStream(&mgr);
    second->autoConnect = false;
    mgr.setTransportForTest(second);
    mgr.startNTRIP();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Connecting);
    QCOMPARE(mgr._session._stream.data(), second);
    QCOMPARE(mgr._session._failedAttempts, 0);
    QVERIFY(!mgr._session.retryPending());
}

void NTRIPManagerTest::testMissingMountpointDoesNotStartTransport()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->ntripSettings();
    saved.setFactValue(settings->ntripServerHostAddress(), QStringLiteral("caster.example.com"));
    saved.setFactValue(settings->ntripMountpoint(), QString());
    NTRIPManager mgr;
    mgr._settings = settings;
    auto* transport = new MockNTRIPStream(&mgr);
    mgr.setTransportForTest(transport);
    expectLogMessage("GPS.NTRIP.NTRIPManager", QtWarningMsg, QRegularExpression(QStringLiteral("Select a mountpoint")));
    mgr.startNTRIP();
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Error);
    QCOMPARE(transport->startCount, 0);
    QVERIFY(!mgr._session._stream);
    QVERIFY(mgr.statusMessage().contains(QStringLiteral("mountpoint")));
    verifyExpectedLogMessage();
}

void NTRIPManagerTest::testCorrectionsAreIndependentOfSink_data()
{
    QTest::addColumn<bool>("withSink");
    QTest::newRow("no-sink") << false;
    QTest::newRow("shared-forwarder") << true;
}

void NTRIPManagerTest::testCorrectionsAreIndependentOfSink()
{
    QFETCH(bool, withSink);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->ntripSettings();
    saved.setFactValue(settings->ntripServerHostAddress(), QStringLiteral("caster.example.com"));
    saved.setFactValue(settings->ntripMountpoint(), QStringLiteral("TEST"));
    NTRIPManager mgr;
    mgr._settings = settings;
    GPSCorrectionManager corrections;
    quint64 attempt = 0;
    if (withSink) {
        connect(&mgr, &NTRIPManager::correctionSessionStarted, &corrections, [&](quint64 id, const QString& sourceId) {
            if (id == mgr.correctionAttemptId()) {
                attempt = id;
                corrections.beginSourceSession(GPSCorrectionSource::Ntrip, sourceId);
            }
        });
        connect(&mgr, &NTRIPManager::correctionSessionEnded, &corrections, [&](quint64 id) {
            if (id == attempt) {
                attempt = 0;
                corrections.endSourceSession(GPSCorrectionSource::Ntrip);
            }
        });
        connect(&mgr, &NTRIPManager::correctionReceivedAt, &corrections,
                [&](const QByteArray& data, int id, bool filtered, qint64 timestamp, quint64 sourceAttempt) {
                    if (attempt && sourceAttempt == attempt) {
                        corrections.acceptFrame({GPSCorrectionSource::Ntrip,
                                                 corrections.sourceSession(GPSCorrectionSource::Ntrip), timestamp, data,
                                                 id, true, filtered});
                    }
                });
    }
    QSignalSpy received(&mgr, &NTRIPManager::correctionReceivedAt);
    auto* transport = new MockNTRIPStream(&mgr);
    transport->autoConnect = false;
    mgr.setTransportForTest(transport);
    mgr.startNTRIP();
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Connecting);
    const QByteArray payload = GpsTestHelpers::buildRtcmFrame(1005, 20);
    transport->simulateRtcmData(payload, 1005);
    QCOMPARE(received.size(), 1);
    QCOMPARE(received.first().first().toByteArray(), payload);
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Connected);
    QCOMPARE(mgr.connectionStats()->bytesReceived(), quint64(payload.size()));
    QCOMPARE(mgr.connectionStats()->messagesReceived(), quint32(1));
    QCOMPARE(corrections.rtcmMavlink()->totalBytesSent(), withSink ? quint64(payload.size()) : quint64(0));
    mgr.stopNTRIP();
    transport->simulateRtcmData(payload, 1005);
    QCOMPARE(received.size(), 1);
}

void NTRIPManagerTest::testCorrectionObserverCanStopSession()
{
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->ntripSettings();
    saved.setFactValue(settings->ntripServerHostAddress(), QStringLiteral("caster.example.com"));
    saved.setFactValue(settings->ntripMountpoint(), QStringLiteral("TEST"));
    NTRIPManager mgr;
    mgr._settings = settings;
    auto* transport = new MockNTRIPStream(&mgr);
    transport->autoConnect = false;
    mgr.setTransportForTest(transport);
    mgr.startNTRIP();
    connect(&mgr, &NTRIPManager::correctionReceivedAt, &mgr, &NTRIPManager::stopNTRIP);
    transport->simulateRtcmData(QByteArrayLiteral("correction"), 1005);
    QCOMPARE(mgr.connectionStatus(), NTRIPManager::ConnectionStatus::Disconnected);
    QVERIFY(!mgr._session._stream);
    QVERIFY(!mgr._session.retryPending());
}

void NTRIPManagerTest::testSessionStartObserverCanStop_data()
{
    QTest::addColumn<bool>("restart");
    QTest::newRow("stop-before-socket-start") << false;
    QTest::newRow("replace-before-socket-start") << true;
}

void NTRIPManagerTest::testSessionStartObserverCanStop()
{
    QFETCH(bool, restart);
    TestFixtures::SettingsFixture saved;
    auto* settings = SettingsManager::instance()->ntripSettings();
    saved.setFactValue(settings->ntripServerHostAddress(), QStringLiteral("caster.example.com"));
    saved.setFactValue(settings->ntripMountpoint(), QStringLiteral("TEST"));
    NTRIPManager mgr;
    mgr._settings = settings;
    auto* first = new MockNTRIPStream(&mgr);
    first->autoConnect = false;
    auto* second = new MockNTRIPStream(&mgr);
    second->autoConnect = false;
    mgr.setTransportForTest(first);
    QSignalSpy started(&mgr, &NTRIPManager::correctionSessionStarted);
    QSignalSpy ended(&mgr, &NTRIPManager::correctionSessionEnded);
    bool initialNotification = true;
    connect(&mgr, &NTRIPManager::correctionSessionStarted, &mgr, [&]() {
        if (!initialNotification) {
            return;
        }
        initialNotification = false;
        mgr.stopNTRIP();
        if (restart) {
            mgr.setTransportForTest(second);
            mgr.startNTRIP();
        }
    });
    mgr.startNTRIP();
    QCOMPARE(first->startCount, 0);
    QCOMPARE(first->stopCount, 1);
    QCOMPARE(ended.size(), 1);
    QCOMPARE(started.size(), restart ? 2 : 1);
    QCOMPARE(second->startCount, restart ? 1 : 0);
    QCOMPARE(mgr.connectionStatus(),
             restart ? NTRIPManager::ConnectionStatus::Connecting : NTRIPManager::ConnectionStatus::Disconnected);
    QCOMPARE(mgr._session._stream.data(), restart ? second : nullptr);
}
