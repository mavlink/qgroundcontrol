#include "NTRIPHttpSessionTest.h"

#include <memory>

#include <QtCore/QCoreApplication>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "MultiSignalSpy.h"
#include "NTRIP/Support/NTRIPTestHelpers.h"
#include "NTRIP/Support/ScriptedNTRIPCaster.h"
#include "NTRIPHttpSession.h"
#include "QGCNetworkClient.h"
#include "Support/GPSTestHelpers.h"

using namespace GPSTest;

namespace {
QByteArray requestFor(const NTRIPConnectionConfig& config)
{
    return NTRIPHttpRequest::build(config).bytes;
}

QString disconnectedBeforeResponse()
{
    return QCoreApplication::translate("NTRIPHttpTransport", "Caster disconnected before completing the HTTP response");
}
}  // namespace

void NTRIPHttpSessionTest::_responseEndsWithFinished()
{
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    NTRIPHttpSession session;
    QByteArray received;
    QStringList events;
    connect(&session, &NTRIPHttpSession::responseStarted, this, [&]() {
        events << QStringLiteral("started");
        QVERIFY(session.isConnected());
    });
    connect(&session, &NTRIPHttpSession::bodyReceived, this, [&](const QByteArray& bytes, qint64 receivedAtMs) {
        QVERIFY(receivedAtMs > 0);
        received += bytes;
    });
    connect(&session, &NTRIPHttpSession::finished, this, [&]() {
        events << QStringLiteral("finished:%1").arg(received.size());
        QVERIFY(!session.isConnected());
    });
    QSignalSpy failed(&session, &NTRIPHttpSession::failed);
    QVERIFY(session.open(caster.connectionConfig()).isEmpty());
    QVERIFY(session.open(connectionConfig(1)).isEmpty());

    auto* connection = caster.waitForConnection();
    QVERIFY(connection && connection->peer);
    QCOMPARE(connection->waitForRequest(), requestFor(caster.connectionConfig()));
    const QByteArray body(3 * NTRIPHttpSession::READ_CHUNK_BYTES + 7, 'x');
    const QByteArray response = "HTTP/1.1 200 OK\r\n\r\n" + body;
    QCOMPARE(connection->write(response), response.size());
    connection->disconnectFromHost();

    QTRY_COMPARE_WITH_TIMEOUT(events.size(), 2, TestTimeout::mediumMs());
    QCOMPARE(events, (QStringList{QStringLiteral("started"), QStringLiteral("finished:%1").arg(body.size())}));
    QCOMPARE(received, body);
    QVERIFY(failed.isEmpty());
    QVERIFY(!session.write(QByteArrayLiteral("late")));
}

void NTRIPHttpSessionTest::_invalidConfigDoesNotConnect()
{
    NTRIPHttpSession session;
    MultiSignalSpy spy;
    QVERIFY(spy.init(&session));
    const NTRIPConnectionConfig withoutMountpoint = connectionConfig(2101, QString());
    const QString error = session.open(withoutMountpoint);
    QVERIFY(!error.isEmpty());
    QCOMPARE(error, withoutMountpoint.streamValidationError());
    QVERIFY(!session.isConnected());
    deliverQueuedCalls();
    QVERIFY_NO_SIGNALS(spy);
}

void NTRIPHttpSessionTest::_refusedConnectionFailsOnce()
{
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    const quint16 port = caster.port();
    caster.close();
    ignoreLogMessage("GPS.NTRIP.NTRIPHttpSession", QtWarningMsg, QRegularExpression(QStringLiteral("^Socket error")));
    NTRIPHttpSession session;
    MultiSignalSpy spy;
    QVERIFY(spy.init(&session));
    QVERIFY(session.open(connectionConfig(port)).isEmpty());
    QVERIFY_WAIT_SIGNAL(spy, "failed", TestTimeout::mediumMs());
    const auto failure = spy.argument<NTRIPFailure>("failed");
    QCOMPARE(failure.code, NTRIPError::SocketError);
    QVERIFY(!failure.detail.isEmpty());
    deliverQueuedCalls();
    QVERIFY_ONLY_SIGNAL(spy, "failed");
}

void NTRIPHttpSessionTest::_unansweredRequestIsRepeatedInVersion1_data()
{
    QTest::addColumn<bool>("credentials");
    QTest::newRow("anonymous") << false;
    QTest::newRow("with-credentials") << true;
}

void NTRIPHttpSessionTest::_unansweredRequestIsRepeatedInVersion1()
{
    QFETCH(bool, credentials);
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    auto config = caster.connectionConfig();
    if (credentials) {
        config.username = QStringLiteral("user");
        config.password = QStringLiteral("pass");
    }
    NTRIPHttpSession session;
    MultiSignalSpy spy;
    QVERIFY(spy.init(&session));
    QByteArray received;
    connect(&session, &NTRIPHttpSession::bodyReceived, this, [&](const QByteArray& bytes) { received += bytes; });
    expectLogMessage("GPS.NTRIP.NTRIPHttpSession", QtWarningMsg, repeatedInVersion1());
    QVERIFY(session.open(config).isEmpty());

    auto* connection = caster.waitForVersion1Request();
    QVERIFY(connection && connection->peer);
    verifyExpectedLogMessage();
    QCOMPARE(caster.requestLines(), (QList<QByteArray>{"GET /TEST HTTP/1.1", "GET /TEST HTTP/1.0"}));
    // The request of RTKLIB's client (reqntrip_c in src/stream.c), which sends neither Host nor Ntrip-Version.
    QCOMPARE(connection->request,
             "GET /TEST HTTP/1.0\r\nUser-Agent: NTRIP " + QGCNetworkHelper::defaultUserAgent().toLatin1() + "\r\n" +
                 (credentials ? QByteArray("Authorization: Basic dXNlcjpwYXNz\r\n") : QByteArray()) + "\r\n");

    // Binary, as a stream is: printable text after the status line could still be a header.
    const QByteArray body(64, '\xd3');
    const QByteArray response = "ICY 200 OK\r\n" + body;
    QCOMPARE(connection->write(response), response.size());
    QTRY_COMPARE_WITH_TIMEOUT(received, body, TestTimeout::mediumMs());
    // The owner sees one exchange: the unanswered request reports nothing.
    QCOMPARE(spy.count("responseStarted"), 1);
    QVERIFY_NO_SIGNAL(spy, "failed");
    session.abort();
}

void NTRIPHttpSessionTest::_requestIsRepeatedOnlyOnce()
{
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    NTRIPHttpSession session;
    MultiSignalSpy spy;
    QVERIFY(spy.init(&session));
    // A second repeat would log again, which fails the test.
    expectLogMessage("GPS.NTRIP.NTRIPHttpSession", QtWarningMsg, repeatedInVersion1());
    QVERIFY(session.open(caster.connectionConfig()).isEmpty());
    auto* connection = caster.waitForVersion1Request();
    QVERIFY(connection && connection->peer);
    verifyExpectedLogMessage();
    connection->disconnectFromHost();
    QVERIFY_WAIT_SIGNAL(spy, "failed", TestTimeout::mediumMs());
    const auto failure = spy.argument<NTRIPFailure>("failed");
    QCOMPARE(failure.code, NTRIPError::InvalidHttpResponse);
    QCOMPARE(failure.detail, disconnectedBeforeResponse());
    deliverQueuedCalls();
    QVERIFY_ONLY_SIGNAL(spy, "failed");
    QCOMPARE(caster.requestLines().size(), 2);
}

void NTRIPHttpSessionTest::_answeredRequestIsNotRepeated()
{
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    NTRIPHttpSession session;
    MultiSignalSpy spy;
    QVERIFY(spy.init(&session));
    QVERIFY(session.open(caster.connectionConfig()).isEmpty());
    auto* connection = caster.waitForConnection();
    QVERIFY(connection && connection->peer);
    QCOMPARE(connection->waitForRequest(), requestFor(caster.connectionConfig()));
    // One byte of an answer is enough: the caster read the request, so another form of it is not tried. A repeat
    // would be refused, and would log.
    caster.close();
    QCOMPARE(connection->write("H"), 1);
    connection->disconnectFromHost();
    QVERIFY_WAIT_SIGNAL(spy, "failed", TestTimeout::mediumMs());
    const auto failure = spy.argument<NTRIPFailure>("failed");
    QCOMPARE(failure.code, NTRIPError::InvalidHttpResponse);
    QCOMPARE(failure.detail, disconnectedBeforeResponse());
    deliverQueuedCalls();
    QVERIFY_ONLY_SIGNAL(spy, "failed");
}

void NTRIPHttpSessionTest::_abortIsSilent()
{
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    NTRIPHttpSession session;
    MultiSignalSpy spy;
    QVERIFY(spy.init(&session));
    QVERIFY(session.open(caster.connectionConfig()).isEmpty());
    auto* connection = caster.waitForConnection();
    QVERIFY(connection && connection->peer);
    QCOMPARE(connection->waitForRequest(), requestFor(caster.connectionConfig()));
    session.abort();
    QVERIFY(!session.isConnected());
    QTRY_COMPARE_WITH_TIMEOUT(connection->peer->state(), QAbstractSocket::UnconnectedState, TestTimeout::mediumMs());
    deliverQueuedCalls();
    QVERIFY_NO_SIGNALS(spy);
}

void NTRIPHttpSessionTest::_retireFromDeliveryDetachesOwner()
{
    ScriptedNTRIPCaster caster;
    QVERIFY(caster.isListening());
    auto owner = std::make_unique<QObject>();
    QPointer<NTRIPHttpSession> session = new NTRIPHttpSession(owner.get());
    int deliveries = 0;
    int ownerCallbacks = 0;
    bool detached = false;
    connect(session, &NTRIPHttpSession::finished, owner.get(), [&]() { ++ownerCallbacks; });
    connect(session, &NTRIPHttpSession::bodyReceived, this, [&]() {
        ++deliveries;
        session->retire();
        detached = !session->parent();
        // Deleting the owner must not delete the retired session during its delivery.
        owner.reset();
    });
    QVERIFY(session->open(caster.connectionConfig()).isEmpty());
    auto* connection = caster.waitForConnection();
    QVERIFY(connection && connection->peer);
    const QByteArray response = "HTTP/1.1 200 OK\r\n\r\n" + QByteArray(2 * NTRIPHttpSession::READ_CHUNK_BYTES, 'x');
    QCOMPARE(connection->write(response), response.size());
    connection->disconnectFromHost();
    QTRY_COMPARE_WITH_TIMEOUT(deliveries, 1, TestTimeout::mediumMs());
    QVERIFY(detached);
    QVERIFY(!owner);
    QTRY_VERIFY_WITH_TIMEOUT(!session, TestTimeout::mediumMs());
    QCOMPARE(deliveries, 1);
    QCOMPARE(ownerCallbacks, 0);
}

UT_REGISTER_TEST_LIGHTWEIGHT(NTRIPHttpSessionTest, TestLabel::Unit)
