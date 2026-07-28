#include "LocalHttpTestServerTest.h"

#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QSignalSpy>

#include "Fixtures/LocalHttpTestServer.h"

void LocalHttpTestServerTest::_testEarlyRequest()
{
    TestFixtures::LocalHttpTestServer server;
    QVERIFY(server.listen());
    server.installHttpResponder(QByteArrayLiteral("ready"));

    QTcpSocket client;
    client.connectToHost(QHostAddress::LocalHost, server.port());
    QVERIFY(client.waitForConnected(TestTimeout::mediumMs()));

    QSignalSpy readyReadSpy(&client, &QTcpSocket::readyRead);
    const QByteArray firstRequestFragment =
        QByteArrayLiteral("GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n");
    QCOMPARE(client.write(firstRequestFragment), firstRequestFragment.size());
    QVERIFY(client.waitForBytesWritten(TestTimeout::mediumMs()));
    QVERIFY(!readyReadSpy.wait(TestTimeout::shortMs()));
    QCOMPARE(client.state(), QAbstractSocket::ConnectedState);

    const QByteArray finalRequestFragment = QByteArrayLiteral("\r\n");
    QCOMPARE(client.write(finalRequestFragment), finalRequestFragment.size());
    QVERIFY(client.waitForBytesWritten(TestTimeout::mediumMs()));

    QTRY_COMPARE_WITH_TIMEOUT(client.state(), QAbstractSocket::UnconnectedState, TestTimeout::mediumMs());
    const QByteArray response = client.readAll();
    QVERIFY(response.startsWith(QByteArrayLiteral("HTTP/1.1 200 OK\r\n")));
    QVERIFY(response.endsWith(QByteArrayLiteral("\r\n\r\nready")));
}

UT_REGISTER_TEST(LocalHttpTestServerTest, TestLabel::Unit)
