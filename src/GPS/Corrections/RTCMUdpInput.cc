#include "RTCMUdpInput.h"

#include <utility>

#include <QtCore/QPointer>
#include <QtNetwork/QNetworkDatagram>
#include <QtNetwork/QUdpSocket>

#include "MonotonicClock.h"
#include "QGCLoggingCategory.h"
#include "RuntimeScheduler.h"
#include "UdpPeer.h"

QGC_LOGGING_CATEGORY(RTCMUdpInputLog, "GPS.Corrections.RTCMUdpInput")

namespace {
/// The dual-stack socket reports IPv4 senders as IPv4-mapped IPv6 addresses; identify them by their IPv4 address.
QHostAddress senderAddress(const QNetworkDatagram& datagram)
{
    const QHostAddress sender = datagram.senderAddress();
    bool ipv4 = false;
    const quint32 address = sender.toIPv4Address(&ipv4);
    return ipv4 ? QHostAddress(address) : sender;
}
}  // namespace

RTCMUdpInput::RTCMUdpInput(QObject* parent, RuntimeScheduler* scheduler)
    : QObject(parent)
    , _scheduler(RuntimeScheduler::orDefault(scheduler, this))
{
    qCDebug(RTCMUdpInputLog) << this;
}

RTCMUdpInput::~RTCMUdpInput()
{
    qCDebug(RTCMUdpInputLog) << this;
    stop();
}

bool RTCMUdpInput::start(quint16 port, QString* errorString)
{
    stop();
    auto* const socket = new QUdpSocket(this);
    if (!socket->bind(QHostAddress::Any, port)) {
        qCDebug(RTCMUdpInputLog) << "Failed to bind UDP socket on port" << port << ":" << socket->errorString();
        if (errorString) {
            *errorString = socket->errorString();
        }
        delete socket;
        return false;
    }
    connect(socket, &QUdpSocket::readyRead, this, &RTCMUdpInput::_readDatagrams);
    connect(socket, &QAbstractSocket::errorOccurred, this, [socket]() {
        qCWarning(RTCMUdpInputLog) << "UDP socket error on port" << socket->localPort() << ":" << socket->errorString();
    });
    _socket = socket;
    qCDebug(RTCMUdpInputLog) << "Listening for RTCM data on UDP port" << socket->localPort();
    return true;
}

void RTCMUdpInput::stop()
{
    _drainScheduled = false;
    _peerParsers.clear();
    QUdpSocket* const socket = std::exchange(_socket, nullptr);
    if (!socket) {
        return;
    }
    qCDebug(RTCMUdpInputLog) << "Stopped listening on UDP port" << socket->localPort();
    socket->disconnect(this);
    socket->close();
    socket->deleteLater();
}

void RTCMUdpInput::_readDatagrams()
{
    QUdpSocket* const socket = _socket;
    if (!socket) {
        return;
    }
    // An observer that stops or restarts the input retires this socket.
    const auto current = [this, socket]() { return socket == _socket; };
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
        const qint64 receivedAtMs = _scheduler->nowMs();
        const QHostAddress sender = senderAddress(datagram);
        // A stream is named by its sender's address, so selection stays on a sender that restarts on a new source
        // port; each source port still frames independently.
        const QString instance = sender.toString();
        const auto peer = _parserForPeer(sender, datagram.senderPort());
        int framesFound = 0;
        int framesDropped = 0;
        const bool delivered = peer->decoder.feed(data, receivedAtMs, [&](const RTCMDecodedFrame& decoded) {
            if (!decoded.valid) {
                ++framesDropped;
                return current();
            }
            ++framesFound;
            emit frameReceived(instance, decoded.data, decoded.receivedAtMs);
            return current();
        });
        if (!delivered) {
            return;
        }

        qCDebug(RTCMUdpInputLog) << "Datagram" << data.size() << "bytes -" << "framesFound:" << framesFound
                                 << "framesDropped:" << framesDropped;
    }
    _scheduleRead();
}

void RTCMUdpInput::_scheduleRead()
{
    if (!_socket || !_socket->hasPendingDatagrams() || _drainScheduled) {
        return;
    }
    _drainScheduled = true;
    // Bounded drains yield to the event loop; a stop or restart before this runs retires the socket.
    QMetaObject::invokeMethod(
        this,
        [this, socket = QPointer<QUdpSocket>(_socket)]() {
            if (socket && socket == _socket) {
                _drainScheduled = false;
                _readDatagrams();
            }
        },
        Qt::QueuedConnection);
}

std::shared_ptr<RTCMUdpInput::PeerParser> RTCMUdpInput::_parserForPeer(const QHostAddress& address, quint16 port)
{
    const qint64 now = _scheduler->nowMs();
    for (auto it = _peerParsers.begin(); it != _peerParsers.end();) {
        if (const auto idle = MonotonicClock::age(std::chrono::milliseconds(it.value()->lastReceivedMs),
                                                  std::chrono::milliseconds(now));
            idle && *idle >= PEER_IDLE_TIMEOUT) {
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
