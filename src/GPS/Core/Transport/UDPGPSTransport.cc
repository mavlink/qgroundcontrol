#include "UDPGPSTransport.h"

#include <algorithm>
#include <cstring>
#include <utility>

#include <QtNetwork/QNetworkDatagram>
#include <QtNetwork/QUdpSocket>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(UDPGPSTransportLog, "GPS.Transport.UDPGPSTransport")

UDPGPSTransport::UDPGPSTransport(quint16 port, GPSCancelToken cancelToken, std::chrono::milliseconds peerIdleTimeout)
    : GPSDeviceTransport(std::move(cancelToken))
    , _port(port)
    , _peerIdleTimeout(peerIdleTimeout)
{}

UDPGPSTransport::~UDPGPSTransport() = default;

GPSOpenResult UDPGPSTransport::open()
{
    if (isCancelled()) {
        return {GPSOpenStatus::Cancelled};
    }
    _socket = std::make_unique<QUdpSocket>();
    if (!_socket->bind(QHostAddress::Any, _port)) {
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
        // The dual-stack socket may report an IPv4 sender as an IPv4-mapped IPv6 address.
        const bool selected = _peerPort != 0 && datagram.senderPort() == _peerPort &&
                              datagram.senderAddress().isEqual(_peerAddress, QHostAddress::TolerantConversion);
        if (!selected) {
            if (_peerPort != 0 && _peerIdle.durationElapsed() < _peerIdleTimeout) {
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
        if (const qsizetype excess = _pending.size() - READ_BUFFER_BYTES; excess > 0) {
            _pending.remove(0, excess);
            qCWarning(UDPGPSTransportLog)
                << "Receiver input outran the decoder; dropped the oldest" << excess << "bytes";
        }
    }
}

QIODevice* UDPGPSTransport::device() const
{
    return _socket.get();
}

bool UDPGPSTransport::waitReadable(QDeadlineTimer deadline)
{
    const auto buffered = [this]() {
        _receivePending();
        return !_pending.isEmpty();
    };
    return buffered() || waitForDevice(buffered, deadline);
}

qint64 UDPGPSTransport::take(std::span<uint8_t> buffer)
{
    const qsizetype taken = (std::min) (static_cast<qsizetype>(buffer.size()), _pending.size());
    std::memcpy(buffer.data(), _pending.constData(), static_cast<size_t>(taken));
    _pending.remove(0, taken);
    return qint64(taken);
}

GPSWriteResult UDPGPSTransport::writeData(QByteArrayView, QDeadlineTimer)
{
    return {GPSWriteStatus::Unsupported};
}
