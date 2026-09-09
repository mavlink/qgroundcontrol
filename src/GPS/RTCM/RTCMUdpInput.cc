#include "RTCMUdpInput.h"

#include <QtCore/QPointer>
#include <QtNetwork/QNetworkDatagram>
#include <QtNetwork/QUdpSocket>

#include "GPSCorrectionFrame.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(RTCMUdpInputLog, "GPS.RTCM.RTCMUdpInput")

RTCMUdpInput::RTCMUdpInput(quint16 port, QObject* parent)
    : QObject(parent)
    , _port(port)
{
    qCDebug(RTCMUdpInputLog) << this;
}

RTCMUdpInput::~RTCMUdpInput()
{
    qCDebug(RTCMUdpInputLog) << this;
    stop();
}

bool RTCMUdpInput::start()
{
    stop();
    _peerParsers.clear();

    _socket = new QUdpSocket(this);
    if (!_socket->bind(QHostAddress::AnyIPv4, _port)) {
        qCWarning(RTCMUdpInputLog) << "Failed to bind UDP socket on port" << _port << ":" << _socket->errorString();
        _socket->deleteLater();
        _socket = nullptr;
        return false;
    }
    connect(_socket, &QUdpSocket::readyRead, this, &RTCMUdpInput::_readDatagrams);

    if (_port == 0) {
        _port = _socket->localPort();
        emit portChanged();
    }

    _running = true;
    emit runningChanged();
    qCDebug(RTCMUdpInputLog) << "Listening for RTCM data on UDP port" << _port;
    return true;
}

void RTCMUdpInput::stop()
{
    _drainScheduled = false;
    if (!_running) {
        return;
    }

    if (_socket) {
        _socket->close();
        _socket->deleteLater();
        _socket = nullptr;
    }
    _running = false;
    _peerParsers.clear();
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
    const QPointer<RTCMUdpInput> guard(this);
    const QPointer<QUdpSocket> socket = _socket;
    qsizetype datagramsRead = 0;
    qsizetype bytesRead = 0;
    while (guard && socket && socket == _socket && socket->hasPendingDatagrams() &&
           datagramsRead < MAX_DATAGRAMS_PER_DRAIN && bytesRead < MAX_BYTES_PER_DRAIN) {
        const QNetworkDatagram datagram = socket->receiveDatagram();
        const QByteArray data = datagram.data();
        ++datagramsRead;
        bytesRead += data.size();
        if (data.isEmpty()) {
            continue;
        }
        const qint64 receivedAtMs = GPSCorrectionFrame::monotonicNowMs();
        const QString instance =
            datagram.senderAddress().toString() + QLatin1Char(':') + QString::number(datagram.senderPort());

        if (!_validateRtcm) {
            qCDebug(RTCMUdpInputLog) << "Received RTCM datagram:" << data.size() << "bytes";
            emit frameReceived({GPSCorrectionSource::Udp, 0, receivedAtMs, data, 0, false, false, instance});
            if (!guard || !socket || socket != _socket) {
                return;
            }
            emit correctionReceived(data, 0, false);
            if (!guard || !socket || socket != _socket) {
                return;
            }
            emit rtcmDataReceived(data);
            continue;
        }

        // Emit one complete RTCM3 frame per signal so RTCMMavlink assigns a distinct
        // GPS_RTCM_DATA sequence per frame (required for correct MAVLink reassembly).
        // A partial frame belongs to its sender, even when another sender interleaves datagrams.
        const auto peer = _parserForPeer(datagram.senderAddress(), datagram.senderPort());
        auto& parser = peer->parser;
        int framesFound = 0;
        int framesDropped = 0;
        for (const char ch : data) {
            if (peer->frameReceivedAtMs == 0 && static_cast<quint8>(ch) == 0xD3) {
                peer->frameReceivedAtMs = receivedAtMs;
            }
            if (!parser.addByte(static_cast<uint8_t>(static_cast<unsigned char>(ch)))) {
                if (!parser.hasPartialFrame()) {
                    peer->frameReceivedAtMs = 0;
                }
                continue;
            }
            if (parser.validateCrc()) {
                ++framesFound;
                ++_validFrames;
                const QByteArray frame = parser.currentFrame();
                const int messageId = parser.messageId();
                const qint64 frameTimestamp = peer->frameReceivedAtMs;
                parser.reset();
                peer->frameReceivedAtMs = 0;
                emit frameReceived(
                    {GPSCorrectionSource::Udp, 0, frameTimestamp, frame, messageId, true, false, instance});
                if (!guard || !socket || socket != _socket) {
                    return;
                }
                emit correctionReceived(frame, messageId, true);
                if (!guard || !socket || socket != _socket) {
                    return;
                }
                emit rtcmDataReceived(frame);
                if (!guard || !socket || socket != _socket) {
                    return;
                }
            } else {
                ++framesDropped;
                ++_invalidFrames;
            }
            parser.reset();
            peer->frameReceivedAtMs = 0;
        }

        if (framesDropped > 0) {
            qCWarning(RTCMUdpInputLog) << "Dropped" << framesDropped << "RTCM frame(s) - CRC mismatch";
        }

        qCDebug(RTCMUdpInputLog) << "Datagram" << data.size() << "bytes -" << "framesFound:" << framesFound
                                 << "framesDropped:" << framesDropped;

        const quint64 totalFrames = _validFrames + _invalidFrames;
        if (totalFrames > 0) {
            const double dropPct = 100.0 * _invalidFrames / totalFrames;
            qCDebug(RTCMUdpInputLog) << QString("RTCM frame stats: %1 valid, %2 invalid, %3% dropped")
                                            .arg(_validFrames)
                                            .arg(_invalidFrames)
                                            .arg(dropPct, 0, 'f', 1);
        }
    }
    if (guard && socket && socket == _socket && socket->hasPendingDatagrams() && !_drainScheduled) {
        _drainScheduled = true;
        QMetaObject::invokeMethod(
            this,
            [this, socket]() {
                if (socket && socket == _socket) {
                    _drainScheduled = false;
                    _readDatagrams();
                }
            },
            Qt::QueuedConnection);
    }
}

std::shared_ptr<RTCMUdpInput::PeerParser> RTCMUdpInput::_parserForPeer(const QHostAddress& address, quint16 port)
{
    const qint64 now = GPSCorrectionFrame::monotonicNowMs();
    for (auto it = _peerParsers.begin(); it != _peerParsers.end();) {
        if (now - it.value()->lastReceivedMs >= PEER_IDLE_TIMEOUT_MS) {
            it = _peerParsers.erase(it);
        } else {
            ++it;
        }
    }
    const QString key = address.toString() + QLatin1Char(':') + QString::number(port);
    auto peer = _peerParsers.value(key);
    if (!peer) {
        if (_peerParsers.size() >= MAX_PEERS) {
            auto oldest = _peerParsers.begin();
            for (auto it = _peerParsers.begin(); it != _peerParsers.end(); ++it) {
                if (it.value()->lastReceivedMs < oldest.value()->lastReceivedMs) {
                    oldest = it;
                }
            }
            _peerParsers.erase(oldest);
        }
        peer = std::make_shared<PeerParser>();
        _peerParsers.insert(key, peer);
    }
    peer->lastReceivedMs = now;
    return peer;
}
