#include "TcpGPSTransport.h"

#include <QtCore/QEventLoop>
#include <QtCore/QScopeGuard>
#include <QtCore/QTimer>
#include <QtNetwork/QTcpSocket>

#include <algorithm>
#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(TcpGPSTransportLog, "GPS.Driver.TcpGPSTransport")

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
    if (isCancelled() || !_socket) {
        return false;
    }
    if (ready()) {
        return true;
    }
    if (deadline.hasExpired() || _socket->state() == QAbstractSocket::UnconnectedState) {
        return false;
    }

    // waitForConnected() aborts the connection on timeout, so short blocking waits
    // cannot provide cancellable DNS/connection progress. Run the worker's events instead.
    QEventLoop loop;
    QTimer cancellation;
    QTimer timeout;
    const auto check = [&]() {
        if (isCancelled() || ready() || _socket->state() == QAbstractSocket::UnconnectedState) {
            loop.quit();
        }
    };
    QObject::connect(_socket.get(), &QTcpSocket::stateChanged, &loop, check);
    QObject::connect(_socket.get(), &QTcpSocket::readyRead, &loop, check);
    QObject::connect(_socket.get(), &QTcpSocket::bytesWritten, &loop, check);
    QObject::connect(&cancellation, &QTimer::timeout, &loop, check);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    timeout.setSingleShot(true);
    timeout.setTimerType(Qt::PreciseTimer);
    cancellation.start(kCancellationPollMs);
    timeout.start(static_cast<int>(deadline.remainingTime()));
    loop.exec();
    return !isCancelled() && ready();
}

bool TcpGPSTransport::open()
{
    if (isCancelled()) {
        return false;
    }
    _socket = std::make_unique<QTcpSocket>();
    _socket->connectToHost(_host, _port);
    if (_waitFor([this]() { return _socket->state() == QAbstractSocket::ConnectedState; },
                 QDeadlineTimer(kConnectTimeoutMs))) {
        return true;
    }
    if (!isCancelled()) {
        qCWarning(TcpGPSTransportLog) << "Failed to connect to GPS receiver" << _host << _port
                                      << _socket->errorString();
    }
    _socket->abort();
    return false;
}

bool TcpGPSTransport::fatalError() const
{
    return !_socket || _socket->state() == QAbstractSocket::UnconnectedState;
}

int TcpGPSTransport::read(uint8_t* buffer, int length, int timeoutMs)
{
    if (isCancelled() || !_socket || !buffer || length < 0) {
        return -1;
    }
    if (length == 0) {
        return 0;
    }
    if (!_waitFor([this]() { return _socket->bytesAvailable() > 0; }, QDeadlineTimer((std::max) (timeoutMs, 0)))) {
        return (isCancelled() || fatalError()) ? -1 : 0;
    }
    return static_cast<int>(_socket->read(reinterpret_cast<char*>(buffer), length));
}

int TcpGPSTransport::write(const uint8_t* buffer, int length)
{
    const auto result = writeBounded(buffer, length, QDeadlineTimer(kWriteTimeoutMs));
    return result.status == WriteStatus::Completed ? result.writtenBytes : -1;
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
        return {WriteStatus::Error};
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
    return {status, acceptedBytes, writtenBytes, acceptedBytes - writtenBytes};
}

bool TcpGPSTransport::setBaudrate(unsigned baudrate)
{
    // A serial bridge must already use the receiver's baud rate; TCP cannot change it.
    return baudrate == fixedBaudrate();
}
