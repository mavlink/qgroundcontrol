#pragma once

#include <chrono>
#include <memory>
#include <stop_token>

#include <QtCore/QByteArray>
#include <QtCore/QElapsedTimer>
#include <QtNetwork/QHostAddress>

#include "GPSTransport.h"

class QUdpSocket;

/// Receive-only GNSS stream from UDP datagrams, for receivers that QGroundControl does not configure.
/// The first sender is selected; another sender replaces it only after the selected one stays silent for
/// kPeerIdleTimeout. Owns its socket on the receiver worker.
class UDPGPSTransport : public GPSTransport
{
    friend class UDPGPSTransportTest;

public:
    static constexpr qsizetype kMaxBufferedBytes = 64 * 1024;
    static constexpr std::chrono::milliseconds kPeerIdleTimeout{5000};

    UDPGPSTransport(quint16 port, std::stop_token stopToken,
                    std::chrono::milliseconds peerIdleTimeout = kPeerIdleTimeout);
    ~UDPGPSTransport() override;

    GPSOpenResult open() override;
    bool fatalError() const override;

    /// Datagrams carry no line rate; drivers see the bridge rate.
    unsigned fixedBaudrate() const override { return BRIDGE_BAUDRATE; }

    GPSReadResult read(uint8_t* buffer, int length, std::chrono::milliseconds timeout) override;
    bool setBaudrate(unsigned baudrate) override;

protected:
    GPSWriteResult writeData(const uint8_t* buffer, int length, QDeadlineTimer deadline) override;

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
