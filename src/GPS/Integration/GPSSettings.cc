#include "GPSSettings.h"

#include <QtCore/QCoreApplication>

#include "AutoConnectSettings.h"
#include "RTKSettings.h"

GPSSettings::Connection GPSSettings::nmea(AutoConnectSettings& settings)
{
    Connection connection;
    auto& profile = connection.profile;
    using Kind = GPSReceiverProfile::Endpoint::Kind;
    using Policy = GPSReceiverProfile::ConfigurationPolicy;
    switch (settings.nmeaSource()->rawValue().toInt()) {
        case AutoConnectSettings::NmeaSourceDisabled:
            break;
        case AutoConnectSettings::NmeaSourceUdp:
            profile.endpoint.kind = Kind::UdpListener;
            profile.endpoint.port = settings.nmeaUdpPort()->rawValue().toInt();
            break;
        case AutoConnectSettings::NmeaSourceTcp:
            profile.endpoint.kind = Kind::Tcp;
            profile.endpoint.host = settings.nmeaTcpHost()->rawValue().toString();
            profile.endpoint.port = settings.nmeaTcpPort()->rawValue().toInt();
            break;
        case AutoConnectSettings::NmeaSourceSerial:
            profile.endpoint.kind = Kind::Serial;
            profile.endpoint.device = settings.autoConnectNmeaPort()->rawValue().toString();
            switch (settings.nmeaReceiverMode()->rawValue().toInt()) {
                case AutoConnectSettings::NmeaReceiverPassive:
                    profile.endpoint.baud = settings.autoConnectNmeaBaud()->rawValue().toInt();
                    break;
                case AutoConnectSettings::NmeaReceiverUblox:
                    profile.configurationPolicy = Policy::Configure;
                    break;
                default:
                    profile.configurationPolicy = static_cast<Policy>(-1);
                    break;
            }
            break;
        default:
            profile.endpoint.kind = static_cast<Kind>(-1);
            break;
    }
    profile = profile.normalized();
    connection.automatic = settings.nmeaAutoConnect()->rawValue().toBool();
    connection.validationError = profile.validationError();
    return connection;
}

GPSSettings::Connection GPSSettings::receiver(RTKSettings& settings, AutoConnectSettings& autoConnect)
{
    Connection connection;
    auto& profile = connection.profile;
    using Kind = GPSReceiverProfile::Endpoint::Kind;
    switch (settings.connectionType()->rawValue().toInt()) {
        case RTKSettings::Serial:
            profile.endpoint.kind = Kind::Serial;
            profile.endpoint.discoverSerialDevice = true;
            break;
        case RTKSettings::Tcp:
            profile.endpoint.kind = Kind::Tcp;
            break;
        case RTKSettings::Udp:
            profile.endpoint.kind = Kind::UdpPeer;
            break;
        default:
            profile.endpoint.kind = static_cast<Kind>(-1);
            break;
    }
    profile.configurationPolicy = GPSReceiverProfile::ConfigurationPolicy::Configure;
    profile.endpoint.device = settings.serialDevice()->rawValue().toString();
    profile.endpoint.host = settings.networkBaseHost()->rawValue().toString();
    profile.endpoint.port = settings.networkBasePort()->rawValue().toInt();
    profile.endpoint.localPort = settings.udpLocalPort()->rawValue().toInt();
    profile.driverType = static_cast<GPSType>(settings.networkReceiverType()->rawValue().toInt());
    profile.receiverName = settings.networkReceiverType()->enumStringValue();
    auto& receiver = profile.receiver;
    receiver.outputProtocol = GPSReceiverConfig::OutputProtocol::Native;
    receiver.role = static_cast<GPSReceiverConfig::Role>(settings.receiverRole()->rawValue().toInt());
    receiver.constellationMask = settings.constellationMask()->rawValue().toInt();
    receiver.dynamicModel = settings.dynamicModel()->rawValue().toInt();
    receiver.outputRateHz = settings.outputRateHz()->rawValue().toInt();
    receiver.headingOffsetDeg = settings.headingOffsetDeg()->rawValue().toFloat();
    const int baseMode = settings.useFixedBasePosition()->rawValue().toInt();
    receiver.base = {
        .useFixedBase = baseMode == static_cast<int>(BaseModeDefinition::Mode::BaseFixed),
        .surveyInAccMeters = settings.surveyInAccuracyLimit()->rawValue().toDouble(),
        .surveyInDurationSecs = settings.surveyInMinObservationDuration()->rawValue().toInt(),
        .fixedBaseLatitude = settings.fixedBasePositionLatitude()->rawValue().toDouble(),
        .fixedBaseLongitude = settings.fixedBasePositionLongitude()->rawValue().toDouble(),
        .fixedBaseAltitudeMeters = settings.fixedBasePositionAltitude()->rawValue().toFloat(),
        .fixedBaseAccuracyMeters = settings.fixedBasePositionAccuracy()->rawValue().toFloat(),
    };
    if (receiver.role == GPSReceiverConfig::Role::RTKBase &&
        baseMode != static_cast<int>(BaseModeDefinition::Mode::BaseSurveyIn) &&
        baseMode != static_cast<int>(BaseModeDefinition::Mode::BaseFixed)) {
        // A bool cannot represent an invalid saved mode; keep this profile inadmissible.
        receiver.role = static_cast<GPSReceiverConfig::Role>(-1);
        connection.validationError = QCoreApplication::translate("GPSSettings", "Select a valid base mode");
    }
    profile = profile.normalized();
    connection.automatic = profile.endpoint.kind == Kind::Serial
                               ? autoConnect.autoConnectRTKGPS()->rawValue().toBool()
                               : autoConnect.autoConnectNetworkRTKGPS()->rawValue().toBool();
    if (connection.validationError.isEmpty()) {
        connection.validationError = profile.validationError();
    }
    return connection;
}
