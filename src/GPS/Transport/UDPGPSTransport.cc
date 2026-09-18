#include "UDPGPSTransport.h"

#include <algorithm>
#include <cstring>
#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QDeadlineTimer>
#include <QtNetwork/QNetworkDatagram>
#include <QtNetwork/QUdpSocket>

#include "GPSSocketWait_p.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(UDPGPSTransportLog, "GPS.Transport.UDPGPSTransport")

UDPGPSTransport::UDPGPSTransport(QString host, quint16 port, const std::atomic_bool& requestStop, quint16 localPort)
    : GPSTransport(requestStop), _host(std::move(host)), _port(port), _localPort(localPort)
{
    qCDebug(UDPGPSTransportLog) << this;
}

UDPGPSTransport::~UDPGPSTransport()
{
    qCDebug(UDPGPSTransportLog) << this;
}

GPSOpenResult UDPGPSTransport::open()
{
    if (isCancelled()) {
        return {GPSOpenStatus::Cancelled};
    }
    _pending.clear();
    _pendingOffset = 0;
    _failed = false;
    const QDeadlineTimer connectDeadline(kConnectTimeoutMs);
    _socket = std::make_unique<QUdpSocket>();
    QObject::connect(_socket.get(), &QUdpSocket::errorOccurred, _socket.get(), [this]() { _failed = true; });
    if (_socket->bind(QHostAddress::Any, _localPort, QAbstractSocket::DontShareAddress)) {
        _socket->connectToHost(_host, _port);
        if (gpsWaitForSocket(
                *this, _socket.get(), [this]() { return _socket->state() == QAbstractSocket::ConnectedState; },
                connectDeadline, kCancellationPollMs)) {
            // UDP has no handshake; GPSProvider reports connected only after driver configuration succeeds.
            return {GPSOpenStatus::Opened};
        }
    }
    if (!isCancelled()) {
        qCWarning(UDPGPSTransportLog) << "Failed to open UDP GPS receiver" << _host << _port << _socket->errorString();
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

bool UDPGPSTransport::fatalError() const
{
    return !_socket || _failed || _socket->state() == QAbstractSocket::UnconnectedState;
}

GPSReadResult UDPGPSTransport::read(uint8_t* buffer, int length, int timeoutMs)
{
    if (isCancelled()) {
        return {GPSReadStatus::Cancelled};
    }
    if (!buffer || length < 0) {
        return {GPSReadStatus::InvalidData};
    }
    if (fatalError()) {
        return {GPSReadStatus::Closed, 0, _socket ? _socket->errorString() : QString()};
    }
    if (length == 0) {
        return {GPSReadStatus::Data};
    }

    QDeadlineTimer deadline((std::max) (timeoutMs, 0));
    while (_pending.isEmpty()) {
        if (!gpsWaitForSocket(
                *this, _socket.get(), [this]() { return _socket->hasPendingDatagrams(); }, deadline,
                kCancellationPollMs)) {
            return {isCancelled()  ? GPSReadStatus::Cancelled
                    : fatalError() ? GPSReadStatus::Error
                                   : GPSReadStatus::TimedOut,
                    0, fatalError() ? _socket->errorString() : QString()};
        }
        const QNetworkDatagram datagram = _socket->receiveDatagram();
        if (!datagram.isValid()) {
            _failed = true;
            return {GPSReadStatus::Error, 0, _socket->errorString()};
        }
        _pending = datagram.data();
        _pendingOffset = 0;
        if (_pending.isEmpty() && deadline.hasExpired()) {
            return {GPSReadStatus::TimedOut};
        }
    }

    // Keep the remainder when the driver requests fewer bytes than a whole datagram.
    const int count = static_cast<int>((std::min) (static_cast<qsizetype>(length), _pending.size() - _pendingOffset));
    std::memcpy(buffer, _pending.constData() + _pendingOffset, count);
    _pendingOffset += count;
    if (_pendingOffset == _pending.size()) {
        _pending.clear();
        _pendingOffset = 0;
    }
    return {GPSReadStatus::Data, count};
}

std::chrono::milliseconds UDPGPSTransport::configurationWriteTimeout() const
{
    return std::chrono::milliseconds(200);
}

GPSWriteResult UDPGPSTransport::writeBounded(const uint8_t* buffer, int length, QDeadlineTimer deadline)
{
    if (isCancelled()) {
        return {GPSWriteStatus::Cancelled};
    }
    if (!buffer || length < 0 || length > kMaxDatagramBytes) {
        return {GPSWriteStatus::InvalidData};
    }
    if (fatalError()) {
        return {GPSWriteStatus::Error, 0, 0, _socket ? _socket->errorString() : QStringLiteral("GPS socket is closed")};
    }
    if (deadline.hasExpired()) {
        return {GPSWriteStatus::TimedOut};
    }
    if (length == 0) {
        return {GPSWriteStatus::Completed};
    }
    const qint64 written = _socket->write(reinterpret_cast<const char*>(buffer), length);
    if (written != length) {
        _failed = true;
    }
    const int count = static_cast<int>(std::clamp(written, qint64(0), qint64(length)));
    return {written == length ? GPSWriteStatus::Completed : GPSWriteStatus::Error, count, count};
}

bool UDPGPSTransport::setBaudrate(unsigned baudrate)
{
    return !isCancelled() && !fatalError() && baudrate == fixedBaudrate();
}
