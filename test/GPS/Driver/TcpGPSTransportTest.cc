#include "TcpGPSTransportTest.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QTimer>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

#include "TcpGPSTransport.h"

void TcpGPSTransportTest::_transferTimeoutAndPeerClose()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    std::atomic_bool stop = false;
    TcpGPSTransport transport(QStringLiteral("localhost"), server.serverPort(), stop);
    QVERIFY(transport.open());
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::shortMs());
    auto* peer = server.nextPendingConnection();
    QVERIFY(peer);
    QVERIFY(transport.setBaudrate(115200));
    QCOMPARE(transport.fixedBaudrate(), 115200u);
    QVERIFY(!transport.setBaudrate(9600));
    QVERIFY(!transport.fatalError());

    const QByteArray payload = QByteArray::fromHex("b56201020300d300ff");
    QCOMPARE(peer->write(payload), payload.size());
    uint8_t buffer[64]{};
    const int received = transport.read(buffer, sizeof(buffer), TestTimeout::shortMs());
    QCOMPARE(received, payload.size());
    QCOMPARE(QByteArray(reinterpret_cast<char*>(buffer), received), payload);
    QCOMPARE(transport.write(reinterpret_cast<const uint8_t*>(payload.constData()), payload.size()), payload.size());
    QTRY_COMPARE_WITH_TIMEOUT(peer->bytesAvailable(), payload.size(), TestTimeout::shortMs());
    QCOMPARE(peer->readAll(), payload);

    // Exercise the transport timeout contract without spending the normal driver timeout.
    QCOMPARE(transport.read(buffer, sizeof(buffer), 20), 0);
    QVERIFY(!transport.fatalError());
    peer->abort();
    QCOMPARE(transport.read(buffer, sizeof(buffer), TestTimeout::shortMs()), -1);
    QVERIFY(transport.fatalError());
    QCOMPARE(transport.write(reinterpret_cast<const uint8_t*>(payload.constData()), payload.size()), -1);
}

void TcpGPSTransportTest::_cancelWait_data()
{
    QTest::addColumn<QString>("phase");
    QTest::newRow("before-open") << QStringLiteral("before-open");
    QTest::newRow("connecting") << QStringLiteral("connecting");
    QTest::newRow("read") << QStringLiteral("read");
    QTest::newRow("write") << QStringLiteral("write");
}

void TcpGPSTransportTest::_cancelWait()
{
    QFETCH(QString, phase);
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    std::atomic_bool stop = phase == QStringLiteral("before-open");
    TcpGPSTransport transport(QStringLiteral("localhost"), server.serverPort(), stop);
    QElapsedTimer elapsed;
    if (phase == QStringLiteral("before-open")) {
        QVERIFY(!transport.open());
    } else if (phase == QStringLiteral("connecting")) {
        QTimer::singleShot(0, &server, [&]() { stop = true; });
        elapsed.start();
        QVERIFY(!transport.open());
        QVERIFY(elapsed.elapsed() < TestTimeout::shortMs());
    } else {
        QVERIFY(transport.open());
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::shortMs());
        auto* peer = server.nextPendingConnection();
        QVERIFY(peer);
        peer->setReadBufferSize(1);
        QTimer::singleShot(0, &server, [&]() { stop = true; });
        elapsed.start();
        if (phase == QStringLiteral("read")) {
            uint8_t buffer[8]{};
            QCOMPARE(transport.read(buffer, sizeof(buffer), TestTimeout::longMs()), -1);
        } else {
            // Exceed the kernel send buffer to keep bytes pending until cancellation.
            const QByteArray payload(8 * 1024 * 1024, 'x');
            QCOMPARE(transport.write(reinterpret_cast<const uint8_t*>(payload.constData()), payload.size()), -1);
        }
        QVERIFY(elapsed.elapsed() < TestTimeout::shortMs());
    }
    QVERIFY(stop);
    QVERIFY(transport.isCancelled());
    uint8_t byte{};
    QCOMPARE(transport.read(&byte, 1, TestTimeout::shortMs()), -1);
    QCOMPARE(transport.write(&byte, 1), -1);
}

void TcpGPSTransportTest::_refusedConnection()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    const quint16 port = server.serverPort();
    server.close();
    std::atomic_bool stop = false;
    TcpGPSTransport transport(QStringLiteral("127.0.0.1"), port, stop);
    expectLogMessage("GPS.Driver.TcpGPSTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to connect to GPS receiver")));
    QVERIFY(!transport.open());
    verifyExpectedLogMessage();
    QVERIFY(transport.fatalError());
}

UT_REGISTER_TEST(TcpGPSTransportTest, TestLabel::Unit)
