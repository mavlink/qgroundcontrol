#include "GPSConnectionSettings.h"

#include "RTKSettings.h"

GPSConnectionConfig GPSConnectionSettings::fromSettings(RTKSettings& settings)
{
    GPSConnectionConfig config;
    config.transport = static_cast<GPSConnectionConfig::Transport>(settings.connectionType()->rawValue().toInt());
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
