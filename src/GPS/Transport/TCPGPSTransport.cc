#include "TCPGPSTransport.h"

#include <algorithm>
#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QScopeGuard>
#include <QtNetwork/QTcpSocket>

#include "GPSSocketWait_p.h"
#include "GPSStreamWrite_p.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(TCPGPSTransportLog, "GPS.Transport.TCPGPSTransport")

TCPGPSTransport::TCPGPSTransport(QString host, quint16 port, const std::atomic_bool& requestStop)
    : GPSTransport(requestStop), _host(std::move(host)), _port(port)
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
    const QDeadlineTimer connectDeadline(kConnectTimeoutMs);
    _socket = std::make_unique<QTcpSocket>();
    _socket->setReadBufferSize(kReadBufferBytes);
    _socket->connectToHost(_host, _port);
    if (gpsWaitForSocket(
            *this, _socket.get(), [this]() { return _socket->state() == QAbstractSocket::ConnectedState; },
            connectDeadline, kCancellationPollMs)) {
        return {GPSOpenStatus::Opened};
    }
    if (!isCancelled()) {
        qCWarning(TCPGPSTransportLog) << "Failed to connect to GPS receiver" << _host << _port
                                      << _socket->errorString();
    }
    const auto result = GPSOpenResult{isCancelled()                  ? GPSOpenStatus::Cancelled
                                      : connectDeadline.hasExpired() ? GPSOpenStatus::TimedOut
                                                                     : GPSOpenStatus::Error,
                                      connectDeadline.hasExpired()
                                          ? QCoreApplication::translate("GPSTransport", "Receiver connection timed out")
                                          : _socket->errorString()};
    _socket->abort();
    return result;
}

bool TCPGPSTransport::fatalError() const
{
    return !_socket || _socket->state() == QAbstractSocket::UnconnectedState;
}

GPSReadResult TCPGPSTransport::read(uint8_t* buffer, int length, int timeoutMs)
{
    if (isCancelled()) {
        return {GPSReadStatus::Cancelled};
    }
    if (!buffer || length < 0) {
        return {GPSReadStatus::InvalidData};
    }
    if (!_socket) {
        return {GPSReadStatus::Closed};
    }
    if (length == 0) {
        return {fatalError() ? GPSReadStatus::Closed : GPSReadStatus::Data};
    }
    if (timeoutMs <= 0 && _socket->bytesAvailable() == 0 && !fatalError()) {
        // bytesAvailable() only sees Qt's buffer; immediate polls must also service kernel input.
        _socket->waitForReadyRead(0);
    }
    if (_socket->bytesAvailable() == 0 && !gpsWaitForSocket(
                                              *this, _socket.get(), [this]() { return _socket->bytesAvailable() > 0; },
                                              QDeadlineTimer((std::max) (timeoutMs, 0)), kCancellationPollMs)) {
        return {isCancelled()  ? GPSReadStatus::Cancelled
                : fatalError() ? GPSReadStatus::Closed
                               : GPSReadStatus::TimedOut,
                0, fatalError() ? _socket->errorString() : QString()};
    }
    const auto count = _socket->read(reinterpret_cast<char*>(buffer), length);
    return {count < 0 ? GPSReadStatus::Error : GPSReadStatus::Data, static_cast<int>((std::max) (count, qint64(0))),
            count < 0 ? _socket->errorString() : QString()};
}

std::chrono::milliseconds TCPGPSTransport::configurationWriteTimeout() const
{
    return std::chrono::milliseconds(kWriteTimeoutMs);
}

GPSWriteResult TCPGPSTransport::writeBounded(const uint8_t* buffer, int length, QDeadlineTimer deadline)
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
            gpsWaitForSocket(
                *this, _socket.get(), [this]() { return _socket->bytesToWrite() == 0; }, remaining,
                kCancellationPollMs);
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
