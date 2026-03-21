#include "MavlinkSettings.h"

DECLARE_SETTINGGROUP(Mavlink, "")
{
    // Move deprecated settings to new location/names

    QSettings deprecatedSettings;
    QSettings newSettings;

    newSettings.beginGroup(_settingsGroup);

    static const char* deprecatedGCSHeartbeatEnabledKey = "gcsHeartbeatEnabled";
    if (!newSettings.contains(sendGCSHeartbeatName) && deprecatedSettings.contains(deprecatedGCSHeartbeatEnabledKey)) {
        newSettings.setValue(sendGCSHeartbeatName, deprecatedSettings.value(deprecatedGCSHeartbeatEnabledKey));
        deprecatedSettings.remove(deprecatedGCSHeartbeatEnabledKey);
    }

    static const char* deprecatedMavlinkGroup = "QGC_MAVLINK_PROTOCOL";
    static const char* deprecatedMavlinkSystemIdKey = "GCS_SYSTEM_ID";
    deprecatedSettings.beginGroup(deprecatedMavlinkGroup);
    if (!newSettings.contains(gcsMavlinkSystemIDName) && deprecatedSettings.contains(deprecatedMavlinkSystemIdKey)) {
        newSettings.setValue(gcsMavlinkSystemIDName, deprecatedSettings.value(deprecatedMavlinkSystemIdKey));
        deprecatedSettings.remove(deprecatedMavlinkSystemIdKey);
    }

    newSettings.endGroup();
    deprecatedSettings.endGroup();
}

DECLARE_SETTINGSFACT(MavlinkSettings, telemetrySave)
DECLARE_SETTINGSFACT(MavlinkSettings, telemetrySaveNotArmed)
DECLARE_SETTINGSFACT(MavlinkSettings, apmStartMavlinkStreams)
DECLARE_SETTINGSFACT(MavlinkSettings, saveCsvTelemetry)
DECLARE_SETTINGSFACT(MavlinkSettings, forwardMavlink)
DECLARE_SETTINGSFACT(MavlinkSettings, forwardMavlinkHostName)
DECLARE_SETTINGSFACT(MavlinkSettings, forwardMavlinkAPMSupportHostName)
DECLARE_SETTINGSFACT(MavlinkSettings, sendGCSHeartbeat)
DECLARE_SETTINGSFACT(MavlinkSettings, gcsMavlinkSystemID)
DECLARE_SETTINGSFACT(MavlinkSettings, noInitialDownloadWhenFlying)
