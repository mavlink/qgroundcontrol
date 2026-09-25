#pragma once

#include <atomic>
#include <memory>

#include <QtCore/QByteArray>
#include <QtCore/QElapsedTimer>
#include <QtNetwork/QHostAddress>

#include "GPSTransport.h"

class QUdpSocket;

/// Receive-only GNSS stream from UDP datagrams, for receivers that QGroundControl does not configure.
/// The first sender is selected; another sender replaces it only after the selected one stays silent for
/// kPeerIdleTimeoutMs. Owns its socket on the receiver worker.
class UDPGPSTransport : public GPSTransport
{
    friend class UDPGPSTransportTest;

public:
    static constexpr unsigned FIXED_BAUDRATE = 115200;
    static constexpr qsizetype kMaxBufferedBytes = 64 * 1024;
    static constexpr int kPeerIdleTimeoutMs = 5000;

    UDPGPSTransport(quint16 port, const std::atomic_bool& requestStop, int peerIdleTimeoutMs = kPeerIdleTimeoutMs);
    ~UDPGPSTransport() override;

    GPSOpenResult open() override;
    bool fatalError() const override;

    /// Datagrams carry no line rate; drivers see the nominal rate.
    unsigned fixedBaudrate() const override { return FIXED_BAUDRATE; }

    GPSReadResult read(uint8_t* buffer, int length, int timeoutMs) override;
    bool setBaudrate(unsigned baudrate) override;

protected:
    GPSWriteResult writeData(const uint8_t* buffer, int length, QDeadlineTimer deadline) override;

private:
    void _receivePending();

    quint16 _port;
    int _peerIdleTimeoutMs;
    std::unique_ptr<QUdpSocket> _socket;
    QByteArray _pending;
    QHostAddress _peerAddress;
    quint16 _peerPort = 0;
    QElapsedTimer _peerIdle;
};
