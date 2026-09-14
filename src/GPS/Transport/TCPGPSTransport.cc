#include "TCPGPSTransport.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QScopeGuard>
#include <QtNetwork/QTcpSocket>

#include <algorithm>
#include <utility>

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

GPSTransport::OpenResult TCPGPSTransport::open()
{
    if (isCancelled()) {
        return {OpenStatus::Cancelled};
    }
    const QDeadlineTimer connectDeadline(kConnectTimeoutMs);
    _socket = std::make_unique<QTcpSocket>();
    _socket->setReadBufferSize(kReadBufferBytes);
    _socket->connectToHost(_host, _port);
    if (waitForSocket(
            _socket.get(), [this]() { return _socket->state() == QAbstractSocket::ConnectedState; }, connectDeadline)) {
        return {OpenStatus::Opened};
    }
    if (!isCancelled()) {
        qCWarning(TCPGPSTransportLog) << "Failed to connect to GPS receiver" << _host << _port
                                      << _socket->errorString();
    }
    const auto result = OpenResult{isCancelled()                  ? OpenStatus::Cancelled
                                   : connectDeadline.hasExpired() ? OpenStatus::TimedOut
                                                                  : OpenStatus::Error,
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

GPSTransport::ReadResult TCPGPSTransport::read(uint8_t* buffer, int length, int timeoutMs)
{
    if (isCancelled()) {
        return {ReadStatus::Cancelled};
    }
    if (!buffer || length < 0) {
        return {ReadStatus::InvalidData};
    }
    if (!_socket) {
        return {ReadStatus::Closed};
    }
    if (length == 0) {
        return {fatalError() ? ReadStatus::Closed : ReadStatus::Data};
    }
    if (timeoutMs <= 0 && _socket->bytesAvailable() == 0 && !fatalError()) {
        // bytesAvailable() only sees Qt's buffer; immediate polls must also service kernel input.
        _socket->waitForReadyRead(0);
    }
    if (_socket->bytesAvailable() == 0 && !waitForSocket(
                                              _socket.get(), [this]() { return _socket->bytesAvailable() > 0; },
                                              QDeadlineTimer((std::max) (timeoutMs, 0)))) {
        return {isCancelled()  ? ReadStatus::Cancelled
                : fatalError() ? ReadStatus::Closed
                               : ReadStatus::TimedOut,
                0, fatalError() ? _socket->errorString() : QString()};
    }
    const auto count = _socket->read(reinterpret_cast<char*>(buffer), length);
    return {count < 0 ? ReadStatus::Error : ReadStatus::Data, static_cast<int>((std::max) (count, qint64(0))),
            count < 0 ? _socket->errorString() : QString()};
}

std::chrono::milliseconds TCPGPSTransport::configurationWriteTimeout() const
{
    return std::chrono::milliseconds(kWriteTimeoutMs);
}

GPSTransport::WriteResult TCPGPSTransport::writeBounded(const uint8_t* buffer, int length, QDeadlineTimer deadline)
{
    if (isCancelled()) {
        return {WriteStatus::Cancelled};
    }
    if (!buffer || length < 0) {
        return {WriteStatus::InvalidData};
    }
    if (fatalError() || _socket->bytesToWrite() != 0) {
        return {WriteStatus::Error, 0, 0, 0, _socket ? _socket->errorString() : QStringLiteral("GPS socket is closed")};
    }
    if (length == 0) {
        return {WriteStatus::Completed};
    }
    if (deadline.hasExpired()) {
        return {WriteStatus::TimedOut};
    }
    qint64 drained = 0;
    const auto connection = QObject::connect(
        _socket.get(), &QTcpSocket::bytesWritten, _socket.get(), [&drained](qint64 count) { drained += count; },
        Qt::DirectConnection);
    const auto disconnect = qScopeGuard([&]() { QObject::disconnect(connection); });
    int accepted = 0;
    WriteStatus status = WriteStatus::Completed;
    while (accepted < length || _socket->bytesToWrite() > 0) {
        if (isCancelled() || fatalError() || deadline.hasExpired()) {
            status = isCancelled() ? WriteStatus::Cancelled : fatalError() ? WriteStatus::Error : WriteStatus::TimedOut;
            break;
        }
        if (accepted < length && _socket->bytesToWrite() < kWriteBufferBytes) {
            const auto count =
                _socket->write(reinterpret_cast<const char*>(buffer) + accepted,
                               (std::min) (qint64(length - accepted), kWriteBufferBytes - _socket->bytesToWrite()));
            if (count < 0) {
                status = WriteStatus::Error;
                break;
            }
            accepted += static_cast<int>(count);
        }
        if (_socket->bytesToWrite() > 0) {
            waitForSocket(_socket.get(), [this]() { return _socket->bytesToWrite() == 0; }, deadline);
        }
    }
    if (isCancelled()) {
        status = WriteStatus::Cancelled;
    } else if (fatalError()) {
        status = WriteStatus::Error;
    } else if (deadline.hasExpired()) {
        status = WriteStatus::TimedOut;
    }
    const int written = static_cast<int>(std::clamp(drained, qint64(0), qint64(accepted)));
    const WriteResult result{status, accepted, written, accepted - written,
                             status == WriteStatus::Error ? _socket->errorString() : QString()};
    if (status != WriteStatus::Completed && accepted > 0) {
        // Discard pending output so it cannot escape after the operation's deadline.
        _socket->abort();
    }
    return result;
}

bool TCPGPSTransport::setBaudrate(unsigned baudrate)
{
    // A serial bridge must already use the receiver's baud rate; TCP cannot change it.
    return !isCancelled() && !fatalError() && baudrate == fixedBaudrate();
}
