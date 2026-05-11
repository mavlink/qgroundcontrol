#include "RTCMUdpInput.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(RTCMUdpInputLog, "GPS.RTCMUdpInput")

RTCMUdpInput::RTCMUdpInput(quint16 port, QObject *parent)
    : QObject(parent)
    , _port(port)
{
    connect(&_socket, &QUdpSocket::readyRead, this, &RTCMUdpInput::_readDatagrams);
}

RTCMUdpInput::~RTCMUdpInput()
{
    stop();
}

bool RTCMUdpInput::start()
{
    stop();

    if (!_socket.bind(QHostAddress::AnyIPv4, _port)) {
        qCWarning(RTCMUdpInputLog) << "Failed to bind UDP socket on port" << _port
                                   << ":" << _socket.errorString();
        return false;
    }

    _running = true;
    emit runningChanged();
    qCDebug(RTCMUdpInputLog) << "Listening for RTCM data on UDP port" << _port;
    return true;
}

void RTCMUdpInput::stop()
{
    if (!_running) {
        return;
    }

    _socket.close();
    _running = false;
    emit runningChanged();
    qCDebug(RTCMUdpInputLog) << "Stopped listening on UDP port" << _port;
}

void RTCMUdpInput::setPort(quint16 port)
{
    if (_port == port) {
        return;
    }

    _port = port;
    emit portChanged();

    if (_running) {
        start(); // restart on new port
    }
}

void RTCMUdpInput::_readDatagrams()
{
    while (_socket.hasPendingDatagrams()) {
        const qint64 size = _socket.pendingDatagramSize();
        if (size <= 0) {
            (void) _socket.readDatagram(nullptr, 0); // discard malformed
            continue;
        }

        QByteArray data(static_cast<qsizetype>(size), Qt::Uninitialized);
        const qint64 read = _socket.readDatagram(data.data(), size);
        if (read <= 0) {
            qCWarning(RTCMUdpInputLog) << "readDatagram failed:" << _socket.errorString();
            continue;
        }

        if (read != size) {
            data.resize(static_cast<qsizetype>(read));
        }

        qCDebug(RTCMUdpInputLog) << "Received RTCM datagram:" << read << "bytes";
        emit rtcmDataReceived(data);
    }
}
