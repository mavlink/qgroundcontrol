#include "GPSCorrectionSettings.h"

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSCorrectionSettingsLog, "GPS.Settings.GPSCorrectionSettings")

// Preserve persisted keys and custom setting overrides.
DECLARE_SETTINGGROUP(GPSCorrection, "NTRIP")
{
    qCDebug(GPSCorrectionSettingsLog) << this;
}

GPSCorrectionSettings::~GPSCorrectionSettings()
{
    qCDebug(GPSCorrectionSettingsLog) << this;
}

DECLARE_SETTINGSFACT(GPSCorrectionSettings, rtcmUdpInputEnabled)
DECLARE_SETTINGSFACT(GPSCorrectionSettings, rtcmUdpInputPort)
DECLARE_SETTINGSFACT(GPSCorrectionSettings, rtcmUdpValidate)
DECLARE_SETTINGSFACT(GPSCorrectionSettings, correctionSource)
DECLARE_SETTINGSFACT(GPSCorrectionSettings, correctionSourceInstance)
