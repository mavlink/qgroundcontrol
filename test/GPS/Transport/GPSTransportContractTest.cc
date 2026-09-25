#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <thread>

#include <QtCore/QCoreApplication>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QElapsedTimer>
#include <QtCore/QEventLoop>
#include <QtCore/QFile>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QUdpSocket>

#include "Driver/Support/ScriptedReceiver.h"
#include "TCPGPSTransport.h"
#include "UDPGPSTransport.h"
#include "UnitTest.h"

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID) && !defined(QGC_NO_SERIAL_LINK)
#include <fcntl.h>
#include <unistd.h>

#include "SerialGPSTransport.h"
#endif

namespace {
enum class TransportKind
{
    Tcp,
    Udp,
    Serial,
    Stream,
};

class ContractEndpoint
{
public:
    virtual ~ContractEndpoint() = default;

    virtual bool peerWrite(const QByteArray& bytes) = 0;
    virtual QByteArray peerRead(qsizetype bytes) = 0;
    virtual void closePeer() = 0;

    std::atomic_bool stop{false};
    std::unique_ptr<GPSTransport> transport;
    bool bidirectional = true;
    bool terminalPeer = true;
    bool blockingRead = true;
};

class TcpEndpoint final : public ContractEndpoint
{
public:
    TcpEndpoint()
    {
        if (!_server.listen(QHostAddress::LocalHost)) {
            throw std::runtime_error("Cannot listen for TCP transport contract");
        }
        transport = std::make_unique<TCPGPSTransport>(QStringLiteral("127.0.0.1"), _server.serverPort(), stop);
        if (transport->open().status != GPSOpenStatus::Opened) {
            throw std::runtime_error("Cannot open TCP transport contract");
        }
        if (!_server.hasPendingConnections() && !_server.waitForNewConnection(TestTimeout::shortMs())) {
            throw std::runtime_error("TCP peer did not connect");
        }
        _peer.reset(_server.nextPendingConnection());
        if (!_peer) {
            throw std::runtime_error("TCP peer missing");
        }
    }

    bool peerWrite(const QByteArray& bytes) override
    {
        return _peer->write(bytes) == bytes.size() && _peer->waitForBytesWritten(TestTimeout::shortMs());
    }

    QByteArray peerRead(qsizetype bytes) override
    {
        QByteArray received;
        const QDeadlineTimer deadline(TestTimeout::shortMs());
        while (received.size() < bytes && !deadline.hasExpired()) {
            if (_peer->bytesAvailable() == 0) {
                _peer->waitForReadyRead(static_cast<int>(deadline.remainingTime()));
            }
            received += _peer->read(bytes - received.size());
        }
        return received;
    }

    void closePeer() override { _peer->disconnectFromHost(); }

private:
    QTcpServer _server;
    std::unique_ptr<QTcpSocket> _peer;
};

class UdpEndpoint final : public ContractEndpoint
{
public:
    UdpEndpoint()
    {
        QUdpSocket reservation;
        if (!reservation.bind(QHostAddress::LocalHost, 0)) {
            throw std::runtime_error("Cannot reserve UDP port");
        }
        _port = reservation.localPort();
        reservation.close();
        transport = std::make_unique<UDPGPSTransport>(_port, stop, 50);
        if (transport->open().status != GPSOpenStatus::Opened) {
            throw std::runtime_error("Cannot open UDP transport contract");
        }
        if (!_peer.bind(QHostAddress::LocalHost, 0)) {
            throw std::runtime_error("Cannot bind UDP peer");
        }
        bidirectional = false;
        terminalPeer = false;
    }

    bool peerWrite(const QByteArray& bytes) override
    {
        return _peer.writeDatagram(bytes, QHostAddress::LocalHost, _port) == bytes.size();
    }

    QByteArray peerRead(qsizetype) override { return {}; }

    void closePeer() override { _peer.close(); }

private:
    quint16 _port = 0;
    QUdpSocket _peer;
};

class StreamEndpoint final : public ContractEndpoint
{
public:
    StreamEndpoint()
    {
        auto receiver = std::make_unique<ScriptedReceiver>(stop);
        _receiver = receiver.get();
        _receiver->setReadHandler([this](uint8_t*, int, int) -> std::optional<GPSReadResult> {
            if (_closed) {
                return GPSReadResult{GPSReadStatus::Closed};
            }
            return std::nullopt;
        });
        _receiver->setWriteHandler(
            [this](const QByteArray& bytes, const ScriptedReceiver::WriteContext&) -> std::optional<GPSWriteResult> {
                if (_closed) {
                    return GPSWriteResult{GPSWriteStatus::Error};
                }
                _written += bytes;
                const int length = bytes.size();
                return GPSWriteResult{GPSWriteStatus::Completed, length, length};
            });
        transport = std::move(receiver);
        blockingRead = false;
    }

    bool peerWrite(const QByteArray& bytes) override
    {
        _receiver->queueReply(bytes);
        return true;
    }

    QByteArray peerRead(qsizetype bytes) override
    {
        if (_written.size() < bytes) {
            return {};
        }
        return _written.left(bytes);
    }

    void closePeer() override
    {
        _closed = true;
        _receiver->setFatalError(true);
    }

private:
    ScriptedReceiver* _receiver = nullptr;
    QByteArray _written;
    bool _closed = false;
};

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID) && !defined(QGC_NO_SERIAL_LINK)
QString openPseudoTerminal(QFile& master)
{
    const int descriptor = posix_openpt(O_RDWR | O_NOCTTY | O_CLOEXEC | O_NONBLOCK);
    if (descriptor < 0) {
        return {};
    }
    if (!master.open(descriptor, QIODevice::ReadWrite, QFileDevice::AutoCloseHandle)) {
        close(descriptor);
        return {};
    }
    if (grantpt(descriptor) != 0 || unlockpt(descriptor) != 0) {
        return {};
    }
    const char* slave = ptsname(descriptor);
    return slave ? QString::fromLocal8Bit(slave) : QString();
}

class SerialEndpoint final : public ContractEndpoint
{
public:
    SerialEndpoint()
    {
        const QString slave = openPseudoTerminal(_master);
        if (slave.isEmpty()) {
            throw std::runtime_error("Cannot create serial pseudo-terminal");
        }
        transport = std::make_unique<SerialGPSTransport>(slave, stop);
        if (transport->open().status != GPSOpenStatus::Opened) {
            throw std::runtime_error("Cannot open serial transport contract");
        }
        terminalPeer = false;
    }

    bool peerWrite(const QByteArray& bytes) override
    {
        return ::write(_master.handle(), bytes.constData(), static_cast<size_t>(bytes.size())) == bytes.size();
    }

    QByteArray peerRead(qsizetype bytes) override
    {
        QByteArray received;
        const QDeadlineTimer deadline(TestTimeout::shortMs());
        while (received.size() < bytes && !deadline.hasExpired()) {
            char buffer[256];
            const auto count = ::read(_master.handle(), buffer, sizeof(buffer));
            if (count > 0) {
                received.append(buffer, count);
            } else if (errno != EAGAIN && errno != EINTR) {
                break;
            } else {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
            }
        }
        return received.left(bytes);
    }

    void closePeer() override { _master.close(); }

private:
    QFile _master;
};
#endif

std::unique_ptr<ContractEndpoint> makeEndpoint(TransportKind kind)
{
    switch (kind) {
        case TransportKind::Tcp:
            return std::make_unique<TcpEndpoint>();
        case TransportKind::Udp:
            return std::make_unique<UdpEndpoint>();
        case TransportKind::Serial:
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID) && !defined(QGC_NO_SERIAL_LINK)
            return std::make_unique<SerialEndpoint>();
#else
            return {};
#endif
        case TransportKind::Stream:
            return std::make_unique<StreamEndpoint>();
    }
    return {};
}

QByteArray readTransport(GPSTransport& transport, qsizetype bytes, int timeoutMs)
{
    QByteArray received;
    std::array<uint8_t, 64> buffer{};
    const QDeadlineTimer deadline(TestTimeout::mediumMs());
    while (received.size() < bytes && !deadline.hasExpired()) {
        const auto desired = std::min<qsizetype>(static_cast<qsizetype>(buffer.size()), bytes - received.size());
        const auto result = transport.read(buffer.data(), static_cast<int>(desired), timeoutMs);
        if (result.status != GPSReadStatus::Data || result.bytesRead <= 0) {
            break;
        }
        received.append(reinterpret_cast<const char*>(buffer.data()), result.bytesRead);
        timeoutMs = 0;
    }
    return received;
}

void addTransportRows()
{
    QTest::addColumn<int>("kind");
    QTest::newRow("TCP") << static_cast<int>(TransportKind::Tcp);
    QTest::newRow("UDP") << static_cast<int>(TransportKind::Udp);
    QTest::newRow("Serial") << static_cast<int>(TransportKind::Serial);
    QTest::newRow("Stream") << static_cast<int>(TransportKind::Stream);
}
}  // namespace

class GPSTransportContractTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _readDeliversBytes_data();
    void _readDeliversBytes();
    void _emptyReadTimesOut_data();
    void _emptyReadTimesOut();
    void _cancelRead_data();
    void _cancelRead();
    void _writeReachesPeer_data();
    void _writeReachesPeer();
    void _closedPeerIsTerminal_data();
    void _closedPeerIsTerminal();
};

void GPSTransportContractTest::_readDeliversBytes_data()
{
    addTransportRows();
}

void GPSTransportContractTest::_readDeliversBytes()
{
    QFETCH(int, kind);
    auto endpoint = makeEndpoint(static_cast<TransportKind>(kind));
    if (!endpoint) {
        QSKIP("Transport unavailable on this platform");
    }
    const QByteArray payload("abcdef");
    QVERIFY(endpoint->peerWrite(payload));
    QCOMPARE(readTransport(*endpoint->transport, payload.size(), TestTimeout::shortMs()), payload);
}

void GPSTransportContractTest::_emptyReadTimesOut_data()
{
    addTransportRows();
}

void GPSTransportContractTest::_emptyReadTimesOut()
{
    QFETCH(int, kind);
    auto endpoint = makeEndpoint(static_cast<TransportKind>(kind));
    if (!endpoint) {
        QSKIP("Transport unavailable on this platform");
    }
    std::array<uint8_t, 8> buffer{};
    QElapsedTimer elapsed;
    elapsed.start();
    const auto result = endpoint->transport->read(buffer.data(), static_cast<int>(buffer.size()), 20);
    QCOMPARE(result.status, GPSReadStatus::TimedOut);
    QVERIFY(elapsed.elapsed() < TestTimeout::shortMs());
    QVERIFY(!endpoint->transport->fatalError());
}

void GPSTransportContractTest::_cancelRead_data()
{
    addTransportRows();
}

void GPSTransportContractTest::_cancelRead()
{
    QFETCH(int, kind);
    auto endpoint = makeEndpoint(static_cast<TransportKind>(kind));
    if (!endpoint) {
        QSKIP("Transport unavailable on this platform");
    }
    if (!endpoint->blockingRead) {
        QSKIP("Transport double does not block reads");
    }
    std::jthread cancellation([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        endpoint->stop = true;
    });
    std::array<uint8_t, 8> buffer{};
    QElapsedTimer elapsed;
    elapsed.start();
    const auto result =
        endpoint->transport->read(buffer.data(), static_cast<int>(buffer.size()), TestTimeout::longMs());
    QCOMPARE(result.status, GPSReadStatus::Cancelled);
    QVERIFY(elapsed.elapsed() < TestTimeout::shortMs());
}

void GPSTransportContractTest::_writeReachesPeer_data()
{
    addTransportRows();
}

void GPSTransportContractTest::_writeReachesPeer()
{
    QFETCH(int, kind);
    auto endpoint = makeEndpoint(static_cast<TransportKind>(kind));
    if (!endpoint) {
        QSKIP("Transport unavailable on this platform");
    }
    if (!endpoint->bidirectional) {
        QSKIP("Receive-only transport");
    }
    const QByteArray payload("command");
    const auto result = endpoint->transport->write(reinterpret_cast<const uint8_t*>(payload.constData()),
                                                   payload.size(), QDeadlineTimer(TestTimeout::shortMs()));
    QCOMPARE(result.status, GPSWriteStatus::Completed);
    QCOMPARE(endpoint->peerRead(payload.size()), payload);
}

void GPSTransportContractTest::_closedPeerIsTerminal_data()
{
    addTransportRows();
}

void GPSTransportContractTest::_closedPeerIsTerminal()
{
    QFETCH(int, kind);
    auto endpoint = makeEndpoint(static_cast<TransportKind>(kind));
    if (!endpoint) {
        QSKIP("Transport unavailable on this platform");
    }
    if (!endpoint->terminalPeer) {
        QSKIP("Connectionless transport has no terminal peer close");
    }
    endpoint->closePeer();
    std::array<uint8_t, 8> buffer{};
    const auto result = endpoint->transport->read(buffer.data(), static_cast<int>(buffer.size()), 20);
    QVERIFY(result.status == GPSReadStatus::Closed || result.status == GPSReadStatus::Error ||
            result.status == GPSReadStatus::Cancelled);
    QVERIFY(endpoint->transport->fatalError() || result.status != GPSReadStatus::Data);
}

UT_REGISTER_TEST(GPSTransportContractTest, TestLabel::Unit)

#include "GPSTransportContractTest.moc"
