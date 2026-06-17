#include "RTCMUdpInput.h"

#include <QtNetwork/QNetworkDatagram>
#include <QtNetwork/QUdpSocket>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(RTCMUdpInputLog, "GPS.RTCMUdpInput")

RTCMUdpInput::RTCMUdpInput(quint16 port, QObject* parent) : QObject(parent), _port(port) {}

RTCMUdpInput::~RTCMUdpInput()
{
    stop();
}

bool RTCMUdpInput::start()
{
    stop();
    _rtcmParser.reset();

    _socket = new QUdpSocket(this);
    if (!_socket->bind(QHostAddress::AnyIPv4, _port)) {
        qCWarning(RTCMUdpInputLog) << "Failed to bind UDP socket on port" << _port << ":" << _socket->errorString();
        _socket->deleteLater();
        _socket = nullptr;
        return false;
    }
    connect(_socket, &QUdpSocket::readyRead, this, &RTCMUdpInput::_readDatagrams);

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

    if (_socket) {
        _socket->close();
        _socket->deleteLater();
        _socket = nullptr;
    }
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
        start();
    }
}

void RTCMUdpInput::_readDatagrams()
{
    if (!_socket) {
        return;
    }
    while (_socket->hasPendingDatagrams()) {
        const QNetworkDatagram datagram = _socket->receiveDatagram();
        const QByteArray data = datagram.data();
        if (data.isEmpty()) {
            continue;
        }

        if (!_validateRtcm) {
            qCDebug(RTCMUdpInputLog) << "Received RTCM datagram:" << data.size() << "bytes";
            emit rtcmDataReceived(data);
            continue;
        }

        int framesFound = 0;
        int framesDropped = 0;
        const QByteArray validData = _rtcmParser.extractValidFrames(data, &framesFound, &framesDropped);

        _validFrames += static_cast<quint64>(framesFound);
        _invalidFrames += static_cast<quint64>(framesDropped);
        if (framesDropped > 0) {
            qCWarning(RTCMUdpInputLog) << "Dropped" << framesDropped << "RTCM frame(s) - CRC mismatch";
        }

        qCDebug(RTCMUdpInputLog) << "Datagram" << data.size() << "bytes -"
                                 << "framesFound:" << framesFound << "framesDropped:" << framesDropped
                                 << "validData:" << validData.size() << "bytes";

        if (!validData.isEmpty()) {
            emit rtcmDataReceived(validData);
        }

        const quint64 totalFrames = _validFrames + _invalidFrames;
        if (totalFrames > 0) {
            const double dropPct = 100.0 * _invalidFrames / totalFrames;
            qCDebug(RTCMUdpInputLog) << QString("RTCM frame stats: %1 valid, %2 invalid, %3% dropped")
                                            .arg(_validFrames)
                                            .arg(_invalidFrames)
                                            .arg(dropPct, 0, 'f', 1);
        }
    }
}
