#pragma once

#include <chrono>
#include <memory>

#include <QtCore/QDeadlineTimer>
#include <QtCore/QString>

#include "GPSDeviceTransport.h"

class QTcpSocket;

/// Owns a TCP socket on the receiver worker. A stop request wakes every wait at once.
/// Serial-to-TCP bridges and their receivers must already run the link at BRIDGE_BAUDRATE.
class TCPGPSTransport : public GPSDeviceTransport
{
public:
    TCPGPSTransport(QString host, quint16 port, GPSCancelToken cancelToken);
    ~TCPGPSTransport() override;

    GPSOpenResult open() override;
    bool fatalError() const override;

    unsigned fixedBaudrate() const override { return BRIDGE_BAUDRATE; }

    std::chrono::milliseconds configurationWriteTimeout() const override;

protected:
    QIODevice* device() const override;
    void retire() override;

private:
    QString _host;
    quint16 _port;
    std::unique_ptr<QTcpSocket> _socket;

    static constexpr std::chrono::milliseconds CONNECT_TIMEOUT{5000};
    static constexpr std::chrono::milliseconds WRITE_TIMEOUT{5000};
};
