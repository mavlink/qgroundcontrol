#pragma once

#include <chrono>
#include <memory>

#include <QtCore/QByteArray>
#include <QtCore/QElapsedTimer>
#include <QtNetwork/QHostAddress>

#include "GPSDeviceTransport.h"

class QUdpSocket;

/// Receive-only GNSS stream from UDP datagrams, for receivers that QGroundControl does not configure.
/// The first sender is selected; another sender replaces it only after the selected one stays silent for
/// PEER_IDLE_TIMEOUT. Datagrams cannot be held back, so input beyond READ_BUFFER_BYTES drops the oldest buffered
/// bytes, with a warning; the decoders' checksums reject the frames a drop cuts. Owns its socket on the receiver
/// worker.
class UDPGPSTransport : public GPSDeviceTransport
{
public:
    static constexpr std::chrono::milliseconds PEER_IDLE_TIMEOUT{5000};

    UDPGPSTransport(quint16 port, GPSCancelToken cancelToken,
                    std::chrono::milliseconds peerIdleTimeout = PEER_IDLE_TIMEOUT);
    ~UDPGPSTransport() override;

    GPSOpenResult open() override;
    bool fatalError() const override;

    /// Datagrams carry no line rate; drivers see the bridge rate.
    unsigned fixedBaudrate() const override { return BRIDGE_BAUDRATE; }

protected:
    QIODevice* device() const override;
    bool waitReadable(QDeadlineTimer deadline) override;
    qint64 take(std::span<uint8_t> buffer) override;
    GPSWriteResult writeData(QByteArrayView bytes, QDeadlineTimer deadline) override;

private:
    void _receivePending();

    quint16 _port;
    std::chrono::milliseconds _peerIdleTimeout;
    std::unique_ptr<QUdpSocket> _socket;
    QByteArray _pending;
    QHostAddress _peerAddress;
    quint16 _peerPort = 0;
    QElapsedTimer _peerIdle;
};
