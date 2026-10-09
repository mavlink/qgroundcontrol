#include "NTRIPTlsTest.h"

#include <array>
#include <memory>
#include <utility>

#include <QtCore/QCryptographicHash>
#include <QtCore/QDateTime>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslError>
#include <QtNetwork/QSslKey>
#include <QtNetwork/QSslServer>
#include <QtNetwork/QSslSocket>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "MultiSignalSpy.h"
#include "NTRIP/Support/NTRIPTestHelpers.h"
#include "NTRIP/Support/NTRIPTlsTestFixtures.h"
#include "NTRIP/Support/ScriptedNTRIPCaster.h"
#include "NTRIPHttpSession.h"
#include "NTRIPHttpTransport.h"
#include "NTRIPSourceTableController.h"
#include "NTRIPTlsPolicy_p.h"
#include "Protocols/Support/ProtocolTestPackets.h"
#include "RTCMFramer.h"
#include "Support/GPSTestHelpers.h"

using namespace GPSTest;

namespace {
QString tlsError(QSslError::SslError code)
{
    return QRegularExpression::escape(QSslError(code).errorString());
}

QByteArray sourceTableResponse()
{
    return okResponse("STR;MP;Id;RTCM 3.2;;2;GPS;NET;USA;40;-74;0;1;gen;none;B;N;4800\r\nENDSOURCETABLE\r\n");
}
}  // namespace

void NTRIPTlsTest::initTestCase()
{
    UnitTest::initTestCase();
    if (!QSslSocket::supportsSsl()) {
        QSKIP("No TLS backend available");
    }
    QVERIFY(!QSslKey(PRIVATE_KEY_PEM, QSsl::Rsa, QSsl::Pem).isNull());
    const auto now = QDateTime::currentDateTimeUtc();
    for (const auto& pem : {SERVER_CERT_PEM, ROTATED_CERT_PEM, MISMATCHED_CERT_PEM, ISSUED_CERT_PEM}) {
        const QSslCertificate certificate(pem, QSsl::Pem);
        QVERIFY(!certificate.isNull());
        QVERIFY2(certificate.effectiveDate() <= now, "Test certificate is not yet valid");
        QVERIFY2(certificate.expiryDate() > now, "Replace the expired test-only TLS certificate");
    }
}

void NTRIPTlsTest::_expectSelfSignedWarning()
{
    expectLogMessage("GPS.NTRIP.NTRIPHttpSession", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^TLS error: \"(?:%1|%2)\"$")
                                            .arg(tlsError(QSslError::SelfSignedCertificate),
                                                 tlsError(QSslError::SelfSignedCertificateInChain))));
}

void NTRIPTlsTest::_expectTlsWarnings(bool allowSelfSigned, bool mismatched)
{
    const QRegularExpression policy(
        mismatched
            ? QStringLiteral("^TLS error: \"%1\"$").arg(tlsError(QSslError::HostNameMismatch))
            : QRegularExpression::anchoredPattern(QRegularExpression::escape(
                  allowSelfSigned ? QStringLiteral("Accepting self-signed certificate (user opted in)")
                                  : QStringLiteral("Rejecting self-signed certificate (enable 'Accept self-signed "
                                                   "certificates' to allow)"))));
    _expectSelfSignedWarning();
    expectLogMessage("GPS.NTRIP.NTRIPHttpSession", QtWarningMsg, policy);
}

void NTRIPTlsTest::_verifyTlsWarnings()
{
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
}

void NTRIPTlsTest::_selfSignedClassification_data()
{
    const QSslCertificate selfSigned(SERVER_CERT_PEM, QSsl::Pem);
    const QSslCertificate issued(ISSUED_CERT_PEM, QSsl::Pem);
    QVERIFY(selfSigned.isSelfSigned());
    QVERIFY(!issued.isNull() && !issued.isSelfSigned());
    QTest::addColumn<QList<QSslError>>("errors");
    QTest::addColumn<bool>("allowed");
    QTest::newRow("empty") << QList<QSslError>{} << false;
    QTest::newRow("self-signed") << QList{QSslError(QSslError::SelfSignedCertificate, selfSigned)} << true;
    QTest::newRow("self-signed-chain") << QList{QSslError(QSslError::SelfSignedCertificateInChain, selfSigned)} << true;
    QList allowedErrors{QSslError(QSslError::SelfSignedCertificate, selfSigned),
                        QSslError(QSslError::SelfSignedCertificateInChain, selfSigned)};
    const std::array trustErrors{
        std::pair{"local-issuer", QSslError::UnableToGetLocalIssuerCertificate},
        std::pair{"first-certificate", QSslError::UnableToVerifyFirstCertificate},
        std::pair{"untrusted", QSslError::CertificateUntrusted},
    };
    for (const auto& [name, code] : trustErrors) {
        QTest::addRow("%s-self-signed", name) << QList{QSslError(code, selfSigned)} << true;
        QTest::addRow("%s-missing-certificate", name) << QList{QSslError(code)} << false;
        QTest::addRow("%s-issued", name) << QList{QSslError(code, issued)} << false;
        allowedErrors.append(QSslError(code, selfSigned));
    }
    QTest::newRow("all-self-signed-trust-errors") << allowedErrors << true;
    const std::array fatalErrors{
        std::pair{"hostname-mismatch", QSslError::HostNameMismatch},
        std::pair{"expired", QSslError::CertificateExpired},
        std::pair{"revoked", QSslError::CertificateRevoked},
        std::pair{"invalid-signature", QSslError::CertificateSignatureFailed},
        std::pair{"no-error", QSslError::NoError},
        std::pair{"unknown", QSslError::UnspecifiedError},
    };
    for (const auto& [name, code] : fatalErrors) {
        const QSslError fatal(code, selfSigned);
        QTest::newRow(name) << QList{fatal} << false;
        auto mixed = allowedErrors;
        mixed.append(fatal);
        QTest::addRow("%s-after-allowed", name) << mixed << false;
        mixed.removeLast();
        mixed.prepend(fatal);
        QTest::addRow("%s-before-allowed", name) << mixed << false;
    }
    auto mixed = allowedErrors;
    mixed.append(QSslError(QSslError::CertificateUntrusted, issued));
    QTest::newRow("self-signed-and-unrelated-certificate") << mixed << false;
}

void NTRIPTlsTest::_selfSignedClassification()
{
    QFETCH(QList<QSslError>, errors);
    QFETCH(bool, allowed);
    QCOMPARE(NTRIPTlsPolicy::isSelfSignedOnly(errors), allowed);
}

void NTRIPTlsTest::_certificatePolicy_data()
{
    QTest::addColumn<bool>("allowSelfSigned");
    QTest::addColumn<bool>("mismatched");
    // The source table is fetched through the same session; _sourceTablePolicyChanges covers its policy.
    QTest::newRow("reject-self-signed-by-default") << false << false;
    QTest::newRow("opt-in") << true << false;
    QTest::newRow("opt-in-still-rejects-hostname-mismatch") << true << true;
}

void NTRIPTlsTest::_certificatePolicy()
{
    QFETCH(bool, allowSelfSigned);
    QFETCH(bool, mismatched);
    ScriptedNTRIPCaster caster(ScriptedNTRIPCaster::Transport::Tls, mismatched
                                                                        ? ScriptedNTRIPCaster::Certificate::Mismatched
                                                                        : ScriptedNTRIPCaster::Certificate::Loopback);
    QVERIFY(caster.isListening());
    auto configuration = caster.connectionConfig();
    QVERIFY(!configuration.allowSelfSignedCerts);
    configuration.allowSelfSignedCerts = allowSelfSigned;
    NTRIPHttpTransport transport(configuration, {});
    MultiSignalSpy spy;
    QVERIFY(spy.init(&transport));
    const QSignalSpy& frames = *spy.spy("correctionFrameReceived");
    _expectTlsWarnings(allowSelfSigned, mismatched);
    transport.start();

    if (!allowSelfSigned || mismatched) {
        QVERIFY_WAIT_SIGNAL(spy, "error", TestTimeout::mediumMs());
        _verifyTlsWarnings();
        const auto failure = spy.argument<NTRIPFailure>("error");
        QCOMPARE(failure.code, NTRIPError::SslError);
        if (mismatched) {
            QVERIFY(failure.detail.contains(QSslError(QSslError::HostNameMismatch).errorString()));
        } else {
            QVERIFY(failure.detail.contains(QStringLiteral("Self-signed certificate rejected")));
        }
        // Callbacks queued by the aborted handshake must not report a second error, before or after stop().
        deliverQueuedCalls();
        QVERIFY_ONLY_SIGNAL(spy, "error");
        transport.stop();
        deliverQueuedCalls();
        QVERIFY_ONLY_SIGNAL(spy, "error");
        return;
    }

    auto* connection = caster.waitForConnection();
    QVERIFY(connection && connection->peer);
    auto* peer = qobject_cast<QSslSocket*>(connection->peer);
    QVERIFY(peer && peer->isEncrypted());
    const QByteArray request = connection->waitForRequest();
    QVERIFY(request.startsWith("GET /TEST HTTP/1.1\r\n"));
    _verifyTlsWarnings();
    QVERIFY(spy.notEmitted("connected", "correctionFrameReceived"));

    const QByteArray first = GPSTest::rtcmMessage(1005);
    const QByteArray second = GPSTest::rtcmMessage(1077);
    const QByteArray response = "HTTP/1.1 200 OK\r\nContent-Type: gnss/data\r\n\r\n" + first;
    QCOMPARE(connection->write(response), response.size());
    QTRY_COMPARE_WITH_TIMEOUT(frames.size(), 1, TestTimeout::mediumMs());
    QCOMPARE(spy.count("connected"), 1);
    QCOMPARE(connection->write(second), second.size());
    QTRY_COMPARE_WITH_TIMEOUT(frames.size(), 2, TestTimeout::mediumMs());
    for (qsizetype index = 0; index < frames.size(); ++index) {
        const auto frame = frameAt(frames, index);
        QCOMPARE(frame.data, index == 0 ? first : second);
        QCOMPARE(frame.messageId, index == 0 ? 1005 : 1077);
        QVERIFY(frame.valid && !frame.filtered);
        QVERIFY(frame.receivedAtMs > 0);
    }
    QCOMPARE(spy.count("connected"), 1);
    transport.stop();
    QTRY_COMPARE_WITH_TIMEOUT(connection->peer->state(), QAbstractSocket::UnconnectedState, TestTimeout::mediumMs());
    deliverQueuedCalls();
    QVERIFY(spy.notEmitted("error"));
}

void NTRIPTlsTest::_sourceTablePolicyChanges_data()
{
    QTest::addColumn<bool>("duringFetch");
    QTest::newRow("in-flight") << true;
    QTest::newRow("cached") << false;
}

void NTRIPTlsTest::_sourceTablePolicyChanges()
{
    QFETCH(bool, duringFetch);
    // The certificate policy is the subject here; the session reports each TLS decision.
    ignoreLogMessage("GPS.NTRIP.NTRIPHttpSession", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^(TLS error:|Accepting self-signed|Rejecting self-signed)")));
    ScriptedNTRIPCaster caster(ScriptedNTRIPCaster::Transport::Tls);
    QVERIFY(caster.isListening());
    auto configuration = caster.connectionConfig();
    configuration.allowSelfSignedCerts = true;
    NTRIPSourceTableController controller;
    QSignalSpy pins(&controller, &NTRIPSourceTableController::certificatePinned);
    const auto respond = [&]() {
        auto* connection = caster.waitForConnection();
        QVERIFY(connection && connection->peer);
        QVERIFY(connection->waitForRequest().startsWith("GET / HTTP/1.1"));
        const QByteArray response = sourceTableResponse();
        QCOMPARE(connection->write(response), response.size());
    };
    controller.fetch(configuration);
    if (!duringFetch) {
        respond();
        QTRY_COMPARE_WITH_TIMEOUT(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::Success,
                                  TestTimeout::mediumMs());
        QCOMPARE(controller.mountpointModel()->rowCount(), 1);
    } else {
        auto* connection = caster.waitForConnection();
        QVERIFY(connection && connection->peer);
        QVERIFY(connection->waitForRequest().startsWith("GET / HTTP/1.1"));
        connection->disconnectFromHost();
    }
    // A trusted-on-first-use certificate is pinned even when only the source table was fetched.
    QCOMPARE(pins.size(), 1);
    QVERIFY(pins.first().first().toString().startsWith(QStringLiteral("127.0.0.1:%1|").arg(caster.port())));

    configuration.allowSelfSignedCerts = false;
    controller.fetch(configuration);
    QTRY_COMPARE_WITH_TIMEOUT(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::Error,
                              TestTimeout::mediumMs());
    QCOMPARE(controller.mountpointModel()->rowCount(), 0);

    configuration.allowSelfSignedCerts = true;
    controller.fetch(configuration);
    respond();
    QTRY_COMPARE_WITH_TIMEOUT(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::Success,
                              TestTimeout::mediumMs());
    QCOMPARE(controller.mountpointModel()->rowCount(), 1);
}

void NTRIPTlsTest::_retireAttempt_data()
{
    QTest::addColumn<bool>("duringHandshake");
    QTest::addColumn<bool>("destroy");
    QTest::newRow("stop-during-handshake") << true << false;
    QTest::newRow("delete-during-handshake") << true << true;
    QTest::newRow("stop-before-http-response") << false << false;
    QTest::newRow("delete-before-http-response") << false << true;
}

void NTRIPTlsTest::_retireAttempt()
{
    QFETCH(bool, duringHandshake);
    QFETCH(bool, destroy);
    QPointer<QSslSocket> retiredPeer;
    bool retired = false;
    ScriptedNTRIPCaster caster(ScriptedNTRIPCaster::Transport::Tls);
    QVERIFY(caster.isListening());
    auto configuration = caster.connectionConfig();
    configuration.allowSelfSignedCerts = true;
    auto transport = std::make_unique<NTRIPHttpTransport>(configuration, QVector<int>{});
    QPointer<NTRIPHttpTransport> alive = transport.get();
    // QSignalSpy keeps its records when a row deletes the transport.
    QSignalSpy connected(transport.get(), &NTRIPTransport::connected);
    QSignalSpy frames(transport.get(), &NTRIPTransport::correctionFrameReceived);
    QSignalSpy errors(transport.get(), &NTRIPTransport::error);
    const auto retire = [&]() {
        retired = true;
        if (destroy) {
            transport.reset();
        } else {
            transport->stop();
        }
    };
    if (duringHandshake) {
        connect(caster.tlsServer(), &QSslServer::startedEncryptionHandshake, transport.get(), [&](QSslSocket* peer) {
            retiredPeer = peer;
            QVERIFY(!peer->isEncrypted());
            retire();
        });
    } else {
        _expectTlsWarnings(true);
    }
    transport->start();
    if (!duringHandshake) {
        auto* connection = caster.waitForConnection();
        QVERIFY(connection);
        retiredPeer = qobject_cast<QSslSocket*>(connection->peer);
        QVERIFY(retiredPeer);
        QVERIFY(retiredPeer->isEncrypted());
        QVERIFY(connection->waitForRequest().endsWith("\r\n\r\n"));
        _verifyTlsWarnings();
        QVERIFY(connected.isEmpty());
        retire();
    }
    QTRY_VERIFY_WITH_TIMEOUT(retired, TestTimeout::mediumMs());
    QTRY_VERIFY_WITH_TIMEOUT(!retiredPeer || retiredPeer->state() == QAbstractSocket::UnconnectedState,
                             TestTimeout::mediumMs());
    deliverQueuedCalls();
    QCOMPARE(alive.isNull(), destroy);
    QVERIFY(connected.isEmpty());
    QVERIFY(frames.isEmpty());
    QVERIFY(errors.isEmpty());
}

void NTRIPTlsTest::_restartRetiresAttempt_data()
{
    // start() retires the attempt as stop() does, which _retireAttempt covers.
    QTest::addColumn<bool>("duringHandshake");
    QTest::newRow("during-handshake") << true;
    QTest::newRow("before-http-response") << false;
}

void NTRIPTlsTest::_restartRetiresAttempt()
{
    QFETCH(bool, duringHandshake);
    QPointer<QSslSocket> retiredPeer;
    bool restarted = false;
    int handshakes = 0;
    ScriptedNTRIPCaster caster(ScriptedNTRIPCaster::Transport::Tls);
    QVERIFY(caster.isListening());
    auto configuration = caster.connectionConfig();
    configuration.allowSelfSignedCerts = true;
    NTRIPHttpTransport transport(configuration, {});
    MultiSignalSpy spy;
    QVERIFY(spy.init(&transport));
    const auto restart = [&]() {
        restarted = true;
        transport.start();
    };
    connect(caster.tlsServer(), &QSslServer::startedEncryptionHandshake, &transport, [&](QSslSocket* peer) {
        if (++handshakes == 1 && duringHandshake) {
            retiredPeer = peer;
            QVERIFY(!peer->isEncrypted());
            restart();
        }
    });
    _expectTlsWarnings(true);
    transport.start();
    if (!duringHandshake) {
        auto* retired = caster.waitForConnection();
        QVERIFY(retired);
        retiredPeer = qobject_cast<QSslSocket*>(retired->peer);
        QVERIFY(retiredPeer);
        QVERIFY(retired->waitForRequest().endsWith("\r\n\r\n"));
        _verifyTlsWarnings();
        // Queue an old-attempt response without dispatching the client's readyRead before restart.
        const QByteArray stale = "HTTP/1.1 200 OK\r\n\r\n" + GPSTest::rtcmMessage(1005);
        QCOMPARE(retired->write(stale), stale.size());
        retiredPeer->flush();
        _expectTlsWarnings(true);
        restart();
    }
    auto* active = caster.waitForConnection();
    QVERIFY(restarted && active);
    auto* peer = qobject_cast<QSslSocket*>(active->peer);
    QVERIFY(peer && peer != retiredPeer.data());
    QVERIFY(peer->isEncrypted());
    const QByteArray request = active->waitForRequest();
    QVERIFY(request.startsWith("GET /TEST HTTP/1.1\r\n") && request.endsWith("\r\n\r\n"));
    _verifyTlsWarnings();
    QTRY_VERIFY_WITH_TIMEOUT(!retiredPeer || retiredPeer->state() == QAbstractSocket::UnconnectedState,
                             TestTimeout::mediumMs());
    QCOMPARE(handshakes, 2);
    QVERIFY(spy.notEmitted("connected", "correctionFrameReceived", "error"));

    const QByteArray current = GPSTest::rtcmMessage(1077);
    const QByteArray response = "HTTP/1.1 200 OK\r\n\r\n" + current;
    QCOMPARE(active->write(response), response.size());
    QTRY_COMPARE_WITH_TIMEOUT(spy.count("correctionFrameReceived"), 1, TestTimeout::mediumMs());
    const auto frame = spy.argument<RTCMDecodedFrame>("correctionFrameReceived");
    QCOMPARE(frame.data, current);
    QVERIFY(frame.valid && !frame.filtered);
    QCOMPARE(spy.count("connected"), 1);
    transport.stop();
    QTRY_COMPARE_WITH_TIMEOUT(peer->state(), QAbstractSocket::UnconnectedState, TestTimeout::mediumMs());
    deliverQueuedCalls();
    QCOMPARE(spy.count("correctionFrameReceived"), 1);
    QVERIFY(spy.notEmitted("error"));
}

void NTRIPTlsTest::_reconnectFromTlsFailure()
{
    bool restarted = false;
    ScriptedNTRIPCaster caster(ScriptedNTRIPCaster::Transport::Tls, ScriptedNTRIPCaster::Certificate::Mismatched);
    QVERIFY(caster.isListening());
    auto configuration = caster.connectionConfig();
    configuration.allowSelfSignedCerts = true;
    NTRIPHttpTransport transport(configuration, {});
    QSignalSpy connected(&transport, &NTRIPTransport::connected);
    QSignalSpy frames(&transport, &NTRIPTransport::correctionFrameReceived);
    QSignalSpy errors(&transport, &NTRIPTransport::error);
    connect(&transport, &NTRIPTransport::error, &transport, [&](const NTRIPFailure& failure) {
        if (restarted) {
            return;
        }
        QCOMPARE(failure.code, NTRIPError::SslError);
        QVERIFY(failure.detail.contains(QSslError(QSslError::HostNameMismatch).errorString()));
        restarted = true;
        _verifyTlsWarnings();
        caster.setCertificate(ScriptedNTRIPCaster::Certificate::Loopback);
        _expectTlsWarnings(true);
        transport.start();
    });
    _expectTlsWarnings(true, true);
    transport.start();
    QTRY_VERIFY_WITH_TIMEOUT(restarted, TestTimeout::mediumMs());
    auto* connection = caster.waitForConnection();
    QVERIFY(connection && connection->peer);
    auto* peer = qobject_cast<QSslSocket*>(connection->peer);
    QVERIFY(peer && peer->isEncrypted());
    const QByteArray request = connection->waitForRequest();
    _verifyTlsWarnings();
    QVERIFY(connected.isEmpty());
    QCOMPARE(errors.size(), 1);
    const QByteArray expected = GPSTest::rtcmMessage(1005);
    const QByteArray response = "HTTP/1.1 200 OK\r\n\r\n" + expected;
    QCOMPARE(connection->write(response), response.size());
    QTRY_COMPARE_WITH_TIMEOUT(frames.size(), 1, TestTimeout::mediumMs());
    QCOMPARE(frameAt(frames).data, expected);
    QCOMPARE(connected.size(), 1);
    transport.stop();
    QTRY_COMPARE_WITH_TIMEOUT(connection->peer->state(), QAbstractSocket::UnconnectedState, TestTimeout::mediumMs());
    deliverQueuedCalls();
    QCOMPARE(errors.size(), 1);
}

void NTRIPTlsTest::_certificatePinning_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<bool>("rotated");
    QTest::addColumn<bool>("pinned");
    QTest::addColumn<bool>("accepted");
    // The pin, when present, holds the loopback certificate for 127.0.0.1; both certificates name both hosts.
    QTest::newRow("first-connection-pins") << QStringLiteral("127.0.0.1") << false << false << true;
    QTest::newRow("pinned-certificate-reconnects") << QStringLiteral("127.0.0.1") << false << true << true;
    QTest::newRow("changed-certificate-rejected") << QStringLiteral("127.0.0.1") << true << true << false;
    QTest::newRow("host-change-repins") << QStringLiteral("localhost") << true << true << true;
}

void NTRIPTlsTest::_certificatePinning()
{
    QFETCH(QString, host);
    QFETCH(bool, rotated);
    QFETCH(bool, pinned);
    QFETCH(bool, accepted);
    using Certificate = ScriptedNTRIPCaster::Certificate;
    ScriptedNTRIPCaster caster(ScriptedNTRIPCaster::Transport::Tls,
                               rotated ? Certificate::Rotated : Certificate::Loopback);
    QVERIFY(caster.isListening());
    const auto pinFor = [&caster](const QString& endpointHost, Certificate certificate) {
        const QSslCertificate leaf(ScriptedNTRIPCaster::certificatePem(certificate), QSsl::Pem);
        return QStringLiteral("%1:%2|%3")
            .arg(endpointHost)
            .arg(caster.port())
            .arg(QString::fromLatin1(leaf.digest(QCryptographicHash::Sha256).toHex()));
    };
    auto configuration = caster.connectionConfig();
    configuration.host = host;
    configuration.allowSelfSignedCerts = true;
    if (pinned) {
        configuration.pinnedCertificate = pinFor(QStringLiteral("127.0.0.1"), Certificate::Loopback);
    }
    const QString presentedPin = pinFor(host, rotated ? Certificate::Rotated : Certificate::Loopback);
    const bool pinsNewCertificate = accepted && presentedPin != configuration.pinnedCertificate;

    NTRIPHttpTransport transport(configuration, {});
    QSignalSpy pins(&transport, &NTRIPTransport::certificatePinned);
    QSignalSpy errors(&transport, &NTRIPTransport::error);
    _expectSelfSignedWarning();
    if (pinsNewCertificate) {
        expectLogMessage("GPS.NTRIP.NTRIPHttpSession", QtWarningMsg,
                         QRegularExpression(QStringLiteral("^Accepting self-signed certificate \\(user opted in\\)$")));
    } else if (!accepted) {
        expectLogMessage("GPS.NTRIP.NTRIPHttpSession", QtWarningMsg,
                         QRegularExpression(QStringLiteral("^Rejecting self-signed certificate that differs")));
    }
    transport.start();

    if (!accepted) {
        QTRY_COMPARE_WITH_TIMEOUT(errors.size(), 1, TestTimeout::mediumMs());
        const auto failure = failureAt(errors);
        QCOMPARE(failure.code, NTRIPError::SslError);
        QVERIFY(failure.detail.contains(QStringLiteral("certificate changed")));
    } else {
        auto* connection = caster.waitForConnection();
        QVERIFY(connection && connection->peer);
        QVERIFY(connection->waitForRequest().startsWith("GET /TEST HTTP/1.1\r\n"));
        QVERIFY(errors.isEmpty());
    }
    verifyExpectedLogMessage();
    if (pinsNewCertificate || !accepted) {
        verifyExpectedLogMessage();
    }
    QCOMPARE(pins.size(), pinsNewCertificate ? 1 : 0);
    if (pinsNewCertificate) {
        QCOMPARE(pins.first().first().toString(), presentedPin);
    }
    transport.stop();
}

void NTRIPTlsTest::_plaintextCasterFailsAsTlsError()
{
    // TLS enabled against a plaintext caster port fails the same way on every attempt, so it must not be retried.
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    auto configuration = caster.connectionConfig();
    configuration.useTls = true;
    NTRIPHttpSession session;
    QSignalSpy failed(&session, &NTRIPHttpSession::failed);
    QSignalSpy started(&session, &NTRIPHttpSession::responseStarted);
    expectLogMessage("GPS.NTRIP.NTRIPHttpSession", QtWarningMsg, QRegularExpression(QStringLiteral("^Socket error")));
    QVERIFY(session.open(configuration).isEmpty());
    auto* connection = caster.waitForConnection();
    QVERIFY(connection && connection->peer);
    const QByteArray plaintext = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
    QCOMPARE(connection->write(plaintext), plaintext.size());
    QTRY_COMPARE_WITH_TIMEOUT(failed.size(), 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    QCOMPARE(failed.first().first().value<NTRIPFailure>().code, NTRIPError::SslError);
    QVERIFY(started.isEmpty());
}

UT_REGISTER_TEST_LIGHTWEIGHT(NTRIPTlsTest, TestLabel::Unit)
