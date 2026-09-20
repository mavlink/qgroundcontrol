#include <array>
#include <memory>
#include <utility>

#include <QtCore/QDateTime>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslConfiguration>
#include <QtNetwork/QSslError>
#include <QtNetwork/QSslKey>
#include <QtNetwork/QSslServer>
#include <QtNetwork/QSslSocket>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "../RTCM/RTCMTestFixtures.h"
#include "NTRIPHttpTransport.h"
#include "NTRIPSourceTableController.h"
#include "NTRIPTlsPolicy_p.h"
#include "NTRIPTlsTestFixtures.h"
#include "RTCMDecodedFrame.h"
#include "UnitTest.h"

namespace {
int timeoutMs()
{
    return TestTimeout::mediumMs();
}

QSslConfiguration serverConfiguration(bool mismatched = false)
{
    using namespace NTRIPTlsTestFixtures;
    auto configuration = QSslConfiguration::defaultConfiguration();
    configuration.setLocalCertificate(QSslCertificate(mismatched ? MISMATCHED_CERT_PEM : SERVER_CERT_PEM, QSsl::Pem));
    configuration.setPrivateKey(QSslKey(PRIVATE_KEY_PEM, QSsl::Rsa, QSsl::Pem));
    configuration.setPeerVerifyMode(QSslSocket::VerifyNone);
    return configuration;
}

NTRIPConnectionConfig connectionConfig(const QSslServer& server)
{
    NTRIPConnectionConfig configuration;
    configuration.host = QStringLiteral("127.0.0.1");
    configuration.port = server.serverPort();
    configuration.mountpoint = QStringLiteral("TEST");
    configuration.useTls = true;
    return configuration;
}

QByteArray chunk(const QByteArray& bytes)
{
    return QByteArray::number(bytes.size(), 16) + "\r\n" + bytes + "\r\n";
}

QByteArray sourceTableResponse(bool chunked = false)
{
    const QByteArray body = "STR;MP;Id;RTCM 3.2;;2;GPS;NET;USA;40;-74;0;1;gen;none;B;N;4800\r\nENDSOURCETABLE\r\n";
    return chunked ? "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n" + chunk(body) + "0\r\n\r\n"
                   : "HTTP/1.1 200 OK\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body;
}
}  // namespace

class NTRIPTlsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;
    void selfSignedClassification_data();
    void selfSignedClassification();
    void certificatePolicy_data();
    void certificatePolicy();
    void sourceTablePolicyChanges_data();
    void sourceTablePolicyChanges();
    void retireAttempt_data();
    void retireAttempt();
    void restartRetiresAttempt_data();
    void restartRetiresAttempt();
    void reconnectFromTlsFailure();

private:
    void _expectTlsWarnings(bool allowSelfSigned, bool mismatched = false);
    void _verifyTlsWarnings();
};

void NTRIPTlsTest::initTestCase()
{
    UnitTest::initTestCase();
    if (!QSslSocket::supportsSsl()) {
        QSKIP("No TLS backend available");
    }
    using namespace NTRIPTlsTestFixtures;
    QVERIFY(!QSslKey(PRIVATE_KEY_PEM, QSsl::Rsa, QSsl::Pem).isNull());
    const auto now = QDateTime::currentDateTimeUtc();
    for (const auto& pem : {SERVER_CERT_PEM, MISMATCHED_CERT_PEM, ISSUED_CERT_PEM}) {
        const QSslCertificate certificate(pem, QSsl::Pem);
        QVERIFY(!certificate.isNull());
        QVERIFY2(certificate.effectiveDate() <= now, "Test certificate is not yet valid");
        QVERIFY2(certificate.expiryDate() > now, "Replace the expired test-only TLS certificate");
    }
}

void NTRIPTlsTest::_expectTlsWarnings(bool allowSelfSigned, bool mismatched)
{
    const auto tlsError = [](QSslError::SslError code) {
        return QRegularExpression::escape(QSslError(code).errorString());
    };
    const QRegularExpression selfSigned(
        QStringLiteral("^TLS error: \"(?:%1|%2)\"$")
            .arg(tlsError(QSslError::SelfSignedCertificate), tlsError(QSslError::SelfSignedCertificateInChain)));
    const QRegularExpression policy(
        mismatched
            ? QStringLiteral("^TLS error: \"%1\"$").arg(tlsError(QSslError::HostNameMismatch))
            : QRegularExpression::anchoredPattern(QRegularExpression::escape(
                  allowSelfSigned ? QStringLiteral("Accepting self-signed certificate (user opted in)")
                                  : QStringLiteral("Rejecting self-signed certificate (enable 'Accept self-signed "
                                                   "certificates' to allow)"))));
    expectLogMessage("GPS.NTRIPHttpTransport", QtWarningMsg, selfSigned);
    expectLogMessage("GPS.NTRIPHttpTransport", QtWarningMsg, policy);
}

void NTRIPTlsTest::_verifyTlsWarnings()
{
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
}

void NTRIPTlsTest::selfSignedClassification_data()
{
    using namespace NTRIPTlsTestFixtures;
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
        QTest::newRow((QByteArray(name) + "-self-signed").constData()) << QList{QSslError(code, selfSigned)} << true;
        QTest::newRow((QByteArray(name) + "-missing-certificate").constData()) << QList{QSslError(code)} << false;
        QTest::newRow((QByteArray(name) + "-issued").constData()) << QList{QSslError(code, issued)} << false;
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
        QTest::newRow((QByteArray(name) + "-after-allowed").constData()) << mixed << false;
        mixed.removeLast();
        mixed.prepend(fatal);
        QTest::newRow((QByteArray(name) + "-before-allowed").constData()) << mixed << false;
    }
    auto mixed = allowedErrors;
    mixed.append(QSslError(QSslError::CertificateUntrusted, issued));
    QTest::newRow("self-signed-and-unrelated-certificate") << mixed << false;
}

void NTRIPTlsTest::selfSignedClassification()
{
    QFETCH(QList<QSslError>, errors);
    QFETCH(bool, allowed);
    QCOMPARE(NTRIPTlsPolicy::isSelfSignedOnly(errors), allowed);
}

void NTRIPTlsTest::certificatePolicy_data()
{
    QTest::addColumn<bool>("allowSelfSigned");
    QTest::addColumn<bool>("mismatched");
    QTest::addColumn<bool>("chunked");
    QTest::addColumn<bool>("sourceTable");
    for (const bool sourceTable : {false, true}) {
        const QByteArray prefix = sourceTable ? "source-table-" : "stream-";
        QTest::newRow((prefix + "reject-self-signed-by-default").constData()) << false << false << false << sourceTable;
        QTest::newRow((prefix + "opt-in-identity").constData()) << true << false << false << sourceTable;
        QTest::newRow((prefix + "opt-in-chunked").constData()) << true << false << true << sourceTable;
        QTest::newRow((prefix + "opt-in-still-rejects-hostname-mismatch").constData())
            << true << true << false << sourceTable;
    }
}

void NTRIPTlsTest::certificatePolicy()
{
    QFETCH(bool, allowSelfSigned);
    QFETCH(bool, mismatched);
    QFETCH(bool, chunked);
    QFETCH(bool, sourceTable);
    QSslServer server;
    server.setSslConfiguration(serverConfiguration(mismatched));
    QVERIFY(server.listen(QHostAddress::LocalHost));
    auto configuration = connectionConfig(server);
    QVERIFY(!configuration.allowSelfSignedCerts);
    configuration.allowSelfSignedCerts = allowSelfSigned;
    if (sourceTable) {
        NTRIPSourceTableController controller;
        controller.fetch(configuration);
        if (!allowSelfSigned || mismatched) {
            QTRY_COMPARE_WITH_TIMEOUT(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::Error,
                                      timeoutMs());
            QVERIFY(!controller.fetchError().isEmpty());
            QCOMPARE(controller.mountpointModel()->rowCount(), 0);
            return;
        }
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), timeoutMs());
        std::unique_ptr<QSslSocket> peer(qobject_cast<QSslSocket*>(server.nextPendingConnection()));
        QVERIFY(peer && peer->isEncrypted());
        QByteArray request;
        QTRY_VERIFY_WITH_TIMEOUT((request += peer->readAll()).endsWith("\r\n\r\n"), timeoutMs());
        QVERIFY(request.startsWith("GET / HTTP/1.1\r\n"));
        const QByteArray response = sourceTableResponse(chunked);
        QCOMPARE(peer->write(response), response.size());
        QTRY_COMPARE_WITH_TIMEOUT(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::Success,
                                  timeoutMs());
        QCOMPARE(controller.mountpointModel()->rowCount(), 1);
        return;
    }
    NTRIPHttpTransport transport(configuration, {});
    QSignalSpy connected(&transport, &NTRIPTransport::connected);
    QSignalSpy frames(&transport, &NTRIPTransport::correctionFrameReceived);
    QSignalSpy errors(&transport, &NTRIPTransport::error);
    QSignalSpy plaintext(&transport, &NTRIPTransport::plaintextCredentialsWarning);
    _expectTlsWarnings(allowSelfSigned, mismatched);
    transport.start();

    if (!allowSelfSigned || mismatched) {
        QTRY_COMPARE_WITH_TIMEOUT(errors.size(), 1, timeoutMs());
        _verifyTlsWarnings();
        const auto failure = qvariant_cast<NTRIPFailure>(errors.first().first());
        QCOMPARE(failure.code, NTRIPError::SslError);
        if (mismatched) {
            QVERIFY(failure.detail.contains(QSslError(QSslError::HostNameMismatch).errorString()));
        } else {
            QVERIFY(failure.detail.contains(QStringLiteral("Self-signed certificate rejected")));
        }
        transport.stop();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
        QCOMPARE(errors.size(), 1);
        QVERIFY(connected.isEmpty());
        QVERIFY(frames.isEmpty());
        QVERIFY(plaintext.isEmpty());
        return;
    }

    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), timeoutMs());
    std::unique_ptr<QSslSocket> peer(qobject_cast<QSslSocket*>(server.nextPendingConnection()));
    QVERIFY(peer);
    QVERIFY(peer->isEncrypted());
    QByteArray request;
    QTRY_VERIFY_WITH_TIMEOUT((request += peer->readAll()).endsWith("\r\n\r\n"), timeoutMs());
    QVERIFY(request.startsWith("GET /TEST HTTP/1.1\r\n"));
    _verifyTlsWarnings();
    QVERIFY(connected.isEmpty());
    QVERIFY(frames.isEmpty());

    const QByteArray first = GpsTestHelpers::buildRtcmFrame(1005);
    const QByteArray second = GpsTestHelpers::buildRtcmFrame(1077);
    const QByteArray response = chunked ? "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n" + chunk(first)
                                        : "HTTP/1.1 200 OK\r\nContent-Type: gnss/data\r\n\r\n" + first;
    QCOMPARE(peer->write(response), response.size());
    QTRY_COMPARE_WITH_TIMEOUT(frames.size(), 1, timeoutMs());
    QCOMPARE(connected.size(), 1);
    const QByteArray continuation = chunked ? chunk(second) : second;
    QCOMPARE(peer->write(continuation), continuation.size());
    QTRY_COMPARE_WITH_TIMEOUT(frames.size(), 2, timeoutMs());
    for (qsizetype index = 0; index < frames.size(); ++index) {
        const auto frame = qvariant_cast<RTCMDecodedFrame>(frames[index][0]);
        QCOMPARE(frame.data, index == 0 ? first : second);
        QCOMPARE(frame.messageId, index == 0 ? 1005 : 1077);
        QVERIFY(frame.valid && !frame.filtered);
        QVERIFY(frame.receivedAtMs > 0);
    }
    QCOMPARE(connected.size(), 1);
    transport.stop();
    QTRY_COMPARE_WITH_TIMEOUT(peer->state(), QAbstractSocket::UnconnectedState, timeoutMs());
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QVERIFY(errors.isEmpty());
    QVERIFY(plaintext.isEmpty());
}

void NTRIPTlsTest::sourceTablePolicyChanges_data()
{
    QTest::addColumn<bool>("duringFetch");
    QTest::newRow("in-flight") << true;
    QTest::newRow("cached") << false;
}

void NTRIPTlsTest::sourceTablePolicyChanges()
{
    QFETCH(bool, duringFetch);
    QSslServer server;
    server.setSslConfiguration(serverConfiguration());
    QVERIFY(server.listen(QHostAddress::LocalHost));
    connect(&server, &QSslServer::pendingConnectionAvailable, &server, [&server]() {
        while (server.hasPendingConnections()) {
            auto* peer = server.nextPendingConnection();
            auto respond = [peer, request = QByteArray{}]() mutable {
                request += peer->readAll();
                if (request.endsWith("\r\n\r\n")) {
                    peer->write(sourceTableResponse());
                    request.clear();
                }
            };
            respond();
            connect(peer, &QTcpSocket::readyRead, peer, std::move(respond));
        }
    });
    auto configuration = connectionConfig(server);
    configuration.allowSelfSignedCerts = true;
    NTRIPSourceTableController controller;
    controller.fetch(configuration);
    if (!duringFetch) {
        QTRY_COMPARE_WITH_TIMEOUT(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::Success,
                                  timeoutMs());
        QCOMPARE(controller.mountpointModel()->rowCount(), 1);
    }

    configuration.allowSelfSignedCerts = false;
    controller.fetch(configuration);
    QTRY_COMPARE_WITH_TIMEOUT(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::Error, timeoutMs());
    QCOMPARE(controller.mountpointModel()->rowCount(), 0);

    configuration.allowSelfSignedCerts = true;
    controller.fetch(configuration);
    QTRY_COMPARE_WITH_TIMEOUT(controller.fetchStatus(), NTRIPSourceTableController::FetchStatus::Success, timeoutMs());
    QCOMPARE(controller.mountpointModel()->rowCount(), 1);
}

void NTRIPTlsTest::retireAttempt_data()
{
    QTest::addColumn<bool>("duringHandshake");
    QTest::addColumn<bool>("destroy");
    QTest::newRow("stop-during-handshake") << true << false;
    QTest::newRow("delete-during-handshake") << true << true;
    QTest::newRow("stop-before-http-response") << false << false;
    QTest::newRow("delete-before-http-response") << false << true;
}

void NTRIPTlsTest::retireAttempt()
{
    QFETCH(bool, duringHandshake);
    QFETCH(bool, destroy);
    QPointer<QSslSocket> retiredPeer;
    bool retired = false;
    QSslServer server;
    server.setSslConfiguration(serverConfiguration());
    QVERIFY(server.listen(QHostAddress::LocalHost));
    auto configuration = connectionConfig(server);
    configuration.allowSelfSignedCerts = true;
    auto transport = std::make_unique<NTRIPHttpTransport>(configuration, NTRIPRtcmFilterConfig{});
    QPointer<NTRIPHttpTransport> alive = transport.get();
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
        connect(&server, &QSslServer::startedEncryptionHandshake, transport.get(), [&](QSslSocket* peer) {
            retiredPeer = peer;
            QVERIFY(!peer->isEncrypted());
            retire();
        });
    } else {
        _expectTlsWarnings(true);
    }
    transport->start();
    if (!duringHandshake) {
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), timeoutMs());
        retiredPeer = qobject_cast<QSslSocket*>(server.nextPendingConnection());
        QVERIFY(retiredPeer);
        QVERIFY(retiredPeer->isEncrypted());
        QByteArray request;
        QTRY_VERIFY_WITH_TIMEOUT((request += retiredPeer->readAll()).endsWith("\r\n\r\n"), timeoutMs());
        _verifyTlsWarnings();
        QVERIFY(connected.isEmpty());
        retire();
    }
    QTRY_VERIFY_WITH_TIMEOUT(retired, timeoutMs());
    QTRY_VERIFY_WITH_TIMEOUT(!retiredPeer || retiredPeer->state() == QAbstractSocket::UnconnectedState, timeoutMs());
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QCOMPARE(alive.isNull(), destroy);
    QVERIFY(connected.isEmpty());
    QVERIFY(frames.isEmpty());
    QVERIFY(errors.isEmpty());
}

void NTRIPTlsTest::restartRetiresAttempt_data()
{
    QTest::addColumn<bool>("duringHandshake");
    QTest::addColumn<bool>("stopFirst");
    QTest::newRow("restart-during-handshake") << true << false;
    QTest::newRow("stop-start-during-handshake") << true << true;
    QTest::newRow("restart-before-http-response") << false << false;
    QTest::newRow("stop-start-before-http-response") << false << true;
}

void NTRIPTlsTest::restartRetiresAttempt()
{
    QFETCH(bool, duringHandshake);
    QFETCH(bool, stopFirst);
    QPointer<QSslSocket> retiredPeer;
    bool restarted = false;
    int handshakes = 0;
    QSslServer server;
    server.setSslConfiguration(serverConfiguration());
    QVERIFY(server.listen(QHostAddress::LocalHost));
    auto configuration = connectionConfig(server);
    configuration.allowSelfSignedCerts = true;
    NTRIPHttpTransport transport(configuration, {});
    QSignalSpy connected(&transport, &NTRIPTransport::connected);
    QSignalSpy frames(&transport, &NTRIPTransport::correctionFrameReceived);
    QSignalSpy errors(&transport, &NTRIPTransport::error);
    const auto restart = [&]() {
        restarted = true;
        if (stopFirst) {
            transport.stop();
        }
        transport.start();
    };
    connect(&server, &QSslServer::startedEncryptionHandshake, &transport, [&](QSslSocket* peer) {
        if (++handshakes == 1 && duringHandshake) {
            retiredPeer = peer;
            QVERIFY(!peer->isEncrypted());
            restart();
        }
    });
    _expectTlsWarnings(true);
    transport.start();
    if (!duringHandshake) {
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), timeoutMs());
        retiredPeer = qobject_cast<QSslSocket*>(server.nextPendingConnection());
        QVERIFY(retiredPeer);
        QByteArray request;
        QTRY_VERIFY_WITH_TIMEOUT((request += retiredPeer->readAll()).endsWith("\r\n\r\n"), timeoutMs());
        _verifyTlsWarnings();
        // Queue an old-attempt response without dispatching the client's readyRead before restart.
        const QByteArray stale = "HTTP/1.1 200 OK\r\n\r\n" + GpsTestHelpers::buildRtcmFrame(1005);
        QCOMPARE(retiredPeer->write(stale), stale.size());
        retiredPeer->flush();
        _expectTlsWarnings(true);
        restart();
    }
    QTRY_VERIFY_WITH_TIMEOUT(restarted && server.hasPendingConnections(), timeoutMs());
    std::unique_ptr<QSslSocket> peer(qobject_cast<QSslSocket*>(server.nextPendingConnection()));
    QVERIFY(peer);
    QVERIFY(peer.get() != retiredPeer.data());
    QVERIFY(peer->isEncrypted());
    QByteArray request;
    QTRY_VERIFY_WITH_TIMEOUT((request += peer->readAll()).endsWith("\r\n\r\n"), timeoutMs());
    QVERIFY(request.startsWith("GET /TEST HTTP/1.1\r\n"));
    _verifyTlsWarnings();
    QTRY_VERIFY_WITH_TIMEOUT(!retiredPeer || retiredPeer->state() == QAbstractSocket::UnconnectedState, timeoutMs());
    QCOMPARE(handshakes, 2);
    QVERIFY(connected.isEmpty());
    QVERIFY(frames.isEmpty());
    QVERIFY(errors.isEmpty());

    const QByteArray current = GpsTestHelpers::buildRtcmFrame(1077);
    const QByteArray response = "HTTP/1.1 200 OK\r\n\r\n" + current;
    QCOMPARE(peer->write(response), response.size());
    QTRY_COMPARE_WITH_TIMEOUT(frames.size(), 1, timeoutMs());
    const auto frame = qvariant_cast<RTCMDecodedFrame>(frames.first().first());
    QCOMPARE(frame.data, current);
    QVERIFY(frame.valid && !frame.filtered);
    QCOMPARE(connected.size(), 1);
    transport.stop();
    QTRY_COMPARE_WITH_TIMEOUT(peer->state(), QAbstractSocket::UnconnectedState, timeoutMs());
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QCOMPARE(frames.size(), 1);
    QVERIFY(errors.isEmpty());
}

void NTRIPTlsTest::reconnectFromTlsFailure()
{
    bool restarted = false;
    QSslServer server;
    server.setSslConfiguration(serverConfiguration(true));
    QVERIFY(server.listen(QHostAddress::LocalHost));
    auto configuration = connectionConfig(server);
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
        server.setSslConfiguration(serverConfiguration());
        _expectTlsWarnings(true);
        transport.start();
    });
    _expectTlsWarnings(true, true);
    transport.start();
    QTRY_VERIFY_WITH_TIMEOUT(restarted && server.hasPendingConnections(), timeoutMs());
    std::unique_ptr<QSslSocket> peer(qobject_cast<QSslSocket*>(server.nextPendingConnection()));
    QVERIFY(peer);
    QVERIFY(peer->isEncrypted());
    QByteArray request;
    QTRY_VERIFY_WITH_TIMEOUT((request += peer->readAll()).endsWith("\r\n\r\n"), timeoutMs());
    _verifyTlsWarnings();
    QVERIFY(connected.isEmpty());
    QCOMPARE(errors.size(), 1);
    const QByteArray expected = GpsTestHelpers::buildRtcmFrame(1005);
    const QByteArray response = "HTTP/1.1 200 OK\r\n\r\n" + expected;
    QCOMPARE(peer->write(response), response.size());
    QTRY_COMPARE_WITH_TIMEOUT(frames.size(), 1, timeoutMs());
    QCOMPARE(qvariant_cast<RTCMDecodedFrame>(frames.first().first()).data, expected);
    QCOMPARE(connected.size(), 1);
    transport.stop();
    QTRY_COMPARE_WITH_TIMEOUT(peer->state(), QAbstractSocket::UnconnectedState, timeoutMs());
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QCOMPARE(errors.size(), 1);
}

UT_REGISTER_TEST(NTRIPTlsTest, TestLabel::Unit)
#include "NTRIPTlsTest.moc"
