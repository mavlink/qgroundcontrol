#include "NTRIPHttpSessionTest.h"

#include <memory>

#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "MultiSignalSpy.h"
#include "NTRIP/Support/NTRIPTestHelpers.h"
#include "NTRIP/Support/ScriptedNTRIPCaster.h"
#include "NTRIPHttpSession.h"
#include "Support/GPSTestHelpers.h"

using namespace GPSTest;

namespace {
QByteArray requestFor(const NTRIPConnectionConfig& config)
{
    return NTRIPHttpRequest::build(config).bytes;
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
