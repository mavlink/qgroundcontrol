#include "GPSTransportContractTest.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <memory>
#include <span>
#include <thread>

#include <QtCore/QCoreApplication>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QElapsedTimer>
#include <QtCore/QEventLoop>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QTimer>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QUdpSocket>

#include "GPSCancellation.h"
#include "TCPGPSTransport.h"
#include "Transport/Support/UnusedUdpPort.h"
#include "UDPGPSTransport.h"

using namespace std::chrono_literals;
using namespace GPSTest;

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID) && !defined(QGC_NO_SERIAL_LINK)
#include <unistd.h>

#include "SerialGPSTransport.h"
#include "Transport/Support/PseudoTerminal.h"
#endif

namespace {
/// A read timeout short enough to keep a test that waits it out fast.
constexpr std::chrono::milliseconds SHORT_READ{20};

enum class TransportKind
{
    Tcp,
    Udp,
    Serial,
};

class ContractEndpoint
{
public:
    virtual ~ContractEndpoint() = default;

    /// Opens the transport and connects its peer. @return why that failed; empty when it succeeded.
    virtual QString start() = 0;
    virtual bool peerWrite(const QByteArray& bytes) = 0;
    virtual QByteArray peerRead(qsizetype bytes) = 0;
    virtual void closePeer() = 0;

    GPSCancelSource stop;
    std::unique_ptr<GPSTransport> transport;
    bool bidirectional = true;
    bool terminalPeer = true;
};

class TcpEndpoint final : public ContractEndpoint
{
public:
    QString start() override
    {
        if (!_server.listen(QHostAddress::LocalHost)) {
            return QStringLiteral("Cannot listen for the TCP peer: %1").arg(_server.errorString());
        }
        transport = std::make_unique<TCPGPSTransport>(QStringLiteral("127.0.0.1"), _server.serverPort(), stop.token());
        if (transport->open().status != GPSOpenStatus::Opened) {
            return QStringLiteral("Cannot open the TCP transport");
        }
        if (!_server.hasPendingConnections() && !_server.waitForNewConnection(TestTimeout::shortMs())) {
            return QStringLiteral("The TCP peer did not connect");
        }
        _peer.reset(_server.nextPendingConnection());
        return _peer ? QString() : QStringLiteral("The TCP peer is missing");
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
    QString start() override
    {
        int failures = 0;
        _port = bindUnusedUdpPort(
            [this](quint16 port) {
                transport = std::make_unique<UDPGPSTransport>(port, stop.token(), PEER_IDLE_TIMEOUT);
                return transport->open().status == GPSOpenStatus::Opened;
            },
            failures);
        if (_port == 0) {
            return QStringLiteral("Cannot bind the UDP transport");
        }
        if (!_peer.bind(QHostAddress::LocalHost, 0)) {
            return QStringLiteral("Cannot bind the UDP peer: %1").arg(_peer.errorString());
        }
        bidirectional = false;
        terminalPeer = false;
        return {};
    }

    bool peerWrite(const QByteArray& bytes) override
    {
        return _peer.writeDatagram(bytes, QHostAddress::LocalHost, _port) == bytes.size();
    }

    QByteArray peerRead(qsizetype) override { return {}; }

    void closePeer() override { _peer.close(); }

private:
    static constexpr std::chrono::milliseconds PEER_IDLE_TIMEOUT{50};

    quint16 _port = 0;
    QUdpSocket _peer;
};

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID) && !defined(QGC_NO_SERIAL_LINK)
class SerialEndpoint final : public ContractEndpoint
{
public:
    QString start() override
    {
        if (!_terminal.isValid()) {
            return QStringLiteral("Cannot create a serial pseudo-terminal");
        }
        transport = std::make_unique<SerialGPSTransport>(_terminal.slavePath(), stop.token());
        if (transport->open().status != GPSOpenStatus::Opened) {
            return QStringLiteral("Cannot open the serial transport");
        }
        terminalPeer = false;
        return {};
    }

    bool peerWrite(const QByteArray& bytes) override
    {
        return ::write(_terminal.masterHandle(), bytes.constData(), static_cast<size_t>(bytes.size())) == bytes.size();
    }

    QByteArray peerRead(qsizetype bytes) override
    {
        QByteArray received;
        const QDeadlineTimer deadline(TestTimeout::shortMs());
        while (received.size() < bytes && !deadline.hasExpired()) {
            received += _terminal.readAvailable();
            QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
        }
        return received.left(bytes);
    }

    void closePeer() override { _terminal.closeMaster(); }

private:
    PseudoTerminal _terminal;
};
#endif

/// The started endpoint of @a kind; nullptr with an empty @a failure when this platform has no such transport.
std::unique_ptr<ContractEndpoint> startEndpoint(TransportKind kind, QString& failure)
{
    std::unique_ptr<ContractEndpoint> endpoint;
    switch (kind) {
        case TransportKind::Tcp:
            endpoint = std::make_unique<TcpEndpoint>();
            break;
        case TransportKind::Udp:
            endpoint = std::make_unique<UdpEndpoint>();
            break;
        case TransportKind::Serial:
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID) && !defined(QGC_NO_SERIAL_LINK)
            endpoint = std::make_unique<SerialEndpoint>();
#endif
            break;
    }
    if (endpoint) {
        failure = endpoint->start();
        if (!failure.isEmpty()) {
            endpoint.reset();
        }
    }
    return endpoint;
}

QByteArray readTransport(GPSTransport& transport, qsizetype bytes, std::chrono::milliseconds timeout)
{
    QByteArray received;
    std::array<uint8_t, 64> buffer{};
    const QDeadlineTimer deadline(TestTimeout::mediumMs());
    while (received.size() < bytes && !deadline.hasExpired()) {
        const auto desired = std::min<qsizetype>(static_cast<qsizetype>(buffer.size()), bytes - received.size());
        const auto result = transport.read(std::span(buffer).first(static_cast<size_t>(desired)), timeout);
        if (result.status != GPSReadStatus::Data || result.bytesRead <= 0) {
            break;
        }
        received.append(reinterpret_cast<const char*>(buffer.data()), result.bytesRead);
        timeout = 0ms;
    }
    return received;
}

void addTransportRows()
{
    QTest::addColumn<int>("kind");
    QTest::newRow("TCP") << static_cast<int>(TransportKind::Tcp);
    QTest::newRow("UDP") << static_cast<int>(TransportKind::Udp);
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID) && !defined(QGC_NO_SERIAL_LINK)
    QTest::newRow("Serial") << static_cast<int>(TransportKind::Serial);
#endif
}

/// The started endpoint of the row's transport, or null with the reason in @a failure.
std::unique_ptr<ContractEndpoint> rowEndpoint(QString& failure)
{
    QFETCH(int, kind);
    return startEndpoint(static_cast<TransportKind>(kind), failure);
}
}  // namespace

void GPSTransportContractTest::init()
{
    UnitTest::init();
    // A probed UDP port another process takes before the transport binds it is retried on another port.
    ignoreLogMessage("GPS.Transport.UDPGPSTransport", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Cannot listen for receiver data on UDP port")));
}

void GPSTransportContractTest::_readDeliversBytes_data()
{
    addTransportRows();
}

void GPSTransportContractTest::_readDeliversBytes()
{
    QString failure;
    const auto endpoint = rowEndpoint(failure);
    QVERIFY2(endpoint, qPrintable(failure));
    const QByteArray payload("abcdef");
    QVERIFY(endpoint->peerWrite(payload));
    QCOMPARE(readTransport(*endpoint->transport, payload.size(), TestTimeout::shortDuration()), payload);
}

void GPSTransportContractTest::_emptyReadTimesOut_data()
{
    addTransportRows();
}

void GPSTransportContractTest::_emptyReadTimesOut()
{
    QString failure;
    const auto endpoint = rowEndpoint(failure);
    QVERIFY2(endpoint, qPrintable(failure));
    std::array<uint8_t, 8> buffer{};
    QElapsedTimer elapsed;
    elapsed.start();
    const auto result = endpoint->transport->read(buffer, SHORT_READ);
    QCOMPARE(result.status, GPSReadStatus::TimedOut);
    // Waits out its own timeout, not a longer one.
    QVERIFY(elapsed.elapsed() < TestTimeout::shortMs());
    QVERIFY(!endpoint->transport->fatalError());
}

void GPSTransportContractTest::_stopWakesBlockedRead_data()
{
    addTransportRows();
}

void GPSTransportContractTest::_stopWakesBlockedRead()
{
    QString failure;
    const auto endpoint = rowEndpoint(failure);
    QVERIFY2(endpoint, qPrintable(failure));
    QElapsedTimer clock;
    clock.start();
    std::atomic<qint64> stoppedAtNs = -1;
    std::thread stopper;
    const auto joinStopper = qScopeGuard([&stopper] {
        if (stopper.joinable()) {
            stopper.join();
        }
    });
    QObject context;
    // Posted events run only once the read is waiting in its event loop.
    QTimer::singleShot(0, &context, [&] {
        stopper = std::thread([&] {
            stoppedAtNs = clock.nsecsElapsed();
            endpoint->stop.cancel();
        });
    });
    std::array<uint8_t, 8> buffer{};
    const auto result = endpoint->transport->read(buffer, TestTimeout::longDuration());
    const qint64 returnedAtNs = clock.nsecsElapsed();
    QCOMPARE(result.status, GPSReadStatus::Cancelled);
    const double latencyMs = static_cast<double>(returnedAtNs - stoppedAtNs.load()) / 1e6;
    // Waits do not poll: unless cancel() wakes the wait, it runs to the read's long deadline.
    QVERIFY2(latencyMs < TestTimeout::shortMs(), qPrintable(QStringLiteral("stop latency %1 ms").arg(latencyMs)));
}

void GPSTransportContractTest::_writeReachesPeer_data()
{
    addTransportRows();
}

void GPSTransportContractTest::_writeReachesPeer()
{
    QString failure;
    const auto endpoint = rowEndpoint(failure);
    QVERIFY2(endpoint, qPrintable(failure));
    if (!endpoint->bidirectional) {
        QSKIP("Receive-only transport");
    }
    const QByteArray payload("command");
    const auto result = endpoint->transport->write(payload, QDeadlineTimer(TestTimeout::shortMs()));
    QCOMPARE(result.status, GPSWriteStatus::Completed);
    QCOMPARE(endpoint->peerRead(payload.size()), payload);
}

void GPSTransportContractTest::_closedPeerIsTerminal_data()
{
    addTransportRows();
}

void GPSTransportContractTest::_closedPeerIsTerminal()
{
    QString failure;
    const auto endpoint = rowEndpoint(failure);
    QVERIFY2(endpoint, qPrintable(failure));
    if (!endpoint->terminalPeer) {
        QSKIP("Transport has no terminal peer close");
    }
    endpoint->closePeer();
    std::array<uint8_t, 8> buffer{};
    const auto result = endpoint->transport->read(buffer, SHORT_READ);
    QVERIFY(result.status == GPSReadStatus::Closed || result.status == GPSReadStatus::Error ||
            result.status == GPSReadStatus::Cancelled);
    QVERIFY(endpoint->transport->fatalError() || result.status != GPSReadStatus::Data);
    if (endpoint->transport->fatalError()) {
        // An empty read reports the failure without reading, as GPSDriver asks why a link failed.
        const auto reported = endpoint->transport->read({}, 0ms);
        QVERIFY(reported.status != GPSReadStatus::Data && reported.status != GPSReadStatus::TimedOut);
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSTransportContractTest, TestLabel::Unit)
