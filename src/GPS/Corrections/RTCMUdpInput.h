#pragma once

#include <chrono>
#include <memory>

#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtNetwork/QHostAddress>

#include "RTCMFramer.h"

class QUdpSocket;
class RuntimeScheduler;

/// Receives RTCM datagrams with bounded work and independent framing for each sender.
/// Valid frames are emitted complete and stamped with the scheduler's time; candidates that fail framing or CRC are
/// dropped.
class RTCMUdpInput : public QObject
{
    Q_OBJECT

public:
    explicit RTCMUdpInput(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr);
    ~RTCMUdpInput() override;

    /// Listens on @a port with fresh framing state, replacing any current socket; port zero requests an ephemeral
    /// port. When binding fails, @a errorString receives the reason.
    bool start(quint16 port, QString* errorString = nullptr);

    void stop();

signals:
    /// A valid frame from @a sender, the sender's address, stamped @a receivedAtMs with the scheduler's time.
    void frameReceived(const QString& sender, const QByteArray& data, qint64 receivedAtMs);

private slots:
    void _readDatagrams();

private:
    void _scheduleRead();

    RuntimeScheduler* const _scheduler;
    QUdpSocket* _socket = nullptr;

    struct PeerParser
    {
        RTCMFrameDecoder decoder;
        qint64 lastReceivedMs = 0;
    };

    std::shared_ptr<PeerParser> _parserForPeer(const QHostAddress& address, quint16 port);
    QHash<QString, std::shared_ptr<PeerParser>> _peerParsers;
    static constexpr qsizetype MAX_PEERS = 32;
    static constexpr std::chrono::milliseconds PEER_IDLE_TIMEOUT{30000};
    bool _drainScheduled = false;
};
