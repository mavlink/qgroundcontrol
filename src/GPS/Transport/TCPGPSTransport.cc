#include "TCPGPSTransport.h"

#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QScopeGuard>
#include <QtNetwork/QTcpSocket>

#include "GPSSocketWait_p.h"
#include "GPSStreamRead_p.h"
#include "GPSStreamWrite_p.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(TCPGPSTransportLog, "GPS.Transport.TCPGPSTransport")

namespace {
GPSOpenResult socketOpenFailure(const GPSTransport& transport, QAbstractSocket& socket, QDeadlineTimer deadline)
{
    const bool timedOut = deadline.hasExpired();
    const GPSOpenResult result{
        transport.isCancelled() ? GPSOpenStatus::Cancelled
        : timedOut              ? GPSOpenStatus::TimedOut
                                : GPSOpenStatus::Error,
        timedOut ? QCoreApplication::translate("GPSTransport", "Receiver connection timed out") : socket.errorString()};
    socket.abort();
    return result;
}
}  // namespace

TCPGPSTransport::TCPGPSTransport(QString host, quint16 port, std::stop_token stopToken)
    : GPSTransport(std::move(stopToken))
    , _host(std::move(host))
    , _port(port)
{
    qCDebug(TCPGPSTransportLog) << this;
}

TCPGPSTransport::~TCPGPSTransport()
{
    qCDebug(TCPGPSTransportLog) << this;
}

GPSOpenResult TCPGPSTransport::open()
{
    if (isCancelled()) {
        return {GPSOpenStatus::Cancelled};
    }
    const QDeadlineTimer connectDeadline(kConnectTimeout);
    _socket = std::make_unique<QTcpSocket>();
    _socket->setReadBufferSize(kReadBufferBytes);
    _socket->connectToHost(_host, _port);
    if (GPSSocketWait::waitFor(
            *this, _socket.get(), [this]() { return _socket->state() == QAbstractSocket::ConnectedState; },
            connectDeadline)) {
        return {GPSOpenStatus::Opened};
    }
    if (!isCancelled()) {
        qCWarning(TCPGPSTransportLog) << "Failed to connect to GPS receiver" << _host << _port
                                      << _socket->errorString();
    }
    return socketOpenFailure(*this, *_socket, connectDeadline);
}

bool TCPGPSTransport::fatalError() const
{
    return !_socket || _socket->state() == QAbstractSocket::UnconnectedState;
}

GPSReadResult TCPGPSTransport::read(uint8_t* buffer, int length, std::chrono::milliseconds timeout)
{
    return GPSStreamRead::readBounded(
        *this, buffer, length, timeout, [this]() { return !_socket; },
        [this](QDeadlineTimer deadline) {
            if (deadline.hasExpired() && _socket->bytesAvailable() == 0 && !fatalError()) {
                // bytesAvailable() only sees Qt's buffer; immediate polls must also service kernel input.
                _socket->waitForReadyRead(0);
            }
            return _socket->bytesAvailable() > 0 ||
                   GPSSocketWait::waitFor(
                       *this, _socket.get(), [this]() { return _socket->bytesAvailable() > 0; }, deadline);
        },
        [this](uint8_t* bytes, int count) { return _socket->read(reinterpret_cast<char*>(bytes), count); },
        [this]() { return GPSReadResult{GPSReadStatus::Closed, 0, _socket ? _socket->errorString() : QString()}; });
}

std::chrono::milliseconds TCPGPSTransport::configurationWriteTimeout() const
{
    return kWriteTimeout;
}

GPSWriteResult TCPGPSTransport::writeData(const uint8_t* buffer, int length, QDeadlineTimer deadline)
{
    qint64 drained = 0;
    QMetaObject::Connection connection;
    if (_socket) {
        connection = QObject::connect(
            _socket.get(), &QTcpSocket::bytesWritten, _socket.get(), [&drained](qint64 count) { drained += count; },
            Qt::DirectConnection);
    }
    const auto disconnect = qScopeGuard([&]() { QObject::disconnect(connection); });
    return GPSStreamWrite::writeBounded(
        *this, _socket.get(), buffer, length, deadline, kWriteBufferBytes,
        [this](QDeadlineTimer remaining) {
            GPSSocketWait::waitFor(*this, _socket.get(), [this]() { return _socket->bytesToWrite() == 0; }, remaining);
        },
        [&drained](int) { return drained; },
        [this]() { return _socket ? _socket->errorString() : QStringLiteral("GPS socket is closed"); },
        [this]() { _socket->abort(); });
}

bool TCPGPSTransport::setBaudrate(unsigned baudrate)
{
    // A serial bridge must already use the receiver's baud rate; TCP cannot change it.
    return !isCancelled() && !fatalError() && baudrate == fixedBaudrate();
}
