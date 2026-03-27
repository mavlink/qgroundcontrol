#include "NTRIPUdpForwarder.h"
#include "QGCLoggingCategory.h"

#include <QtNetwork/QUdpSocket>

QGC_LOGGING_CATEGORY(NTRIPUdpForwarderLog, "GPS.NTRIPUdpForwarder")

NTRIPUdpForwarder::NTRIPUdpForwarder(QObject *parent)
    : QObject(parent)
{
}

NTRIPUdpForwarder::~NTRIPUdpForwarder()
{
    stop();
}

bool NTRIPUdpForwarder::configure(const QString &address, quint16 port)
{
    stop();

    const QHostAddress parsed(address);
    if (address.isEmpty() || parsed.isNull() || port == 0) {
        qCWarning(NTRIPUdpForwarderLog) << "Invalid UDP forward config:" << address << port;
        return false;
    }

    _address = parsed;
    _port = port;
    _socket = new QUdpSocket(this);
    _enabled = true;

    qCDebug(NTRIPUdpForwarderLog) << "UDP forwarding configured:" << address << ":" << port;
    return true;
}

void NTRIPUdpForwarder::forward(const QByteArray &data)
{
    if (!_enabled || !_socket || _port == 0) {
        return;
    }

    const qint64 sent = _socket->writeDatagram(data, _address, _port);
    if (sent < 0) {
        qCWarning(NTRIPUdpForwarderLog) << "UDP forward failed:" << _socket->errorString();
    }
}

void NTRIPUdpForwarder::stop()
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
