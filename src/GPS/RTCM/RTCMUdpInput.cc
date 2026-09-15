#include "RTCMUdpInput.h"

#include <utility>

#include <QtCore/QPointer>
#include <QtCore/QScopeGuard>
#include <QtNetwork/QNetworkDatagram>
#include <QtNetwork/QUdpSocket>

#include "GPSCorrectionFrame.h"
#include "QGCLoggingCategory.h"
#include "UdpPeer.h"

QGC_LOGGING_CATEGORY(RTCMUdpInputLog, "GPS.Corrections.RTCMUdpInput")

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
    const QPointer<RTCMUdpInput> guard(this);
    const quint64 revision = _lifecycleRevision + 1;
    stop();
    if (!guard || _lifecycleRevision != revision) {
        return false;
    }
    const QPointer<QUdpSocket> socket = new QUdpSocket(this);
    _socket = socket;
    auto rollback = qScopeGuard([guard, socket]() {
        if (guard && socket && socket == guard->_socket) {
            guard->stop();
        }
    });
    if (!socket->bind(QHostAddress::AnyIPv4, _port)) {
        qCWarning(RTCMUdpInputLog) << "Failed to bind UDP socket on port" << _port << ":" << socket->errorString();
        return false;
    }
    connect(socket, &QUdpSocket::readyRead, this, &RTCMUdpInput::_readDatagrams);
    connect(socket, &QAbstractSocket::errorOccurred, this, [this, guard, socket, revision]() {
        if (guard && socket && socket == _socket && revision == _lifecycleRevision) {
            qCWarning(RTCMUdpInputLog) << "UDP socket error on port" << _port << ":" << socket->errorString();
        }
    });

    if (_port == 0) {
        _port = socket->localPort();
        emit portChanged();
        if (!guard || _lifecycleRevision != revision) {
            return false;
        }
    }

    _running = true;
    emit runningChanged();
    if (!guard || _lifecycleRevision != revision) {
        return false;
    }
    rollback.dismiss();
    qCDebug(RTCMUdpInputLog) << "Listening for RTCM data on UDP port" << _port;
    return true;
}

void RTCMUdpInput::stop()
{
    const QPointer<RTCMUdpInput> guard(this);
    const quint64 revision = _resetStream();
    const bool wasRunning = std::exchange(_running, false);
    const QPointer<QUdpSocket> socket = std::exchange(_socket, nullptr);
    if (socket) {
        socket->close();
        if (socket) {
            socket->deleteLater();
        }
    }
    if (guard && revision == _lifecycleRevision && wasRunning) {
        qCDebug(RTCMUdpInputLog) << "Stopped listening on UDP port" << _port;
        emit runningChanged();
    }
}

void RTCMUdpInput::setPort(quint16 port)
{
    configure(port, _validateRtcm);
}

void RTCMUdpInput::setValidation(bool validate)
{
    configure(_port, validate);
}

void RTCMUdpInput::configure(quint16 port, bool validate)
{
    if (_port == port && _validateRtcm == validate) {
        return;
    }
    const QPointer<RTCMUdpInput> guard(this);
    const quint64 revision = _resetStream();
    const bool portHasChanged = _port != port;
    _port = port;
    _validateRtcm = validate;
    if (portHasChanged) {
        emit portChanged();
    }
    if (guard && revision == _lifecycleRevision && _running) {
        start();
    }
}

quint64 RTCMUdpInput::_resetStream()
{
    _drainScheduled = false;
    _peerParsers.clear();
    return ++_lifecycleRevision;
}

void RTCMUdpInput::_readDatagrams()
{
    if (!_running || !_socket || _readingDatagrams) {
        return;
    }
    const QPointer<RTCMUdpInput> guard(this);
    const QPointer<QUdpSocket> socket = _socket;
    const quint64 revision = _lifecycleRevision;
    _readingDatagrams = true;
    const auto finishReading = qScopeGuard([guard]() {
        if (guard) {
            guard->_readingDatagrams = false;
            guard->_scheduleRead();
        }
    });
    const auto current = [this, guard, socket, revision]() {
        return guard && socket && socket == _socket && revision == _lifecycleRevision && _running;
    };
    UdpDrainBudget budget;
    while (current() && socket->hasPendingDatagrams() && budget.available()) {
        const QNetworkDatagram datagram = socket->receiveDatagram();
        if (!datagram.isValid()) {
            break;
        }
        const QByteArray data = datagram.data();
        budget.consume(data.size());
        if (data.isEmpty()) {
            continue;
        }
        const qint64 receivedAtMs = GPSCorrectionFrame::monotonicNowMs();
        const QString instance = udpPeerKey(datagram.senderAddress(), datagram.senderPort());

        if (!_validateRtcm) {
            qCDebug(RTCMUdpInputLog) << "Received RTCM datagram:" << data.size() << "bytes";
            emit frameReceived({GPSCorrectionSource::Udp, 0, receivedAtMs, data, 0, false, false, instance});
            continue;
        }

        // Preserve sender boundaries and one MAVLink sequence per frame.
        const auto peer = _parserForPeer(datagram.senderAddress(), datagram.senderPort());
        int framesFound = 0;
        int framesDropped = 0;
        for (const char ch : data) {
            for (auto decoded = peer->decoder.addByte(static_cast<uint8_t>(ch), receivedAtMs); decoded;
                 decoded = peer->decoder.nextFrame()) {
                const GPSCorrectionFrame frame = {GPSCorrectionSource::Udp, 0,
                                                  decoded->receivedAtMs,    decoded->data,
                                                  decoded->messageId,       decoded->valid,
                                                  decoded->filtered,        instance};
                if (decoded->valid) {
                    ++framesFound;
                    ++_validFrames;
                    emit frameReceived(frame);
                } else {
                    ++framesDropped;
                    ++_invalidFrames;
                    emit frameRejected(frame, GPSCorrectionReason::InvalidFrame);
                }
                if (!current()) {
                    return;
                }
            }
        }

        if (framesDropped > 0) {
            qCWarning(RTCMUdpInputLog) << "Dropped" << framesDropped << "RTCM frame(s) - invalid framing or CRC";
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
}

void RTCMUdpInput::_scheduleRead()
{
    if (_running && _socket && _socket->hasPendingDatagrams() && !_drainScheduled) {
        _drainScheduled = true;
        const QPointer<QUdpSocket> socket = _socket;
        const quint64 revision = _lifecycleRevision;
        QMetaObject::invokeMethod(
            this,
            [this, socket, revision]() {
                if (socket && socket == _socket && revision == _lifecycleRevision) {
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
    const QString key = udpPeerKey(address, port);
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
