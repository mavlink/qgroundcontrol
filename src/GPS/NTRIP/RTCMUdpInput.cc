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
    _rtcmParser.reset();

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
            (void) _socket.readDatagram(nullptr, 0);
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

        if (!_validateRtcm) {
            qCDebug(RTCMUdpInputLog) << "Received RTCM datagram:" << read << "bytes";
            emit rtcmDataReceived(data);
            return;
        }

        // RTCM Validation keeps track of some stats to see what % of the stream is garbage.

        for (qsizetype i = 0; i < data.size(); ++i) {
            if (!_rtcmParser.addByte(static_cast<uint8_t>(data[i]))) {
                continue;
            }

            if (_rtcmParser.validateCrc()) {
                const uint16_t frameSize = RTCMParser::kHeaderSize + _rtcmParser.messageLength() + RTCMParser::kCrcSize;
                QByteArray frame(reinterpret_cast<const char*>(_rtcmParser.message()), frameSize);
                _validBytes += frameSize;
                qCDebug(RTCMUdpInputLog) << "RTCM message" << _rtcmParser.messageId() << frameSize << "bytes";
                emit rtcmDataReceived(frame);
            } else {
                const uint16_t frameSize = RTCMParser::kHeaderSize + _rtcmParser.messageLength() + RTCMParser::kCrcSize;
                qCWarning(RTCMUdpInputLog) << "Dropped RTCM message" << _rtcmParser.messageId() << "- CRC mismatch";
                _invalidBytes += frameSize;
            }

            _rtcmParser.reset();
        }

        const quint64 totalBytes = _validBytes + _invalidBytes;
        if (totalBytes > 0 && (totalBytes % 100000) < static_cast<quint64>(data.size())) {
            const double dropPct = 100.0 * _invalidBytes / totalBytes;
            qCDebug(RTCMUdpInputLog) << QString("RTCM byte stats: %1 valid, %2 invalid, %3% dropped")
                                            .arg(_validBytes).arg(_invalidBytes).arg(dropPct, 0, 'f', 1);
        }
    }
}
