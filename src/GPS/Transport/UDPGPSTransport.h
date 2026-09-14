#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QString>

#include <atomic>
#include <memory>

#include "GPSTransport.h"

class QUdpSocket;

/// Bidirectional UDP receiver link on the receiver worker, restricted to the configured peer.
/// Serial bridges and their receivers must already use 115200 baud.
class UDPGPSTransport : public GPSTransport
{
public:
    static constexpr int kMaxDatagramBytes = 65507;

    UDPGPSTransport(QString host, quint16 port, const std::atomic_bool& requestStop, quint16 localPort = 0);
    ~UDPGPSTransport() override;

    OpenResult open() override;
    bool fatalError() const override;

    unsigned fixedBaudrate() const override { return 115200; }

    ReadResult read(uint8_t* buffer, int length, int timeoutMs) override;
    std::chrono::milliseconds configurationWriteTimeout() const override;
    WriteResult writeBounded(const uint8_t* buffer, int length, QDeadlineTimer deadline) override;
    bool setBaudrate(unsigned baudrate) override;

private:
    QString _host;
    quint16 _port;
    quint16 _localPort;
    std::unique_ptr<QUdpSocket> _socket;
    QByteArray _pending;
    qsizetype _pendingOffset = 0;
    bool _failed = false;

    static constexpr int kConnectTimeoutMs = 5000;
};
