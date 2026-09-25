#include "UDPGPSTransport.h"

#include <algorithm>
#include <cstring>
#include <utility>

#include <QtNetwork/QNetworkDatagram>
#include <QtNetwork/QUdpSocket>

#include "GPSSocketWait_p.h"
#include "GPSStreamRead_p.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(UDPGPSTransportLog, "GPS.Transport.UDPGPSTransport")

UDPGPSTransport::UDPGPSTransport(quint16 port, std::stop_token stopToken, std::chrono::milliseconds peerIdleTimeout)
    : GPSTransport(std::move(stopToken))
    , _port(port)
    , _peerIdleTimeout(peerIdleTimeout)
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
            if (_peerPort != 0 && _peerIdle.isValid() && _peerIdle.durationElapsed() < _peerIdleTimeout) {
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

GPSReadResult UDPGPSTransport::read(uint8_t* buffer, int length, std::chrono::milliseconds timeout)
{
    const auto buffered = [this]() {
        _receivePending();
        return !_pending.isEmpty();
    };
    return GPSStreamRead::readBounded(
        *this, buffer, length, timeout, [this]() { return !_socket; },
        [this, &buffered](QDeadlineTimer deadline) {
            return buffered() || GPSSocketWait::waitFor(*this, _socket.get(), buffered, deadline);
        },
        [this](uint8_t* bytes, int count) {
            const qsizetype taken = (std::min) (qsizetype(count), _pending.size());
            std::memcpy(bytes, _pending.constData(), static_cast<size_t>(taken));
            _pending.remove(0, taken);
            return qint64(taken);
        },
        [this]() { return GPSReadResult{GPSReadStatus::Closed, 0, _socket ? _socket->errorString() : QString()}; });
}

GPSWriteResult UDPGPSTransport::writeData(const uint8_t*, int, QDeadlineTimer)
{
    return {GPSWriteStatus::Unsupported};
}

bool UDPGPSTransport::setBaudrate(unsigned baudrate)
{
    return !isCancelled() && !fatalError() && baudrate == fixedBaudrate();
}
