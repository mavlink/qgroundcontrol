#pragma once

#include <cstdint>

#include <QtCore/QString>

#include "GPSProvider.h"

class NotificationQueue;
class GPSSerialPorts;

/// The receiver operations GPSReceiverConnectionPolicy drives. The implementation runs the receiver sessions; the
/// policy only decides when to connect. Connections return false when they fail or are superseded.
class GPSReceiverConnectionTarget
{
public:
    virtual bool hasReceiver() const = 0;
    virtual GPSConnectionError connectionError() const = 0;
    virtual void setConnectionError(GPSConnectionError error, const QString& message) = 0;
    virtual void disconnectReceiver(bool clearError) = 0;
    virtual bool connectTcp(const QString& host, quint16 port, GPSType type, bool allowPersistentChanges) = 0;
    virtual bool connectUdp(quint16 port, GPSType type) = 0;
#ifndef QGC_NO_SERIAL_LINK
    virtual GPSSerialPorts* serialPorts() const = 0;
    virtual bool connectSerial(const QString& device, GPSType type, uint32_t baudRate, bool allowPersistentChanges) = 0;
#endif
    /// Receiver notifications are published once the outermost operation using this queue finishes.
    virtual NotificationQueue& notifications() = 0;

protected:
    ~GPSReceiverConnectionTarget() = default;
};
