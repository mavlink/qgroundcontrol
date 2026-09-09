#include "RTKConnectionConfig.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QUrl>

#include <cmath>
#include <limits>

#include "RTKSettings.h"

RTKConnectionConfig RTKConnectionConfig::fromSettings(RTKSettings& settings)
{
    RTKConnectionConfig config;
    config.transport = static_cast<Transport>(settings.connectionType()->rawValue().toInt());
    config.device = settings.serialDevice()->rawValue().toString().trimmed();
    config.receiverType = static_cast<GPSType>(settings.networkReceiverType()->rawValue().toInt());
    config.receiverName = settings.networkReceiverType()->enumStringValue();
    config.host = settings.networkBaseHost()->rawValue().toString().trimmed();
    config.port = settings.networkBasePort()->rawValue().toInt();
    config.localPort = settings.udpLocalPort()->rawValue().toInt();
    config.receiver.role = static_cast<GPSReceiverConfig::Role>(settings.receiverRole()->rawValue().toInt());
    config.baseMode = settings.useFixedBasePosition()->rawValue().toInt();
    config.receiver.base = {
        .useFixedBase = config.baseMode == static_cast<int>(BaseModeDefinition::Mode::BaseFixed),
        .surveyInAccMeters = settings.surveyInAccuracyLimit()->rawValue().toDouble(),
        .surveyInDurationSecs = settings.surveyInMinObservationDuration()->rawValue().toInt(),
        .fixedBaseLatitude = settings.fixedBasePositionLatitude()->rawValue().toDouble(),
        .fixedBaseLongitude = settings.fixedBasePositionLongitude()->rawValue().toDouble(),
        .fixedBaseAltitudeMeters = settings.fixedBasePositionAltitude()->rawValue().toFloat(),
        .fixedBaseAccuracyMeters = settings.fixedBasePositionAccuracy()->rawValue().toFloat(),
    };
    return config;
}

QString RTKConnectionConfig::validationError() const
{
    const auto tr = [](const char* text) { return QCoreApplication::translate("RTKConnectionConfig", text); };
    if (transport < Serial || transport > Udp || receiverType < GPSType::u_blox || receiverType > GPSType::femto) {
        return tr("Select a valid receiver and connection type");
    }
    if (transport != Serial) {
        QUrl endpoint;
        endpoint.setScheme(transport == Udp ? QStringLiteral("udp") : QStringLiteral("tcp"));
        endpoint.setHost(host);
        if (host.isEmpty() || !endpoint.isValid() || endpoint.host().isEmpty() || port < 1 || port > 65535 ||
            (transport == Udp && (localPort < 0 || localPort > 65535))) {
            return tr("Enter a valid receiver host and port");
        }
    }
    if (receiver.role != GPSReceiverConfig::Role::RTKBase && receiver.role != GPSReceiverConfig::Role::Position) {
        return tr("Select a valid receiver role");
    }
    if (receiver.role == GPSReceiverConfig::Role::Position) {
        return {};
    }
    if (baseMode < 0 || baseMode > 1) {
        return tr("Select a valid base mode");
    }
    if (receiver.base.useFixedBase) {
        if (!std::isfinite(receiver.base.fixedBaseLatitude) || std::abs(receiver.base.fixedBaseLatitude) > 90.0 ||
            !std::isfinite(receiver.base.fixedBaseLongitude) || std::abs(receiver.base.fixedBaseLongitude) > 180.0 ||
            !std::isfinite(receiver.base.fixedBaseAltitudeMeters) ||
            !std::isfinite(receiver.base.fixedBaseAccuracyMeters) || receiver.base.fixedBaseAccuracyMeters < 0.0f) {
            return tr("Enter a valid fixed base position and accuracy");
        }
    } else if (!std::isfinite(receiver.base.surveyInAccMeters) || receiver.base.surveyInAccMeters <= 0.0 ||
               receiver.base.surveyInAccMeters * 10000.0 > std::numeric_limits<uint32_t>::max() ||
               receiver.base.surveyInDurationSecs <= 0) {
        return tr("Enter a valid survey-in accuracy and duration");
    }
    return {};
}
