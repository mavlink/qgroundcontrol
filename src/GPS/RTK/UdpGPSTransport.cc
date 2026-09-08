#include "UdpGPSTransport.h"

#include <QtCore/QDeadlineTimer>
#include <QtCore/QEventLoop>
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkDatagram>
#include <QtNetwork/QUdpSocket>

#include <algorithm>
#include <cstring>
#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(UdpGPSTransportLog, "GPS.RTK.UdpGPSTransport")

UdpGPSTransport::UdpGPSTransport(QString host, quint16 port, const std::atomic_bool& requestStop, quint16 localPort)
    : _host(std::move(host))
    , _port(port)
    , _requestStop(requestStop)
    , _localPort(localPort)
{
    qCDebug(UdpGPSTransportLog) << this;
}

UdpGPSTransport::~UdpGPSTransport()
{
    qCDebug(UdpGPSTransportLog) << this;
}

bool UdpGPSTransport::_waitFor(const std::function<bool()>& ready, int timeoutMs)
{
    if (_requestStop || fatalError()) {
        return false;
    }
    if (ready()) {
        return true;
    }
    if (timeoutMs <= 0) {
        return false;
    }

    QEventLoop loop;
    QTimer cancellation;
    QTimer deadline;
    const auto check = [&]() {
        if (_requestStop || fatalError() || ready()) {
            loop.quit();
        }
    };
    QObject::connect(_socket.get(), &QUdpSocket::stateChanged, &loop, check);
    QObject::connect(_socket.get(), &QUdpSocket::errorOccurred, &loop, check);
    QObject::connect(_socket.get(), &QUdpSocket::readyRead, &loop, check);
    QObject::connect(&cancellation, &QTimer::timeout, &loop, check);
    QObject::connect(&deadline, &QTimer::timeout, &loop, &QEventLoop::quit);
    deadline.setSingleShot(true);
    cancellation.start(kCancellationPollMs);
    deadline.start(timeoutMs);
    loop.exec();
    return !_requestStop && !fatalError() && ready();
}

bool UdpGPSTransport::open()
{
    if (_requestStop) {
        return false;
    }
    _pending.clear();
    _pendingOffset = 0;
    _failed = false;
    _socket = std::make_unique<QUdpSocket>();
    QObject::connect(_socket.get(), &QUdpSocket::errorOccurred, _socket.get(), [this]() { _failed = true; });
    if (_socket->bind(QHostAddress::Any, _localPort, QAbstractSocket::DontShareAddress)) {
        _socket->connectToHost(_host, _port);
        if (_waitFor([this]() { return _socket->state() == QAbstractSocket::ConnectedState; }, kConnectTimeoutMs)) {
            // UDP has no handshake; GPSProvider reports connected only after driver configuration succeeds.
            return true;
        }
    }
    if (!_requestStop) {
        qCWarning(UdpGPSTransportLog) << "Failed to open UDP GPS receiver" << _host << _port << _socket->errorString();
    }
    _socket->abort();
    return false;
}

bool UdpGPSTransport::fatalError() const
{
    return !_socket || _failed || _socket->state() == QAbstractSocket::UnconnectedState;
}

int UdpGPSTransport::read(uint8_t* buffer, int length, int timeoutMs)
{
    if (_requestStop || fatalError() || !buffer || length < 0) {
        return -1;
    }
    if (length == 0) {
        return 0;
    }

    QDeadlineTimer deadline((std::max) (timeoutMs, 0));
    while (_pending.isEmpty()) {
        if (!_waitFor([this]() { return _socket->hasPendingDatagrams(); },
                      static_cast<int>(deadline.remainingTime()))) {
            return (_requestStop || fatalError()) ? -1 : 0;
        }
        const QNetworkDatagram datagram = _socket->receiveDatagram();
        if (!datagram.isValid()) {
            _failed = true;
            return -1;
        }
        _pending = datagram.data();
        _pendingOffset = 0;
        if (_pending.isEmpty() && deadline.hasExpired()) {
            return 0;
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
    return count;
}

int UdpGPSTransport::write(const uint8_t* buffer, int length)
{
    if (_requestStop || fatalError() || !buffer || length < 0) {
        return -1;
    }
    if (length == 0) {
        return 0;
    }
    if (_socket->write(reinterpret_cast<const char*>(buffer), length) != length) {
        _failed = true;
        return -1;
    }
    return length;
}

bool UdpGPSTransport::setBaudrate(unsigned baudrate)
{
    return baudrate == fixedBaudrate();
}
