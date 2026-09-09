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
    QCOMPARE(transport.open().status, GPSTransport::OpenStatus::Opened);
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
    const int received = transport.read(buffer, sizeof(buffer), TestTimeout::shortMs()).bytesRead;
    QCOMPARE(received, payload.size());
    QCOMPARE(QByteArray(reinterpret_cast<char*>(buffer), received), payload);
    QCOMPARE(transport.write(reinterpret_cast<const uint8_t*>(payload.constData()), payload.size()).writtenBytes,
             payload.size());
    QTRY_COMPARE_WITH_TIMEOUT(peer->bytesAvailable(), payload.size(), TestTimeout::shortMs());
    QCOMPARE(peer->readAll(), payload);

    // Exercise the transport timeout contract without spending the normal driver timeout.
    QCOMPARE(transport.read(buffer, sizeof(buffer), 20).bytesRead, 0);
    QVERIFY(!transport.fatalError());
    peer->abort();
    QVERIFY(transport.read(buffer, sizeof(buffer), TestTimeout::shortMs()).status != GPSTransport::ReadStatus::Data);
    QVERIFY(transport.fatalError());
    QVERIFY(transport.write(reinterpret_cast<const uint8_t*>(payload.constData()), payload.size()).status !=
            GPSTransport::WriteStatus::Completed);
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
        QVERIFY(transport.open().status != GPSTransport::OpenStatus::Opened);
    } else if (phase == QStringLiteral("connecting")) {
        QTimer::singleShot(0, &server, [&]() { stop = true; });
        elapsed.start();
        QVERIFY(transport.open().status != GPSTransport::OpenStatus::Opened);
        QVERIFY(elapsed.elapsed() < TestTimeout::shortMs());
    } else {
        QCOMPARE(transport.open().status, GPSTransport::OpenStatus::Opened);
        QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::shortMs());
        auto* peer = server.nextPendingConnection();
        QVERIFY(peer);
        peer->setReadBufferSize(1);
        QTimer::singleShot(0, &server, [&]() { stop = true; });
        elapsed.start();
        if (phase == QStringLiteral("read")) {
            uint8_t buffer[8]{};
            QVERIFY(transport.read(buffer, sizeof(buffer), TestTimeout::longMs()).status !=
                    GPSTransport::ReadStatus::Data);
        } else {
            // Exceed the kernel send buffer to keep bytes pending until cancellation.
            const QByteArray payload(8 * 1024 * 1024, 'x');
            QVERIFY(transport.write(reinterpret_cast<const uint8_t*>(payload.constData()), payload.size()).status !=
                    GPSTransport::WriteStatus::Completed);
        }
        QVERIFY(elapsed.elapsed() < TestTimeout::shortMs());
    }
    QVERIFY(stop);
    QVERIFY(transport.isCancelled());
    uint8_t byte{};
    QVERIFY(transport.read(&byte, 1, TestTimeout::shortMs()).status != GPSTransport::ReadStatus::Data);
    QVERIFY(transport.write(&byte, 1).status != GPSTransport::WriteStatus::Completed);
}

void TcpGPSTransportTest::_refusedConnection()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    const quint16 port = server.serverPort();
    server.close();
    std::atomic_bool stop = false;
    TcpGPSTransport transport(QStringLiteral("127.0.0.1"), port, stop);
    expectLogMessage("GPS.Driver.Transport.TcpGPSTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to connect to GPS receiver")));
    QVERIFY(transport.open().status != GPSTransport::OpenStatus::Opened);
    verifyExpectedLogMessage();
    QVERIFY(transport.fatalError());
}

void TcpGPSTransportTest::_boundedWriteEvidence_data()
{
    QTest::addColumn<bool>("cancel");
    QTest::newRow("deadline") << false;
    QTest::newRow("cancelled-after-acceptance") << true;
}

void TcpGPSTransportTest::_boundedWriteEvidence()
{
    QFETCH(bool, cancel);
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    std::atomic_bool stop = false;
    TcpGPSTransport transport(QStringLiteral("localhost"), server.serverPort(), stop);
    QCOMPARE(transport.open().status, GPSTransport::OpenStatus::Opened);
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::shortMs());
    auto* peer = server.nextPendingConnection();
    QVERIFY(peer);
    peer->setReadBufferSize(1);
    const QByteArray payload(8 * 1024 * 1024, 'x');
    if (cancel) {
        QTimer::singleShot(0, &server, [&]() { stop = true; });
    }
    QElapsedTimer elapsed;
    elapsed.start();
    const auto result = transport.writeBounded(reinterpret_cast<const uint8_t*>(payload.constData()), payload.size(),
                                               QDeadlineTimer(100));
    QCOMPARE(result.status, cancel ? GPSTransport::WriteStatus::Cancelled : GPSTransport::WriteStatus::TimedOut);
    QCOMPARE(result.acceptedBytes, payload.size());
    QVERIFY(result.uncertainBytes > 0);
    QCOMPARE(result.writtenBytes + result.uncertainBytes, result.acceptedBytes);
    QVERIFY(elapsed.elapsed() < 1000);
    QCOMPARE(stop.load(), cancel);
}

UT_REGISTER_TEST(TcpGPSTransportTest, TestLabel::Unit)

void TcpGPSTransportTest::_boundedIngressPreservesStream()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    std::atomic_bool stop = false;
    TcpGPSTransport transport(QStringLiteral("localhost"), server.serverPort(), stop);
    QCOMPARE(transport.open().status, GPSTransport::OpenStatus::Opened);
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::shortMs());
    auto* peer = server.nextPendingConnection();
    QVERIFY(peer);
    QByteArray payload(4 * TcpGPSTransport::kReadBufferBytes, Qt::Uninitialized);
    for (qsizetype i = 0; i < payload.size(); ++i) {
        payload[i] = static_cast<char>(i % 251);
    }
    QCOMPARE(peer->write(payload), payload.size());
    // Stall the decoder while Qt services input, as it does during configuration/write waits.
    QTRY_COMPARE_WITH_TIMEOUT(transport._socket->bytesAvailable(), TcpGPSTransport::kReadBufferBytes,
                              TestTimeout::shortMs());
    QCOMPARE(transport._socket->readBufferSize(), TcpGPSTransport::kReadBufferBytes);
    QVERIFY(!transport.fatalError());
    QByteArray received;
    uint8_t bytes[4096]{};
    const QDeadlineTimer deadline(TestTimeout::mediumMs());
    while (received.size() < payload.size() && !deadline.hasExpired()) {
        const auto result = transport.read(bytes, sizeof(bytes), 100);
        QVERIFY(result.status == GPSTransport::ReadStatus::Data || result.status == GPSTransport::ReadStatus::TimedOut);
        received.append(reinterpret_cast<const char*>(bytes), result.bytesRead);
        QVERIFY(transport._socket->bytesAvailable() <= TcpGPSTransport::kReadBufferBytes);
    }
    QCOMPARE(received, payload);
}
