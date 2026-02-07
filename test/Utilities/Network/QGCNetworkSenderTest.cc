#include "QGCNetworkSenderTest.h"
#include "QGCNetworkSender.h"

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QUdpSocket>
#include <QtCore/QElapsedTimer>
#include <QtCore/QtEndian>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

UT_REGISTER_TEST(QGCNetworkSenderTest, TestLabel::Unit, TestLabel::Utilities, TestLabel::Network)

void QGCNetworkSenderTest::init()
{
    UnitTest::init();

    _sender = new QGCNetworkSender(this);

    // Set up UDP receiver
    _udpReceiver = new QUdpSocket(this);
    QVERIFY(_udpReceiver->bind(QHostAddress::LocalHost, 0));
    _udpPort = _udpReceiver->localPort();

    // Set up TCP server
    _tcpServer = new QTcpServer(this);
    QVERIFY(_tcpServer->listen(QHostAddress::LocalHost, 0));
    _tcpPort = _tcpServer->serverPort();
}

void QGCNetworkSenderTest::cleanup()
{
    delete _sender;
    _sender = nullptr;

    delete _udpReceiver;
    _udpReceiver = nullptr;

    delete _tcpServer;
    _tcpServer = nullptr;

    UnitTest::cleanup();
}

void QGCNetworkSenderTest::_testInitialState()
{
    QVERIFY(!_sender->isRunning());
    QVERIFY(!_sender->isConnected());
    QVERIFY(!_sender->hasError());
    QVERIFY(_sender->lastSendSucceeded());
    QCOMPARE(_sender->protocol(), QGCNetworkSender::Protocol::UDP);
    QVERIFY(_sender->host().isNull());
    QCOMPARE(_sender->port(), quint16(0));
}

void QGCNetworkSenderTest::_testSetEndpoint()
{
    _sender->setEndpoint(QHostAddress::LocalHost, 12345);
    QCOMPARE(_sender->host(), QHostAddress(QHostAddress::LocalHost));
    QCOMPARE(_sender->port(), quint16(12345));
}

void QGCNetworkSenderTest::_testSetProtocol()
{
    QCOMPARE(_sender->protocol(), QGCNetworkSender::Protocol::UDP);

    _sender->setProtocol(QGCNetworkSender::Protocol::TCP);
    QCOMPARE(_sender->protocol(), QGCNetworkSender::Protocol::TCP);

    _sender->setProtocol(QGCNetworkSender::Protocol::TLS);
    QCOMPARE(_sender->protocol(), QGCNetworkSender::Protocol::TLS);
}

void QGCNetworkSenderTest::_testStartStop()
{
    QVERIFY(!_sender->isRunning());

    _sender->start();
    QVERIFY(_sender->isRunning());

    _sender->stop();
    QVERIFY(!_sender->isRunning());
}

void QGCNetworkSenderTest::_testSendUdp()
{
    _sender->setEndpoint(QHostAddress::LocalHost, _udpPort);
    _sender->setProtocol(QGCNetworkSender::Protocol::UDP);
    _sender->start();

    QSignalSpy dataSentSpy(_sender, &QGCNetworkSender::dataSent);

    const QByteArray testData = "Hello UDP!";
    _sender->send(testData);
    _sender->flush();

    // Wait for data
    QVERIFY(_udpReceiver->waitForReadyRead(1000));

    QByteArray received;
    while (_udpReceiver->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(_udpReceiver->pendingDatagramSize());
        _udpReceiver->readDatagram(datagram.data(), datagram.size());
        received.append(datagram);
    }

    QCOMPARE(received, testData);
    QVERIFY(!dataSentSpy.isEmpty());

    _sender->stop();
}

void QGCNetworkSenderTest::_testSendTcp()
{
    _sender->setEndpoint(QHostAddress::LocalHost, _tcpPort);
    _sender->setProtocol(QGCNetworkSender::Protocol::TCP);
    _sender->setConnectTimeout(1000);
    _sender->start();

    QSignalSpy connectedSpy(_sender, &QGCNetworkSender::connected);

    const QByteArray testData = "Hello TCP!";
    _sender->send(testData);

    // Wait for connection
    QVERIFY(_tcpServer->waitForNewConnection(2000));
    QTcpSocket* client = _tcpServer->nextPendingConnection();
    QVERIFY(client);

    // Wait for data
    QVERIFY(client->waitForReadyRead(1000));
    QByteArray received;
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 2000) {
        if (client->bytesAvailable() > 0) {
            received.append(client->readAll());
        } else {
            client->waitForReadyRead(50);
        }
        if (received.size() >= 4) {
            const quint32 frameLen = qFromBigEndian<quint32>(received.constData());
            if (received.size() >= static_cast<int>(4 + frameLen)) {
                break;
            }
        }
    }

    // TCP uses 4-byte big-endian length-prefix framing.
    QVERIFY(received.size() >= 4);
    const quint32 frameLen = qFromBigEndian<quint32>(received.constData());
    QCOMPARE(static_cast<int>(frameLen), received.size() - 4);
    QCOMPARE(received.mid(4), testData);

    // Should have received connected signal
    QVERIFY(connectedSpy.wait(1000) || !connectedSpy.isEmpty());
    QVERIFY(_sender->isConnected());

    _sender->stop();
    delete client;
}

void QGCNetworkSenderTest::_testTcpReconnect()
{
    _sender->setEndpoint(QHostAddress::LocalHost, _tcpPort);
    _sender->setProtocol(QGCNetworkSender::Protocol::TCP);
    _sender->setConnectTimeout(500);
    _sender->setReconnectInterval(100);
    _sender->start();

    // First connection
    _sender->send("test1");
    QVERIFY(_tcpServer->waitForNewConnection(2000));
    QTcpSocket* client1 = _tcpServer->nextPendingConnection();
    QVERIFY(client1);
    QVERIFY(client1->waitForReadyRead(1000));

    // Disconnect client
    client1->close();
    delete client1;

    QTest::qWait(200); // Wait for disconnect detection

    // Send more data - should trigger reconnect
    _sender->send("test2");

    // Wait for reconnection
    QVERIFY(_tcpServer->waitForNewConnection(2000));
    QTcpSocket* client2 = _tcpServer->nextPendingConnection();
    QVERIFY(client2);
    QVERIFY(client2->waitForReadyRead(1000));

    _sender->stop();
    delete client2;
}

void QGCNetworkSenderTest::_testTransientErrorDoesNotBlockSend()
{
    _sender->setEndpoint(QHostAddress::LocalHost, quint16(1)); // Expected to fail
    _sender->setProtocol(QGCNetworkSender::Protocol::TCP);
    _sender->setConnectTimeout(200);
    _sender->setReconnectInterval(100);
    _sender->start();

    QSignalSpy errorSpy(_sender, &QGCNetworkSender::errorOccurred);
    _sender->send("will-fail");
    _sender->flush();

    QVERIFY(errorSpy.wait(1000) || !errorSpy.isEmpty());
    QVERIFY(_sender->hasError());

    // Switch to UDP and verify we can still send while hasError() is true.
    _sender->setProtocol(QGCNetworkSender::Protocol::UDP);
    _sender->setEndpoint(QHostAddress::LocalHost, _udpPort);

    const QByteArray recoveryData = "recovery";
    _sender->send(recoveryData);
    _sender->flush();

    QVERIFY(_udpReceiver->waitForReadyRead(1000));
    QByteArray datagram;
    datagram.resize(_udpReceiver->pendingDatagramSize());
    _udpReceiver->readDatagram(datagram.data(), datagram.size());
    QCOMPARE(datagram, recoveryData);

    _sender->stop();
}

void QGCNetworkSenderTest::_testMaxPendingBytes()
{
    _sender->setMaxPendingBytes(100);
    QCOMPARE(_sender->maxPendingBytes(), qint64(100));

    // Don't start - just verify setting
}

void QGCNetworkSenderTest::_testTlsConfiguration()
{
    QVERIFY(_sender->tlsVerifyPeer()); // Default true

    _sender->setTlsVerifyPeer(false);
    QVERIFY(!_sender->tlsVerifyPeer());

    _sender->setTlsVerifyPeer(true);
    QVERIFY(_sender->tlsVerifyPeer());
}
