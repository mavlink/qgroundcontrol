#include "UDPGPSTransport.h"

#include <algorithm>
#include <cstring>

#include <QtNetwork/QNetworkDatagram>
#include <QtNetwork/QUdpSocket>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(UDPGPSTransportLog, "GPS.Transport.UDPGPSTransport")

UDPGPSTransport::UDPGPSTransport(quint16 port, const std::atomic_bool& requestStop, int peerIdleTimeoutMs)
    : GPSTransport(requestStop)
    , _port(port)
    , _peerIdleTimeoutMs(peerIdleTimeoutMs)
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
    _socket = std::make_unique<QUdpSocket>();
    if (!_socket->bind(QHostAddress::AnyIPv4, _port)) {
        qCWarning(UDPGPSTransportLog) << "Cannot listen for receiver data on UDP port" << _port
                                      << _socket->errorString();
        return {GPSOpenStatus::Error, _socket->errorString()};
    }
    return {GPSOpenStatus::Opened};
}

bool UDPGPSTransport::fatalError() const
{
    return !_socket || _socket->state() != QAbstractSocket::BoundState;
}

void UDPGPSTransport::_receivePending()
{
    while (_socket && _socket->hasPendingDatagrams()) {
        const QNetworkDatagram datagram = _socket->receiveDatagram();
        if (!datagram.isValid()) {
            break;
        }
        const bool selected =
            _peerPort != 0 && datagram.senderPort() == _peerPort && datagram.senderAddress() == _peerAddress;
        if (!selected) {
            if (_peerPort != 0 && _peerIdle.isValid() && _peerIdle.elapsed() < _peerIdleTimeoutMs) {
                continue;
            }
            qCDebug(UDPGPSTransportLog) << "Receiving from UDP sender" << datagram.senderAddress()
                                        << datagram.senderPort();
            _peerAddress = datagram.senderAddress();
            _peerPort = static_cast<quint16>(datagram.senderPort());
            // A replaced sender's partial sentence must not join the new stream.
            _pending.clear();
        }
        _peerIdle.start();
        _pending.append(datagram.data());
        if (_pending.size() > kMaxBufferedBytes) {
            _pending.remove(0, _pending.size() - kMaxBufferedBytes);
        }
    }
}

GPSReadResult UDPGPSTransport::read(uint8_t* buffer, int length, int timeoutMs)
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
    const QDeadlineTimer deadline((std::max) (timeoutMs, 0), Qt::PreciseTimer);
    _receivePending();
    while (_pending.isEmpty()) {
        if (isCancelled()) {
            return {GPSReadStatus::Cancelled};
        }
        if (fatalError()) {
            return {GPSReadStatus::Closed, 0, _socket->errorString()};
        }
        if (deadline.hasExpired()) {
            return {GPSReadStatus::TimedOut};
        }
        (void) _socket->waitForReadyRead(
            static_cast<int>((std::min) (qint64(kCancellationPollMs), deadline.remainingTime())));
        _receivePending();
    }
    const auto count = static_cast<int>((std::min) (qsizetype(length), _pending.size()));
    std::memcpy(buffer, _pending.constData(), static_cast<size_t>(count));
    _pending.remove(0, count);
    return {GPSReadStatus::Data, count};
}

GPSWriteResult UDPGPSTransport::writeData(const uint8_t*, int, QDeadlineTimer)
{
    return {GPSWriteStatus::Unsupported};
}

bool UDPGPSTransport::setBaudrate(unsigned baudrate)
{
    return !isCancelled() && !fatalError() && baudrate == fixedBaudrate();
}
