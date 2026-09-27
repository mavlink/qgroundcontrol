#include "GPSCorrectionSettings.h"

#include <utility>

#include <QtCore/QSettings>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSCorrectionSettingsLog, "GPS.Settings.GPSCorrectionSettings")

// Preserve persisted keys and custom setting overrides.
DECLARE_SETTINGGROUP(GPSCorrection, "NTRIP")
{
    qCDebug(GPSCorrectionSettingsLog) << this;

    // UDP forwarding moved from NTRIP-only to the selected stream, and its keys with it.
    QSettings settings;
    settings.beginGroup(settingsGroup);
    for (const auto& [legacy, current] : {std::pair{"ntripUdpForwardEnabled", rtcmUdpOutputEnabledName},
                                          std::pair{"ntripUdpTargetAddress", rtcmUdpOutputAddressName},
                                          std::pair{"ntripUdpTargetPort", rtcmUdpOutputPortName}}) {
        const QString legacyKey = QLatin1String(legacy);
        if (settings.contains(legacyKey)) {
            if (!settings.contains(current)) {
                settings.setValue(current, settings.value(legacyKey));
            }
            settings.remove(legacyKey);
        }
    }
    settings.endGroup();
}

GPSCorrectionSettings::~GPSCorrectionSettings()
{
    qCDebug(GPSCorrectionSettingsLog) << this;
}

DECLARE_SETTINGSFACT(GPSCorrectionSettings, rtcmUdpInputEnabled)
DECLARE_SETTINGSFACT(GPSCorrectionSettings, rtcmUdpInputPort)
DECLARE_SETTINGSFACT(GPSCorrectionSettings, rtcmUdpValidate)
DECLARE_SETTINGSFACT(GPSCorrectionSettings, rtcmUdpOutputEnabled)
DECLARE_SETTINGSFACT(GPSCorrectionSettings, rtcmUdpOutputAddress)
DECLARE_SETTINGSFACT(GPSCorrectionSettings, rtcmUdpOutputPort)
DECLARE_SETTINGSFACT(GPSCorrectionSettings, correctionSource)
DECLARE_SETTINGSFACT(GPSCorrectionSettings, correctionSourceInstance)
