#include "NMEAConnectionConfig.h"

#include "AutoConnectSettings.h"

NMEAConnectionConfig NMEAConnectionConfig::fromSettings(AutoConnectSettings& settings)
{
    NMEAConnectionConfig config;
    config.source = static_cast<Source>(settings.nmeaSource()->rawValue().toInt());
    switch (config.source) {
        case Udp:
            config.port = settings.nmeaUdpPort()->rawValue().toInt();
            break;
        case Tcp:
            config.host = settings.nmeaTcpHost()->rawValue().toString().trimmed();
            config.port = settings.nmeaTcpPort()->rawValue().toInt();
            break;
        case Serial:
            config.device = settings.autoConnectNmeaPort()->rawValue().toString().trimmed();
            config.receiverMode = static_cast<ReceiverMode>(settings.nmeaReceiverMode()->rawValue().toInt());
            if (config.receiverMode == Passive) {
                config.baud = settings.autoConnectNmeaBaud()->rawValue().toInt();
            }
            break;
        case Disabled:
            break;
    }
    return config;
}

GPSReceiverProfile NMEAConnectionConfig::profile() const
{
    GPSReceiverProfile profile;
    switch (source) {
        case Disabled:
            break;
        case Udp:
            profile.endpoint.kind = GPSReceiverProfile::Endpoint::Kind::UdpListener;
            break;
        case Tcp:
            profile.endpoint.kind = GPSReceiverProfile::Endpoint::Kind::Tcp;
            break;
        case Serial:
            profile.endpoint.kind = GPSReceiverProfile::Endpoint::Kind::Serial;
            profile.configurationPolicy = receiverMode == Passive ? GPSReceiverProfile::ConfigurationPolicy::Passive
                                                                  : GPSReceiverProfile::ConfigurationPolicy::Configure;
            if (receiverMode != Passive && receiverMode != Ublox) {
                profile.configurationPolicy = static_cast<GPSReceiverProfile::ConfigurationPolicy>(-1);
            }
            break;
        default:
            profile.endpoint.kind = static_cast<GPSReceiverProfile::Endpoint::Kind>(-1);
            break;
    }
    profile.endpoint.device = device;
    profile.endpoint.host = host;
    profile.endpoint.port = port;
    profile.endpoint.baud = baud;
    return profile.normalized();
}

bool NMEAConnectionConfig::operator==(const NMEAConnectionConfig& other) const
{
    return profile() == other.profile();
}

QString NMEAConnectionConfig::validationError() const
{
    return profile().validationError();
}
