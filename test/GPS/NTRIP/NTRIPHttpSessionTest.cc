#include <memory>

#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "NTRIPConfiguration.h"
#include "NTRIPHttpSession.h"
#include "UnitTest.h"

namespace {
NTRIPConnectionConfig localConfig(quint16 port)
{
    NTRIPConnectionConfig config;
    config.host = QStringLiteral("127.0.0.1");
    config.port = port;
    return config;
}
}  // namespace

class NTRIPHttpSessionTest : public UnitTest
{
    Q_OBJECT

private slots:
    void responseEndsWithClosed();
    void refusedConnectionFailsOnce();
    void abortIsSilent();
    void retireFromDeliveryDetachesOwner();
};

void NTRIPHttpSessionTest::responseEndsWithClosed()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    NTRIPHttpSession session;
    QByteArray received;
    QStringList events;
    connect(&session, &NTRIPHttpSession::established, this, [&]() {
        events << QStringLiteral("established");
        QVERIFY(session.isConnected());
        QVERIFY(session.write(QByteArrayLiteral("GET / HTTP/1.1\r\n\r\n")));
    });
    connect(&session, &NTRIPHttpSession::bytesReceived, this, [&](const QByteArray& bytes, qint64 receivedAtMs) {
        QVERIFY(receivedAtMs > 0);
        received += bytes;
    });
    connect(&session, &NTRIPHttpSession::closed, this, [&]() {
        events << QStringLiteral("closed:%1").arg(received.size());
        QVERIFY(!session.isConnected());
    });
    QSignalSpy failed(&session, &NTRIPHttpSession::failed);
    session.open(localConfig(server.serverPort()));
    session.open(localConfig(1));

    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::mediumMs());
    std::unique_ptr<QTcpSocket> peer(server.nextPendingConnection());
    QByteArray request;
    QTRY_VERIFY_WITH_TIMEOUT((request += peer->readAll()).endsWith("\r\n\r\n"), TestTimeout::mediumMs());
    const QByteArray response(3 * NTRIPHttpSession::kReadChunkBytes + 7, 'x');
    QCOMPARE(peer->write(response), response.size());
    peer->disconnectFromHost();

    QTRY_COMPARE_WITH_TIMEOUT(events.size(), 2, TestTimeout::mediumMs());
    QCOMPARE(events, (QStringList{QStringLiteral("established"), QStringLiteral("closed:%1").arg(response.size())}));
    QCOMPARE(received, response);
    QVERIFY(failed.isEmpty());
    QVERIFY(!session.write(QByteArrayLiteral("late")));
}

void NTRIPHttpSessionTest::refusedConnectionFailsOnce()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    const quint16 port = server.serverPort();
    server.close();
    ignoreLogMessage("GPS.NTRIP.NTRIPHttpSession", QtWarningMsg, QRegularExpression(QStringLiteral("^Socket error")));
    NTRIPHttpSession session;
    QSignalSpy established(&session, &NTRIPHttpSession::established);
    QSignalSpy failed(&session, &NTRIPHttpSession::failed);
    QSignalSpy closed(&session, &NTRIPHttpSession::closed);
    session.open(localConfig(port));
    QTRY_COMPARE_WITH_TIMEOUT(failed.size(), 1, TestTimeout::mediumMs());
    QCOMPARE(failed.first().first().value<NTRIPError>(), NTRIPError::SocketError);
    QVERIFY(!failed.first().at(1).toString().isEmpty());
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QCOMPARE(failed.size(), 1);
    QVERIFY(established.isEmpty());
    QVERIFY(closed.isEmpty());
}

void NTRIPHttpSessionTest::abortIsSilent()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    NTRIPHttpSession session;
    QSignalSpy established(&session, &NTRIPHttpSession::established);
    QSignalSpy failed(&session, &NTRIPHttpSession::failed);
    QSignalSpy closed(&session, &NTRIPHttpSession::closed);
    session.open(localConfig(server.serverPort()));
    QTRY_COMPARE_WITH_TIMEOUT(established.size(), 1, TestTimeout::mediumMs());
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::mediumMs());
    std::unique_ptr<QTcpSocket> peer(server.nextPendingConnection());
    session.abort();
    QVERIFY(!session.isConnected());
    QTRY_COMPARE_WITH_TIMEOUT(peer->state(), QAbstractSocket::UnconnectedState, TestTimeout::mediumMs());
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    QVERIFY(failed.isEmpty());
    QVERIFY(closed.isEmpty());
}

void NTRIPHttpSessionTest::retireFromDeliveryDetachesOwner()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    auto owner = std::make_unique<QObject>();
    QPointer<NTRIPHttpSession> session = new NTRIPHttpSession(owner.get());
    int deliveries = 0;
    int ownerCallbacks = 0;
    bool detached = false;
    connect(session, &NTRIPHttpSession::closed, owner.get(), [&]() { ++ownerCallbacks; });
    connect(session, &NTRIPHttpSession::bytesReceived, this, [&]() {
        ++deliveries;
        session->retire();
        detached = !session->parent();
        // Deleting the owner must not delete the retired session during its delivery.
        owner.reset();
    });
    session->open(localConfig(server.serverPort()));
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::mediumMs());
    std::unique_ptr<QTcpSocket> peer(server.nextPendingConnection());
    const QByteArray response(2 * NTRIPHttpSession::kReadChunkBytes, 'x');
    QCOMPARE(peer->write(response), response.size());
    peer->disconnectFromHost();
    QTRY_COMPARE_WITH_TIMEOUT(deliveries, 1, TestTimeout::mediumMs());
    QVERIFY(detached);
    QVERIFY(!owner);
    QTRY_VERIFY_WITH_TIMEOUT(!session, TestTimeout::mediumMs());
    QCOMPARE(deliveries, 1);
    QCOMPARE(ownerCallbacks, 0);
}

UT_REGISTER_TEST(NTRIPHttpSessionTest, TestLabel::Unit)
#include "NTRIPHttpSessionTest.moc"
