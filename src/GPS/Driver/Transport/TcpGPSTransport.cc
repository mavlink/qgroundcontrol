#include "TcpGPSTransport.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QScopeGuard>
#include <QtNetwork/QTcpSocket>

#include <algorithm>
#include <utility>

#include "GPSSocketWait.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(TcpGPSTransportLog, "GPS.Driver.Transport.TcpGPSTransport")

TcpGPSTransport::TcpGPSTransport(QString host, quint16 port, const std::atomic_bool& requestStop)
    : GPSTransport(requestStop)
    , _host(std::move(host))
    , _port(port)
{
    qCDebug(TcpGPSTransportLog) << this;
}

TcpGPSTransport::~TcpGPSTransport()
{
    qCDebug(TcpGPSTransportLog) << this;
}

bool TcpGPSTransport::_waitFor(const std::function<bool()>& ready, QDeadlineTimer deadline)
{
    return gpsWaitForSocket(
        _socket.get(), ready, [this]() { return isCancelled(); }, [this]() { return fatalError(); }, deadline);
}

GPSTransport::OpenResult TcpGPSTransport::open()
{
    if (isCancelled()) {
        return {OpenStatus::Cancelled};
    }
    const QDeadlineTimer connectDeadline(kConnectTimeoutMs);
    _socket = std::make_unique<QTcpSocket>();
    _socket->setReadBufferSize(kReadBufferBytes);
    _socket->connectToHost(_host, _port);
    if (_waitFor([this]() { return _socket->state() == QAbstractSocket::ConnectedState; }, connectDeadline)) {
        return {OpenStatus::Opened};
    }
    if (!isCancelled()) {
        qCWarning(TcpGPSTransportLog) << "Failed to connect to GPS receiver" << _host << _port
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

bool TcpGPSTransport::fatalError() const
{
    return !_socket || _socket->state() == QAbstractSocket::UnconnectedState;
}

GPSTransport::ReadResult TcpGPSTransport::read(uint8_t* buffer, int length, int timeoutMs)
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
        return {ReadStatus::Data};
    }
    if (_socket->bytesAvailable() == 0 &&
        !_waitFor([this]() { return _socket->bytesAvailable() > 0; }, QDeadlineTimer((std::max) (timeoutMs, 0)))) {
        return {isCancelled()  ? ReadStatus::Cancelled
                : fatalError() ? ReadStatus::Closed
                               : ReadStatus::TimedOut,
                0, fatalError() ? _socket->errorString() : QString()};
    }
    const auto count = _socket->read(reinterpret_cast<char*>(buffer), length);
    return {count < 0 ? ReadStatus::Error : ReadStatus::Data, static_cast<int>((std::max) (count, qint64(0))),
            count < 0 ? _socket->errorString() : QString()};
}

std::chrono::milliseconds TcpGPSTransport::configurationWriteTimeout() const
{
    return std::chrono::milliseconds(kWriteTimeoutMs);
}

GPSTransport::WriteResult TcpGPSTransport::writeBounded(const uint8_t* buffer, int length, QDeadlineTimer deadline)
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
    const qint64 accepted = _socket->write(reinterpret_cast<const char*>(buffer), length);
    const bool completed = accepted == length && _waitFor([this]() { return _socket->bytesToWrite() == 0; }, deadline);
    const int acceptedBytes = static_cast<int>(std::clamp(accepted, qint64(0), qint64(length)));
    const int writtenBytes = static_cast<int>(std::clamp(drained, qint64(0), qint64(acceptedBytes)));
    const WriteStatus status = isCancelled()                ? WriteStatus::Cancelled
                               : completed && !fatalError() ? WriteStatus::Completed
                               : deadline.hasExpired()      ? WriteStatus::TimedOut
                                                            : WriteStatus::Error;
    return {status, acceptedBytes, writtenBytes, acceptedBytes - writtenBytes,
            status == WriteStatus::Error ? _socket->errorString() : QString()};
}

bool TcpGPSTransport::setBaudrate(unsigned baudrate)
{
    // A serial bridge must already use the receiver's baud rate; TCP cannot change it.
    return baudrate == fixedBaudrate();
}
