#include "TCPGPSTransportTest.h"

#include <chrono>
#include <memory>
#include <span>

#include <QtCore/QDeadlineTimer>
#include <QtCore/QElapsedTimer>
#include <QtCore/QSemaphore>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

#include "GPSCancellation.h"
#include "TCPGPSTransport.h"

using namespace std::chrono_literals;

namespace {
/// A write deadline an unread peer's full buffers cannot meet; short, so the test does not wait long for it.
constexpr int STALLED_WRITE_DEADLINE_MS = 100;
/// One read of a stream the test drains in a loop under its own deadline.
constexpr std::chrono::milliseconds READ_SLICE{100};

class InspectableTCPGPSTransport : public TCPGPSTransport
{
public:
    using TCPGPSTransport::device;
    using TCPGPSTransport::TCPGPSTransport;
};
}  // namespace

void TCPGPSTransportTest::_fixedRateAndPeerClose()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    GPSCancelSource stop;
    TCPGPSTransport transport(QStringLiteral("localhost"), server.serverPort(), stop.token());
    uint8_t buffer[64]{};
    QCOMPARE(transport.read({}, 0ms).status, GPSReadStatus::Closed);
    QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
    QCOMPARE(transport.read({}, 0ms).status, GPSReadStatus::Data);
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::shortMs());
    auto* peer = server.nextPendingConnection();
    QVERIFY(peer);
    QVERIFY(transport.setBaudrate(115200));
    QCOMPARE(transport.fixedBaudrate(), 115200u);
    QVERIFY(!transport.setBaudrate(9600));
    QVERIFY(!transport.fatalError());

    // GPSTransportContractTest covers reads, writes and timeouts. The final buffered bytes survive the peer's orderly
    // disconnect.
    const QByteArray payload = QByteArray::fromHex("b56201020300d300ff");
    QCOMPARE(peer->write(payload), payload.size());
    peer->disconnectFromHost();
    QTRY_VERIFY_WITH_TIMEOUT(transport.fatalError(), TestTimeout::shortMs());
    QCOMPARE(transport.read({}, 0ms).status, GPSReadStatus::Closed);
    const auto finalRead = transport.read(buffer, 0ms);
    QCOMPARE(finalRead.status, GPSReadStatus::Data);
    QCOMPARE(finalRead.bytesRead, payload.size());
    QCOMPARE(QByteArray(reinterpret_cast<char*>(buffer), finalRead.bytesRead), payload);
    QCOMPARE(transport.read(buffer, 0ms).status, GPSReadStatus::Closed);
    QVERIFY(transport.write(payload, QDeadlineTimer(TestTimeout::shortMs())).status != GPSWriteStatus::Completed);
}

void TCPGPSTransportTest::_cancelWait_data()
{
    QTest::addColumn<QString>("phase");
    QTest::newRow("before-open") << QStringLiteral("before-open");
    QTest::newRow("connecting") << QStringLiteral("connecting");
}

void TCPGPSTransportTest::_cancelWait()
{
    QFETCH(QString, phase);
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    GPSCancelSource stop;
    if (phase == QStringLiteral("before-open")) {
        stop.cancel();
    }
    TCPGPSTransport transport(QStringLiteral("localhost"), server.serverPort(), stop.token());
    if (phase == QStringLiteral("before-open")) {
        QCOMPARE(transport.open().status, GPSOpenStatus::Cancelled);
    } else {
        QTimer::singleShot(0, &server, [&]() { stop.cancel(); });
        QElapsedTimer elapsed;
        elapsed.start();
        QVERIFY(transport.open().status != GPSOpenStatus::Opened);
        // Ended by the stop rather than the five-second connect timeout.
        QVERIFY(elapsed.elapsed() < TestTimeout::shortMs());
    }
    QVERIFY(stop.isCancelled());
    QVERIFY(transport.isCancelled());
    uint8_t byte{};
    QVERIFY(transport.read(std::span(&byte, 1), TestTimeout::shortDuration()).status != GPSReadStatus::Data);
    QCOMPARE(transport.write("*", QDeadlineTimer(TestTimeout::shortMs())).status, GPSWriteStatus::Cancelled);
}

void TCPGPSTransportTest::_refusedConnection()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    const quint16 port = server.serverPort();
    server.close();
    GPSCancelSource stop;
    TCPGPSTransport transport(QStringLiteral("127.0.0.1"), port, stop.token());
    expectLogMessage("GPS.Transport.TCPGPSTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Failed to connect to GPS receiver")));
    QVERIFY(transport.open().status != GPSOpenStatus::Opened);
    verifyExpectedLogMessage();
    QVERIFY(transport.fatalError());
    QCOMPARE(transport.read({}, 0ms).status, GPSReadStatus::Closed);
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
    GPSCancelSource stop;
    TCPGPSTransport transport(QStringLiteral("localhost"), server.serverPort(), stop.token());
    QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::shortMs());
    auto* peer = server.nextPendingConnection();
    QVERIFY(peer);
    peer->setReadBufferSize(1);
    const QByteArray payload(8 * 1024 * 1024, 'x');
    if (cancel) {
        QTimer::singleShot(0, &server, [&]() { stop.cancel(); });
    }
    QElapsedTimer elapsed;
    elapsed.start();
    const auto result = transport.write(payload, QDeadlineTimer(STALLED_WRITE_DEADLINE_MS));
    QCOMPARE(result.status, cancel ? GPSWriteStatus::Cancelled : GPSWriteStatus::TimedOut);
    QVERIFY(result.acceptedBytes > 0);
    QVERIFY(result.acceptedBytes < payload.size());
    QVERIFY(result.uncertainBytes() >= 0);
    QVERIFY(result.uncertainBytes() <= TCPGPSTransport::WRITE_BUFFER_BYTES);
    QVERIFY(transport.fatalError());
    QCOMPARE(transport.write(payload, QDeadlineTimer(STALLED_WRITE_DEADLINE_MS)).acceptedBytes, 0);
    QCOMPARE(result.writtenBytes + result.uncertainBytes(), result.acceptedBytes);
    // Bounded by its deadline or the stop, not by the unread peer.
    QVERIFY(elapsed.elapsed() < TestTimeout::shortMs());
    QCOMPARE(stop.isCancelled(), cancel);
}

void TCPGPSTransportTest::_boundedIngressPreservesStream()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    GPSCancelSource stop;
    InspectableTCPGPSTransport transport(QStringLiteral("localhost"), server.serverPort(), stop.token());
    QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), TestTimeout::shortMs());
    auto* peer = server.nextPendingConnection();
    QVERIFY(peer);
    QByteArray payload(4 * TCPGPSTransport::READ_BUFFER_BYTES, Qt::Uninitialized);
    for (qsizetype i = 0; i < payload.size(); ++i) {
        payload[i] = static_cast<char>(i % 251);
    }
    QCOMPARE(peer->write(payload), payload.size());
    // Stall the decoder while Qt services input, as it does during configuration/write waits.
    QTRY_COMPARE_WITH_TIMEOUT(transport.device()->bytesAvailable(), TCPGPSTransport::READ_BUFFER_BYTES,
                              TestTimeout::shortMs());
    auto* const socket = qobject_cast<QTcpSocket*>(transport.device());
    QVERIFY(socket);
    QCOMPARE(socket->readBufferSize(), TCPGPSTransport::READ_BUFFER_BYTES);
    QVERIFY(!transport.fatalError());
    QByteArray received;
    uint8_t bytes[4096]{};
    const QDeadlineTimer deadline(TestTimeout::mediumMs());
    while (received.size() < payload.size() && !deadline.hasExpired()) {
        const auto result = transport.read(bytes, READ_SLICE);
        QVERIFY(result.status == GPSReadStatus::Data || result.status == GPSReadStatus::TimedOut);
        received.append(reinterpret_cast<const char*>(bytes), result.bytesRead);
        QVERIFY(transport.device()->bytesAvailable() <= TCPGPSTransport::READ_BUFFER_BYTES);
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
    GPSCancelSource stop;
    TCPGPSTransport transport(QStringLiteral("127.0.0.1"), server.serverPort(), stop.token());
    QCOMPARE(transport.open().status, GPSOpenStatus::Opened);
    if (!server.hasPendingConnections()) {
        QVERIFY(server.waitForNewConnection(TestTimeout::shortMs()));
    }
    auto* peer = server.nextPendingConnection();
    QVERIFY(peer);
    QCOMPARE(peer->write("reply", 5), 5);
    QVERIFY(peer->waitForBytesWritten(TestTimeout::shortMs()));
    // Do not dispatch receiver events before polling; the bytes are still in the kernel.
    // Loopback delivery is asynchronous on some platforms (macOS), so repeat the immediate poll.
    uint8_t bytes[16]{};
    const QDeadlineTimer deadline(TestTimeout::shortMs());
    GPSReadResult result = transport.read(bytes, std::chrono::milliseconds(timeout));
    while (result.status == GPSReadStatus::TimedOut && !deadline.hasExpired()) {
        QThread::yieldCurrentThread();
        result = transport.read(bytes, std::chrono::milliseconds(timeout));
    }
    QCOMPARE(result.status, GPSReadStatus::Data);
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
    GPSCancelSource stop;
    QSemaphore waiting;
    GPSOpenStatus opened = GPSOpenStatus::Error;
    GPSReadStatus readStatus = GPSReadStatus::Error;
    GPSWriteResult written;
    std::unique_ptr<QThread> worker(QThread::create([&] {
        TCPGPSTransport transport(QStringLiteral("127.0.0.1"), port, stop.token());
        opened = transport.open().status;
        QObject context;
        // Runs only once the blocking transport call pumps its worker event loop.
        QTimer::singleShot(0, &context, [&] { waiting.release(); });
        if (write) {
            const QByteArray payload(8 * 1024 * 1024, 'x');
            written = transport.write(payload, QDeadlineTimer(TestTimeout::mediumMs()));
        } else {
            uint8_t byte{};
            readStatus = transport.read(std::span(&byte, 1), TestTimeout::mediumDuration()).status;
        }
    }));
    worker->start();
    const bool entered = waiting.tryAcquire(1, TestTimeout::mediumMs());
    stop.cancel();
    const bool timely = worker->wait(TestTimeout::shortMs());
    if (!timely) {
        worker->wait();
    }
    QVERIFY(entered);
    QVERIFY(timely);
    QCOMPARE(opened, GPSOpenStatus::Opened);
    if (write) {
        QCOMPARE(written.status, GPSWriteStatus::Cancelled);
        QVERIFY(written.acceptedBytes > 0);
        QCOMPARE(written.writtenBytes + written.uncertainBytes(), written.acceptedBytes);
    } else {
        QCOMPARE(readStatus, GPSReadStatus::Cancelled);
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(TCPGPSTransportTest, TestLabel::Unit)
