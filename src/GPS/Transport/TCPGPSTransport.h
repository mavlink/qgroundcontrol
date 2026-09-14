#pragma once

#include <QtCore/QDeadlineTimer>
#include <QtCore/QString>

#include <atomic>
#include <memory>

#include "GPSTransport.h"

class QTcpSocket;

/// Owns a TCP socket on the receiver worker. Cancellation is checked during every wait.
/// Serial bridges and their receivers must be configured for 115200 baud before connecting.
class TCPGPSTransport : public GPSTransport
{
    friend class TCPGPSTransportTest;

public:
    static constexpr qint64 kWriteBufferBytes = 4 * 1024;
    static constexpr qint64 kReadBufferBytes = 64 * 1024;

    TCPGPSTransport(QString host, quint16 port, const std::atomic_bool& requestStop);
    ~TCPGPSTransport() override;

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
    std::unique_ptr<QTcpSocket> _socket;

    static constexpr int kConnectTimeoutMs = 5000;
    static constexpr int kWriteTimeoutMs = 5000;
};
