
/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MavlinkSettings.h"
#include "LinkManager.h"

#include <QQmlEngine>

DECLARE_SETTINGGROUP(Mavlink, "")
{
    qmlRegisterUncreatableType<MavlinkSettings>("QGroundControl.SettingsManager", 1, 0, "MavlinkSettings", "Reference only");

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
DECLARE_SETTINGSFACT(MavlinkSettings, requireMatchingMavlinkVersions)

DECLARE_SETTINGSFACT_NO_FUNC(MavlinkSettings, mavlink2SigningKey)
{
    if (!_mavlink2SigningKeyFact) {
        _mavlink2SigningKeyFact = _createSettingsFact(mavlink2SigningKeyName);
        connect(_mavlink2SigningKeyFact, &Fact::rawValueChanged, this, &MavlinkSettings::_mavlink2SigningKeyChanged);
    }
    return _mavlink2SigningKeyFact;
}

void MavlinkSettings::_mavlink2SigningKeyChanged(void)
{
    LinkManager::instance()->resetMavlinkSigning();
}
