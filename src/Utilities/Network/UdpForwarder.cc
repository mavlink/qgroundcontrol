#include "UdpForwarder.h"

#include <QtNetwork/QUdpSocket>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(UdpForwarderLog, "GPS.UdpForwarder")

UdpForwarder::UdpForwarder(QObject* parent) : QObject(parent) {}

UdpForwarder::~UdpForwarder()
{
    stop();
}

bool UdpForwarder::configure(const QString& address, quint16 port)
{
    stop();

    const QHostAddress parsed(address);
    if (address.isEmpty() || parsed.isNull() || port == 0) {
        qCWarning(UdpForwarderLog) << "Invalid UDP forward config:" << address << port;
        return false;
    }

    _address = parsed;
    _port = port;
    _socket = new QUdpSocket(this);
    _enabled = true;

    qCDebug(UdpForwarderLog) << "UDP forwarding configured:" << address << ":" << port;
    return true;
}

void UdpForwarder::forward(const QByteArray& data)
{
    if (!_enabled || !_socket || _port == 0) {
        return;
    }

    // No rate limiting: RTCM runs 5-50 KB/s; writeDatagram()'s return value
    // surfaces saturation if it ever becomes a real problem.
    const qint64 sent = _socket->writeDatagram(data, _address, _port);
    if (sent < 0) {
        qCWarning(UdpForwarderLog) << "UDP forward failed:" << _socket->errorString();
    }
}

void UdpForwarder::stop()
{
    if (_socket) {
        _socket->close();
        _socket->deleteLater();
        _socket = nullptr;
    }
    _enabled = false;
    _port = 0;
    _address.clear();
}
