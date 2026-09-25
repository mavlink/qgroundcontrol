#pragma once

#include <cstdint>

#include <QtCore/QString>
#include <QtCore/QStringView>

#include "GPSProvider.h"

class GPSNotificationQueue;
class SerialPortManager;

/// The receiver operations RTKConnectionPolicy drives. The implementation runs the receiver sessions; the policy
/// only decides when to connect. Connections return false when they fail or are superseded.
class RTKConnectionTarget
{
public:
    virtual bool hasReceiver() const = 0;
    virtual GPSConnectionError connectionError() const = 0;
    virtual void setConnectionError(GPSConnectionError error, const QString& message) = 0;
    virtual void disconnectReceiver(bool clearError) = 0;
    virtual bool connectTcp(const QString& host, quint16 port, GPSType type, bool allowPersistentChanges) = 0;
    virtual bool connectUdp(quint16 port, GPSType type) = 0;
#ifndef QGC_NO_SERIAL_LINK
    virtual SerialPortManager* serialPorts() const = 0;
    virtual bool connectSerial(const QString& device, GPSType type, uint32_t baudRate, bool allowPersistentChanges) = 0;
    /// Connects a discovered port, identifying the receiver family from its board name.
    virtual bool connectDiscovered(const QString& device, QStringView boardName) = 0;
#endif
    /// Receiver notifications are published once the outermost operation using this queue finishes.
    virtual GPSNotificationQueue& notifications() = 0;

protected:
    ~RTKConnectionTarget() = default;
};
