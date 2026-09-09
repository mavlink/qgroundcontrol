#pragma once

#include <QtCore/QString>

#include <atomic>
#include <functional>
#include <memory>

#include "GPSTransport.h"

class QTcpSocket;

/// Owns a TCP socket on the receiver worker. Cancellation is checked during every wait.
/// Serial bridges and their receivers must be configured for 115200 baud before connecting.
class TcpGPSTransport : public GPSTransport
{
public:
    TcpGPSTransport(QString host, quint16 port, const std::atomic_bool& requestStop);
    ~TcpGPSTransport() override;

    bool open() override;
    bool fatalError() const override;

    unsigned fixedBaudrate() const override { return 115200; }

    int read(uint8_t* buffer, int length, int timeoutMs) override;
    int write(const uint8_t* buffer, int length) override;
    bool setBaudrate(unsigned baudrate) override;

private:
    bool _waitFor(const std::function<bool()>& ready, int timeoutMs);

    QString _host;
    quint16 _port;
    std::unique_ptr<QTcpSocket> _socket;

    static constexpr int kConnectTimeoutMs = 5000;
    static constexpr int kWriteTimeoutMs = 5000;
    static constexpr int kCancellationPollMs = 50;
};
