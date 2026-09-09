#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QString>

#include <atomic>
#include <functional>
#include <memory>

#include "GPSTransport.h"

class QUdpSocket;

/// Bidirectional UDP receiver link on the receiver worker, restricted to the configured peer.
/// Serial bridges and their receivers must already use 115200 baud.
class UdpGPSTransport : public GPSTransport
{
public:
    UdpGPSTransport(QString host, quint16 port, const std::atomic_bool& requestStop, quint16 localPort = 0);
    ~UdpGPSTransport() override;

    bool open() override;
    bool fatalError() const override;

    unsigned fixedBaudrate() const override { return 115200; }

    int read(uint8_t* buffer, int length, int timeoutMs) override;
    int write(const uint8_t* buffer, int length) override;
    WriteResult writeBounded(const uint8_t* buffer, int length, QDeadlineTimer deadline) override;
    bool setBaudrate(unsigned baudrate) override;

private:
    bool _waitFor(const std::function<bool()>& ready, QDeadlineTimer deadline);

    QString _host;
    quint16 _port;
    quint16 _localPort;
    std::unique_ptr<QUdpSocket> _socket;
    QByteArray _pending;
    qsizetype _pendingOffset = 0;
    bool _failed = false;

    static constexpr int kConnectTimeoutMs = 5000;
};
