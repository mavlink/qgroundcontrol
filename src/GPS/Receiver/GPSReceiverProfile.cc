#include "GPSReceiverProfile.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QUrl>

#include "GPSReceiverCapabilities.h"

QString GPSReceiverProfile::networkHost() const
{
    QUrl address;
    address.setScheme(endpoint.kind == Endpoint::Kind::Tcp ? QStringLiteral("tcp") : QStringLiteral("udp"));
    address.setHost(endpoint.host.trimmed());
    return address.isValid() ? address.host() : QString();
}

GPSReceiverProfile GPSReceiverProfile::normalized() const
{
    GPSReceiverProfile profile = *this;
    profile.endpoint.device = endpoint.device.trimmed();
    profile.endpoint.host = endpoint.host.trimmed();
    switch (endpoint.kind) {
        case Endpoint::Kind::Disabled:
            return {};
        case Endpoint::Kind::Serial:
            profile.endpoint.discoverSerialDevice = profile.endpoint.device.isEmpty() && endpoint.discoverSerialDevice;
            profile.endpoint.host.clear();
            profile.endpoint.port = 0;
            profile.endpoint.localPort = 0;
            if (configurationPolicy == ConfigurationPolicy::Configure) {
                profile.endpoint.baud = 0;
            }
            break;
        case Endpoint::Kind::Tcp:
        case Endpoint::Kind::UdpPeer:
            profile.endpoint.device.clear();
            profile.endpoint.baud = 0;
            profile.endpoint.discoverSerialDevice = false;
            if (const QString host = networkHost(); !host.isEmpty()) {
                profile.endpoint.host = host;
            }
            if (endpoint.kind == Endpoint::Kind::Tcp) {
                profile.endpoint.localPort = 0;
            }
            break;
        case Endpoint::Kind::UdpListener:
            profile.endpoint.device.clear();
            profile.endpoint.host.clear();
            profile.endpoint.localPort = 0;
            profile.endpoint.baud = 0;
            profile.endpoint.discoverSerialDevice = false;
            break;
    }
    if (configurationPolicy == ConfigurationPolicy::Passive) {
        profile.driverType = GPSType::u_blox;
        profile.receiverName.clear();
        // Passive sources never use receiver-specific configuration.
        profile.receiver.base = {};
        profile.receiver.headingOffsetDeg = GPSReceiverConfig().headingOffsetDeg;
        profile.receiver.constellationMask = 0;
        profile.receiver.dynamicModel = 0;
        profile.receiver.outputRateHz = 0;
    } else if (receiver.role == GPSReceiverConfig::Role::Position) {
        profile.receiver.base = {};
    } else if (receiver.base.useFixedBase) {
        profile.receiver.base.surveyInAccMeters = 0;
        profile.receiver.base.surveyInDurationSecs = 0;
    } else {
        profile.receiver.base.fixedBaseLatitude = 0;
        profile.receiver.base.fixedBaseLongitude = 0;
        profile.receiver.base.fixedBaseAltitudeMeters = 0;
        profile.receiver.base.fixedBaseAccuracyMeters = 0;
    }
    return profile;
}

bool GPSReceiverProfile::operator==(const GPSReceiverProfile& other) const
{
    const auto left = normalized();
    const auto right = other.normalized();
    return left.endpoint == right.endpoint && left.configurationPolicy == right.configurationPolicy &&
           left.driverType == right.driverType && left.receiver == right.receiver &&
           left.receiverName == right.receiverName;
}

QString GPSReceiverProfile::validationError() const
{
    const auto tr = [](const char* text) { return QCoreApplication::translate("GPSReceiverProfile", text); };
    if (endpoint.kind == Endpoint::Kind::Disabled) {
        return {};
    }
    if (configurationPolicy != ConfigurationPolicy::Passive && configurationPolicy != ConfigurationPolicy::Configure) {
        return tr("Select a valid receiver configuration");
    }
    switch (endpoint.kind) {
        case Endpoint::Kind::Serial:
            if (endpoint.device.trimmed().isEmpty() && !endpoint.discoverSerialDevice) {
                return tr("Select a serial device");
            }
            if (configurationPolicy == ConfigurationPolicy::Passive && endpoint.baud <= 0) {
                return tr("Select a valid baud rate");
            }
            break;
        case Endpoint::Kind::Tcp:
        case Endpoint::Kind::UdpPeer:
            if (networkHost().isEmpty() || endpoint.port < 1 || endpoint.port > 65535) {
                return endpoint.kind == Endpoint::Kind::Tcp ? tr("Enter a valid TCP host and port")
                                                            : tr("Enter a valid receiver host and port");
            }
            if (endpoint.kind == Endpoint::Kind::UdpPeer && configurationPolicy == ConfigurationPolicy::Passive) {
                return tr("Passive UDP input requires a listener endpoint");
            }
            if (endpoint.kind == Endpoint::Kind::UdpPeer && (endpoint.localPort < 0 || endpoint.localPort > 65535)) {
                return tr("Enter a valid UDP port");
            }
            break;
        case Endpoint::Kind::UdpListener:
            if (endpoint.port < 0 || endpoint.port > 65535) {
                return tr("Enter a valid UDP port");
            }
            if (configurationPolicy != ConfigurationPolicy::Passive) {
                return tr("A UDP listener cannot configure a receiver");
            }
            break;
        case Endpoint::Kind::Disabled:
            break;
        default:
            return tr("Select a valid receiver and connection type");
    }
    if (configurationPolicy == ConfigurationPolicy::Passive) {
        return receiver.role == GPSReceiverConfig::Role::Position &&
                       receiver.outputProtocol == GPSReceiverConfig::OutputProtocol::NMEA
                   ? QString()
                   : tr("Passive input requires NMEA position output");
    }
    const auto config = normalized().receiver;
    if (const QString error = GPSReceiverCapabilities::forType(driverType).validationError(config); !error.isEmpty()) {
        return error;
    }
    return config.validationError();
}
