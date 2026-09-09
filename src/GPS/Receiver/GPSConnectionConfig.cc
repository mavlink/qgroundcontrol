#include "GPSConnectionConfig.h"

#include <QtCore/QCoreApplication>

GPSReceiverProfile GPSConnectionConfig::profile() const
{
    GPSReceiverProfile profile;
    switch (transport) {
        case Serial:
            profile.endpoint.kind = GPSReceiverProfile::Endpoint::Kind::Serial;
            profile.endpoint.discoverSerialDevice = device.trimmed().isEmpty();
            break;
        case Tcp:
            profile.endpoint.kind = GPSReceiverProfile::Endpoint::Kind::Tcp;
            break;
        case Udp:
            profile.endpoint.kind = GPSReceiverProfile::Endpoint::Kind::UdpPeer;
            break;
        default:
            profile.endpoint.kind = static_cast<GPSReceiverProfile::Endpoint::Kind>(-1);
            break;
    }
    profile.endpoint.device = device;
    profile.endpoint.host = host;
    profile.endpoint.port = port;
    profile.endpoint.localPort = localPort;
    profile.configurationPolicy = GPSReceiverProfile::ConfigurationPolicy::Configure;
    profile.driverType = receiverType;
    profile.receiverName = receiverName;
    profile.receiver = receiver;
    return profile.normalized();
}

QString GPSConnectionConfig::validationError() const
{
    if (receiver.role == GPSReceiverConfig::Role::RTKBase && (baseMode < 0 || baseMode > 1)) {
        return QCoreApplication::translate("GPSConnectionConfig", "Select a valid base mode");
    }
    return profile().validationError();
}
