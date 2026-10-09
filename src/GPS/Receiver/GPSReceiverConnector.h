#pragma once

#include <cstdint>
#include <functional>

#include <QtCore/QString>
#include <QtCore/QStringList>

#include "GPSReceiverConfig.h"
#include "GPSReceiverDescriptor.h"
#include "GPSReceiverWorker.h"
#include "RTKSettings.h"
#ifndef QGC_NO_SERIAL_LINK
#include "SerialPortManager.h"
#endif

/// The receiver configuration the application's settings binding supplies as values.
struct GPSReceiverConfiguration
{
    /// Only a configured base is written to by QGroundControl.
    RTKSettings::ReceiverRole receiverRole = RTKSettings::ConfiguredBase;
    /// A passive receiver's RTCM output becomes a correction source for vehicles.
    bool forwardReceiverRtcm = true;
    /// A GPSReceiverDescriptor manufacturer, or GPS_AUTOMATIC_MANUFACTURER to detect the family on every connect.
    int baseReceiverManufacturer = GPS_AUTOMATIC_MANUFACTURER;
    RTKSettings::ConnectionType connectionType = RTKSettings::Serial;
    QString tcpHost;
    uint32_t tcpPort = 0;
    uint32_t udpPort = 0;
    QString serialDevice;
    uint32_t serialBaudRate = 0;
    GPSBaseStationConfig base;
    /// Keep the saved receiver connected, or discover a configured base on USB when no device is saved.
    bool autoConnect = true;
    bool operator==(const GPSReceiverConfiguration&) const = default;

    /// The connection this configuration uses; builds without serial links connect a serial selection over TCP.
    [[nodiscard]] RTKSettings::ConnectionType effectiveConnectionType() const
    {
#ifdef QGC_NO_SERIAL_LINK
        return connectionType == RTKSettings::Udp ? RTKSettings::Udp : RTKSettings::Tcp;
#else
        return connectionType == RTKSettings::Tcp || connectionType == RTKSettings::Udp ? connectionType
                                                                                        : RTKSettings::Serial;
#endif
    }

    /// Selects the same receiver, connection, and base mode, which a pending retry and the flash-save
    /// permission were made for.
    [[nodiscard]] bool sameConnection(const GPSReceiverConfiguration& other) const
    {
        return receiverRole == other.receiverRole && baseReceiverManufacturer == other.baseReceiverManufacturer &&
               base.mode.index() == other.base.mode.index() && connectionType == other.connectionType &&
               serialDevice == other.serialDevice && serialBaudRate == other.serialBaudRate &&
               tcpHost == other.tcpHost && tcpPort == other.tcpPort && udpPort == other.udpPort;
    }
};

/// Creates a receiver session's worker; tests inject scripted workers.
using GPSReceiverWorkerFactory =
    std::function<GPSReceiverWorker*(GPSReceiverWorker::TransportFactory, GPSType, const GPSReceiverConfig&, QObject*)>;

#ifndef QGC_NO_SERIAL_LINK
/// Holds a serial device for one receiver session until the last copy is released.
using GPSSerialClaim = SerialPortManager::ReservationPtr;
#endif

/// The receiver operations GPSReceiverConnectionPolicy drives. They change state without emitting; the receiver emits
/// once its public method or report handler returns.
class GPSReceiverConnector
{
public:
    virtual bool hasReceiver() const = 0;
    virtual void setConnectionError(const QString& message) = 0;
    virtual void endConnection(bool clearError) = 0;
    virtual bool connectTcp(const QString& host, quint16 port, GPSType type, bool allowPersistentChanges) = 0;
    virtual bool connectUdp(quint16 port, GPSType type) = 0;
#ifndef QGC_NO_SERIAL_LINK
    virtual bool connectSerial(const QString& device, GPSType type, uint32_t baudRate, bool allowPersistentChanges,
                               GPSSerialClaim claim) = 0;
    /// The device is plugged in and not in its bootloader.
    virtual bool serialPortAvailable(const QString& device) const = 0;
    /// Free devices whose USB identity is a known RTK receiver adapter, one per physical device, which discovery may
    /// connect with the saved manufacturer.
    virtual QStringList rtkReceiverPorts() const = 0;
    /// Claims the device until the last copy is released; empty when it is in use.
    virtual GPSSerialClaim claimSerialPort(const QString& device) = 0;
#endif

protected:
    ~GPSReceiverConnector() = default;
};
