#pragma once

#include <cstdint>
#include <functional>
#include <optional>

#include <QtCore/QString>

#include "GPSReceiverConfig.h"
#include "GPSReceiverDescriptor.h"
#include "GPSReceiverSerialPorts.h"
#include "GPSReceiverWorker.h"
#include "RTKSettings.h"

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

/// Whether a receiver configuration can connect, whatever devices are plugged in or reachable, and the request its
/// connection sends. GPSReceiverConnectionPolicy::connectability() builds it.
struct GPSReceiverConnectability
{
    /// The connection the configuration uses: without serial links, a serial selection connects over TCP.
    RTKSettings::ConnectionType connection = RTKSettings::Serial;
    /// The receiver family to connect; empty when no receiver type is selected.
    std::optional<GPSType> type;
    /// The family supports the selected base mode; a passive receiver has none to support.
    bool baseModeSupported = false;
    /// UDP only receives, so a receiver QGroundControl configures needs serial or TCP.
    bool connectionSupported = false;
    /// The serial device, TCP address or UDP port to connect is entered, so a receiver is saved.
    bool hasEndpoint = false;
    /// Why the configuration cannot connect, as shown to the user; empty when it can. Never empty without a type, a
    /// supported base mode, a supported connection and an endpoint.
    QString error;
    /// What a connection asks of the receiver, which @a error has validated; built once the type, the base mode, the
    /// connection and the endpoint are known.
    GPSReceiverConfig request;
};

/// Creates a receiver session's worker; tests inject scripted workers.
using GPSReceiverWorkerFactory =
    std::function<GPSReceiverWorker*(GPSReceiverWorker::TransportFactory, GPSType, const GPSReceiverConfig&, QObject*)>;

/// The receiver operations GPSReceiverConnectionPolicy drives. They change state without emitting; the receiver emits
/// once its public method or report handler returns. A connection sends the request of the policy's
/// GPSReceiverConnectability as it is given.
class GPSReceiverConnector
{
public:
    /// What ending the connection does with the connection error on display.
    enum class ShownError
    {
        Keep,
        Clear,
    };

    virtual bool hasReceiver() const = 0;
    virtual void setConnectionError(const QString& message) = 0;
    virtual void endConnection(ShownError error) = 0;
    virtual bool connectTcp(const QString& host, quint16 port, GPSType type, GPSReceiverConfig request) = 0;
    virtual bool connectUdp(quint16 port, GPSType type, GPSReceiverConfig request) = 0;
    /// @a claim is the policy's claim of @a device from the receiver's GPSReceiverSerialPorts.
    virtual bool connectSerial(const QString& device, GPSType type, GPSReceiverConfig request,
                               GPSSerialClaim claim) = 0;

protected:
    ~GPSReceiverConnector() = default;
};
