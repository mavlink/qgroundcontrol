#include "UdpGPSTransport.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDeadlineTimer>
#include <QtNetwork/QNetworkDatagram>
#include <QtNetwork/QUdpSocket>

#include <algorithm>
#include <cstring>
#include <utility>

#include "GPSSocketWait.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(UdpGPSTransportLog, "GPS.Driver.Transport.UdpGPSTransport")

UdpGPSTransport::UdpGPSTransport(QString host, quint16 port, const std::atomic_bool& requestStop, quint16 localPort)
    : GPSTransport(requestStop)
    , _host(std::move(host))
    , _port(port)
    , _localPort(localPort)
{
    qCDebug(UdpGPSTransportLog) << this;
}

UdpGPSTransport::~UdpGPSTransport()
{
    qCDebug(UdpGPSTransportLog) << this;
}

bool UdpGPSTransport::_waitFor(const std::function<bool()>& ready, QDeadlineTimer deadline)
{
    return gpsWaitForSocket(
        _socket.get(), ready, [this]() { return isCancelled(); }, [this]() { return fatalError(); }, deadline);
}

GPSTransport::OpenResult UdpGPSTransport::open()
{
    if (isCancelled()) {
        return {OpenStatus::Cancelled};
    }
    _pending.clear();
    _pendingOffset = 0;
    _failed = false;
    const QDeadlineTimer connectDeadline(kConnectTimeoutMs);
    _socket = std::make_unique<QUdpSocket>();
    QObject::connect(_socket.get(), &QUdpSocket::errorOccurred, _socket.get(), [this]() { _failed = true; });
    if (_socket->bind(QHostAddress::Any, _localPort, QAbstractSocket::DontShareAddress)) {
        _socket->connectToHost(_host, _port);
        if (_waitFor([this]() { return _socket->state() == QAbstractSocket::ConnectedState; }, connectDeadline)) {
            // UDP has no handshake; GPSProvider reports connected only after driver configuration succeeds.
            return {OpenStatus::Opened};
        }
    }
    if (!isCancelled()) {
        qCWarning(UdpGPSTransportLog) << "Failed to open UDP GPS receiver" << _host << _port << _socket->errorString();
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

bool UdpGPSTransport::fatalError() const
{
    return !_socket || _failed || _socket->state() == QAbstractSocket::UnconnectedState;
}

GPSTransport::ReadResult UdpGPSTransport::read(uint8_t* buffer, int length, int timeoutMs)
{
    if (isCancelled()) {
        return {ReadStatus::Cancelled};
    }
    if (!buffer || length < 0) {
        return {ReadStatus::InvalidData};
    }
    if (fatalError()) {
        return {ReadStatus::Closed, 0, _socket ? _socket->errorString() : QString()};
    }
    if (length == 0) {
        return {ReadStatus::TimedOut};
    }

    QDeadlineTimer deadline((std::max) (timeoutMs, 0));
    while (_pending.isEmpty()) {
        if (!_waitFor([this]() { return _socket->hasPendingDatagrams(); }, deadline)) {
            return {isCancelled()  ? ReadStatus::Cancelled
                    : fatalError() ? ReadStatus::Error
                                   : ReadStatus::TimedOut,
                    0, fatalError() ? _socket->errorString() : QString()};
        }
        const QNetworkDatagram datagram = _socket->receiveDatagram();
        if (!datagram.isValid()) {
            _failed = true;
            return {ReadStatus::Error, 0, _socket->errorString()};
        }
        _pending = datagram.data();
        _pendingOffset = 0;
        if (_pending.isEmpty() && deadline.hasExpired()) {
            return {ReadStatus::TimedOut};
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
    return {ReadStatus::Data, count};
}

std::chrono::milliseconds UdpGPSTransport::configurationWriteTimeout() const
{
    return std::chrono::milliseconds(200);
}

GPSTransport::WriteResult UdpGPSTransport::writeBounded(const uint8_t* buffer, int length, QDeadlineTimer deadline)
{
    if (isCancelled()) {
        return {WriteStatus::Cancelled};
    }
    if (!buffer || length < 0) {
        return {WriteStatus::InvalidData};
    }
    if (fatalError()) {
        return {WriteStatus::Error, 0, 0, 0, _socket ? _socket->errorString() : QStringLiteral("GPS socket is closed")};
    }
    if (deadline.hasExpired()) {
        return {WriteStatus::TimedOut};
    }
    if (length == 0) {
        return {WriteStatus::Completed};
    }
    const qint64 written = _socket->write(reinterpret_cast<const char*>(buffer), length);
    if (written != length) {
        _failed = true;
    }
    const int count = static_cast<int>(std::clamp(written, qint64(0), qint64(length)));
    return {written == length ? WriteStatus::Completed : WriteStatus::Error, count, count, 0};
}

bool UdpGPSTransport::setBaudrate(unsigned baudrate)
{
    return baudrate == fixedBaudrate();
}
