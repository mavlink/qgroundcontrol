#pragma once

#include <QtCore/QHash>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtNetwork/QHostAddress>

#include <memory>

#include "GPSCorrectionDiagnostics.h"
#include "GPSCorrectionFrame.h"
#include "RTCMFrameDecoder.h"

Q_DECLARE_LOGGING_CATEGORY(RTCMUdpInputLog)

class QUdpSocket;

/// Receives RTCM datagrams with bounded work and independent framing for each sender.
/// Validated input emits complete timestamped frames; raw mode preserves datagram boundaries.
class RTCMUdpInput : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)
    Q_PROPERTY(quint16 port READ port NOTIFY portChanged)

public:
    explicit RTCMUdpInput(quint16 port, QObject* parent = nullptr);
    ~RTCMUdpInput() override;

    /// Bind the socket and begin accepting datagrams.
    /// Safe to call on an already-running instance — restarts with the current port.
    /// Port 0 binds an ephemeral port; port() then reports the bound port.
    bool start();

    /// Unbind the socket and stop accepting datagrams.
    void stop();

    bool isRunning() const { return _running; }

    quint16 port() const { return _port; }

    /// Change the listen port. If already running, restarts automatically.
    void setPort(quint16 port);

    /// Enable/disable validation of RTCM data.
    /// With this enabled, only valid RTCM packets are converted to MAVLink.
    void setValidation(const bool validate) { _validateRtcm = validate; }

signals:
    void frameReceived(const GPSCorrectionFrame& frame);
    void frameRejected(const GPSCorrectionFrame& frame, GPSCorrectionReason reason);

    void runningChanged();
    void portChanged();

private slots:
    void _readDatagrams();

private:
    QUdpSocket* _socket = nullptr;
    quint16 _port;
    bool _running = false;
    bool _validateRtcm = false;

    struct PeerParser
    {
        RTCMFrameDecoder decoder;
        qint64 lastReceivedMs = 0;
    };

    std::shared_ptr<PeerParser> _parserForPeer(const QHostAddress& address, quint16 port);
    QHash<QString, std::shared_ptr<PeerParser>> _peerParsers;
    static constexpr qsizetype MAX_PEERS = 32;
    static constexpr qint64 PEER_IDLE_TIMEOUT_MS = 30000;
    quint64 _validFrames = 0;
    quint64 _invalidFrames = 0;
    bool _drainScheduled = false;
    quint64 _lifecycleRevision = 0;
    static constexpr qsizetype MAX_DATAGRAMS_PER_DRAIN = 16;
    static constexpr qsizetype MAX_BYTES_PER_DRAIN = 64 * 1024;
};
