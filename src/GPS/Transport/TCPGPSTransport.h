#pragma once

#include <chrono>
#include <memory>
#include <stop_token>

#include <QtCore/QDeadlineTimer>
#include <QtCore/QString>

#include "GPSTransport.h"

class QTcpSocket;

/// Owns a TCP socket on the receiver worker. A stop request wakes every wait at once.
/// Serial-to-TCP bridges and their receivers must already run the link at BRIDGE_BAUDRATE.
class TCPGPSTransport : public GPSTransport
{
    friend class TCPGPSTransportTest;

public:
    static constexpr qint64 kWriteBufferBytes = 4 * 1024;
    static constexpr qint64 kReadBufferBytes = 64 * 1024;

    TCPGPSTransport(QString host, quint16 port, std::stop_token stopToken);
    ~TCPGPSTransport() override;

    GPSOpenResult open() override;
    bool fatalError() const override;

    unsigned fixedBaudrate() const override { return BRIDGE_BAUDRATE; }

    GPSReadResult read(uint8_t* buffer, int length, std::chrono::milliseconds timeout) override;
    std::chrono::milliseconds configurationWriteTimeout() const override;
    bool setBaudrate(unsigned baudrate) override;

protected:
    GPSWriteResult writeData(const uint8_t* buffer, int length, QDeadlineTimer deadline) override;

private:
    QString _host;
    quint16 _port;
    std::unique_ptr<QTcpSocket> _socket;

    static constexpr std::chrono::milliseconds kConnectTimeout{5000};
    static constexpr std::chrono::milliseconds kWriteTimeout{5000};
};
