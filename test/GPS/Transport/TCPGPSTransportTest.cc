#include "TCPGPSTransportTest.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QSemaphore>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

#include <memory>

#include "TCPGPSTransport.h"

void TCPGPSTransportTest::_transferTimeoutAndPeerClose()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    std::atomic_bool stop = false;
    TCPGPSTransport transport(QStringLiteral("localhost"), server.serverPort(), stop);
    uint8_t buffer[64]{};
    QCOMPARE(transport.read(buffer, 0, 0).status, GPSTransport::ReadStatus::Closed);
    QCOMPARE(transport.open().status, GPSTransport::OpenStatus::Opened);
    QCOMPARE(transport.read(buffer, 0, 0).status, GPSTransport::ReadStatus::Data);
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::shortMs());
    auto* peer = server.nextPendingConnection();
    QVERIFY(peer);
    QVERIFY(transport.setBaudrate(115200));
    QCOMPARE(transport.fixedBaudrate(), 115200u);
    QVERIFY(!transport.setBaudrate(9600));
    QVERIFY(!transport.fatalError());

    const QByteArray payload = QByteArray::fromHex("b56201020300d300ff");
    QCOMPARE(peer->write(payload), payload.size());
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
    // Preserve the final buffered bytes even after the peer's orderly disconnect is observed.
    QCOMPARE(peer->write(payload), payload.size());
    peer->disconnectFromHost();
    QTRY_VERIFY_WITH_TIMEOUT(transport.fatalError(), TestTimeout::shortMs());
    QCOMPARE(transport.read(buffer, 0, 0).status, GPSTransport::ReadStatus::Closed);
    const auto finalRead = transport.read(buffer, sizeof(buffer), 0);
    QCOMPARE(finalRead.status, GPSTransport::ReadStatus::Data);
    QCOMPARE(finalRead.bytesRead, payload.size());
    QCOMPARE(QByteArray(reinterpret_cast<char*>(buffer), finalRead.bytesRead), payload);
    QCOMPARE(transport.read(buffer, sizeof(buffer), 0).status, GPSTransport::ReadStatus::Closed);
    QVERIFY(transport.write(reinterpret_cast<const uint8_t*>(payload.constData()), payload.size()).status !=
            GPSTransport::WriteStatus::Completed);
}

void TCPGPSTransportTest::_cancelWait_data()
{
    QTest::addColumn<QString>("phase");
    QTest::newRow("before-open") << QStringLiteral("before-open");
    QTest::newRow("connecting") << QStringLiteral("connecting");
    QTest::newRow("read") << QStringLiteral("read");
    QTest::newRow("write") << QStringLiteral("write");
}

void TCPGPSTransportTest::_cancelWait()
{
    QFETCH(QString, phase);
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    std::atomic_bool stop = phase == QStringLiteral("before-open");
    TCPGPSTransport transport(QStringLiteral("localhost"), server.serverPort(), stop);
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

void TCPGPSTransportTest::_refusedConnection()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    const quint16 port = server.serverPort();
    server.close();
    std::atomic_bool stop = false;
    TCPGPSTransport transport(QStringLiteral("127.0.0.1"), port, stop);
    expectLogMessage("GPS.Transport.TCPGPSTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to connect to GPS receiver")));
    QVERIFY(transport.open().status != GPSTransport::OpenStatus::Opened);
    verifyExpectedLogMessage();
    QVERIFY(transport.fatalError());
    uint8_t byte{};
    QCOMPARE(transport.read(&byte, 0, 0).status, GPSTransport::ReadStatus::Closed);
}

void TCPGPSTransportTest::_boundedWriteEvidence_data()
{
    QTest::addColumn<bool>("cancel");
    QTest::newRow("deadline") << false;
    QTest::newRow("cancelled-after-acceptance") << true;
}

void TCPGPSTransportTest::_boundedWriteEvidence()
{
    QFETCH(bool, cancel);
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    std::atomic_bool stop = false;
    TCPGPSTransport transport(QStringLiteral("localhost"), server.serverPort(), stop);
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
    QVERIFY(result.acceptedBytes > 0);
    QVERIFY(result.acceptedBytes < payload.size());
    QVERIFY(result.uncertainBytes >= 0);
    QVERIFY(result.uncertainBytes <= TCPGPSTransport::kWriteBufferBytes);
    QVERIFY(transport.fatalError());
    QCOMPARE(
        transport
            .writeBounded(reinterpret_cast<const uint8_t*>(payload.constData()), payload.size(), QDeadlineTimer(100))
            .acceptedBytes,
        0);
    QCOMPARE(result.writtenBytes + result.uncertainBytes, result.acceptedBytes);
    QVERIFY(elapsed.elapsed() < 1000);
    QCOMPARE(stop.load(), cancel);
}

UT_REGISTER_TEST(TCPGPSTransportTest, TestLabel::Unit)

void TCPGPSTransportTest::_boundedIngressPreservesStream()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    std::atomic_bool stop = false;
    TCPGPSTransport transport(QStringLiteral("localhost"), server.serverPort(), stop);
    QCOMPARE(transport.open().status, GPSTransport::OpenStatus::Opened);
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::shortMs());
    auto* peer = server.nextPendingConnection();
    QVERIFY(peer);
    QByteArray payload(4 * TCPGPSTransport::kReadBufferBytes, Qt::Uninitialized);
    for (qsizetype i = 0; i < payload.size(); ++i) {
        payload[i] = static_cast<char>(i % 251);
    }
    QCOMPARE(peer->write(payload), payload.size());
    // Stall the decoder while Qt services input, as it does during configuration/write waits.
    QTRY_COMPARE_WITH_TIMEOUT(transport._socket->bytesAvailable(), TCPGPSTransport::kReadBufferBytes,
                              TestTimeout::shortMs());
    QCOMPARE(transport._socket->readBufferSize(), TCPGPSTransport::kReadBufferBytes);
    QVERIFY(!transport.fatalError());
    QByteArray received;
    uint8_t bytes[4096]{};
    const QDeadlineTimer deadline(TestTimeout::mediumMs());
    while (received.size() < payload.size() && !deadline.hasExpired()) {
        const auto result = transport.read(bytes, sizeof(bytes), 100);
        QVERIFY(result.status == GPSTransport::ReadStatus::Data || result.status == GPSTransport::ReadStatus::TimedOut);
        received.append(reinterpret_cast<const char*>(bytes), result.bytesRead);
        QVERIFY(transport._socket->bytesAvailable() <= TCPGPSTransport::kReadBufferBytes);
    }
    QCOMPARE(received, payload);
}

void TCPGPSTransportTest::_immediateRead_data()
{
    QTest::addColumn<int>("timeout");
    QTest::newRow("zero") << 0;
    QTest::newRow("negative") << -1;
}

void TCPGPSTransportTest::_immediateRead()
{
    QFETCH(int, timeout);
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    std::atomic_bool stop = false;
    TCPGPSTransport transport(QStringLiteral("127.0.0.1"), server.serverPort(), stop);
    QCOMPARE(transport.open().status, GPSTransport::OpenStatus::Opened);
    if (!server.hasPendingConnections()) {
        QVERIFY(server.waitForNewConnection(TestTimeout::shortMs()));
    }
    auto* peer = server.nextPendingConnection();
    QVERIFY(peer);
    QCOMPARE(peer->write("reply", 5), 5);
    QVERIFY(peer->waitForBytesWritten(TestTimeout::shortMs()));
    // Do not dispatch receiver events before polling; the bytes are still in the kernel.
    uint8_t bytes[16]{};
    const auto result = transport.read(bytes, sizeof(bytes), timeout);
    QCOMPARE(result.status, GPSTransport::ReadStatus::Data);
    QCOMPARE(QByteArray(reinterpret_cast<char*>(bytes), result.bytesRead), QByteArray("reply"));
}

void TCPGPSTransportTest::_cancelFromAnotherThread_data()
{
    QTest::addColumn<bool>("write");
    QTest::newRow("read") << false;
    QTest::newRow("write") << true;
}

void TCPGPSTransportTest::_cancelFromAnotherThread()
{
    QFETCH(bool, write);
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    const auto port = server.serverPort();
    std::atomic_bool stop = false;
    QSemaphore waiting;
    GPSTransport::OpenStatus opened = GPSTransport::OpenStatus::Error;
    GPSTransport::ReadStatus readStatus = GPSTransport::ReadStatus::Error;
    GPSTransport::WriteResult written;
    std::unique_ptr<QThread> worker(QThread::create([&] {
        TCPGPSTransport transport(QStringLiteral("127.0.0.1"), port, stop);
        opened = transport.open().status;
        QObject context;
        // Runs only once the blocking transport call pumps its worker event loop.
        QTimer::singleShot(0, &context, [&] { waiting.release(); });
        if (write) {
            const QByteArray payload(8 * 1024 * 1024, 'x');
            written = transport.writeBounded(reinterpret_cast<const uint8_t*>(payload.constData()), payload.size(),
                                             QDeadlineTimer(TestTimeout::mediumMs()));
        } else {
            uint8_t byte{};
            readStatus = transport.read(&byte, 1, TestTimeout::mediumMs()).status;
        }
    }));
    worker->start();
    const bool entered = waiting.tryAcquire(1, TestTimeout::mediumMs());
    QElapsedTimer elapsed;
    elapsed.start();
    stop = true;
    const bool timely = worker->wait(TestTimeout::shortMs());
    if (!timely) {
        worker->wait();
    }
    QVERIFY(entered);
    QVERIFY(timely);
    QVERIFY(elapsed.elapsed() < TestTimeout::shortMs());
    QCOMPARE(opened, GPSTransport::OpenStatus::Opened);
    if (write) {
        QCOMPARE(written.status, GPSTransport::WriteStatus::Cancelled);
        QVERIFY(written.acceptedBytes > 0);
        QCOMPARE(written.writtenBytes + written.uncertainBytes, written.acceptedBytes);
    } else {
        QCOMPARE(readStatus, GPSTransport::ReadStatus::Cancelled);
    }
}
