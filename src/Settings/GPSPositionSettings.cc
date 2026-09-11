#include "GPSPositionSettings.h"

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSPositionSettingsLog, "GPS.Settings.GPSPositionSettings")

DECLARE_SETTINGGROUP(GPSPosition, "GPSPosition")
{
    qCDebug(GPSPositionSettingsLog) << this;
}

GPSPositionSettings::~GPSPositionSettings()
{
    qCDebug(GPSPositionSettingsLog) << this;
}

DECLARE_SETTINGSFACT(GPSPositionSettings, sourceMode)
