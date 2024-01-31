/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MavlinkLogSettings.h"

#include <QQmlEngine>
#include <QtQml>


DECLARE_SETTINGGROUP(MavlinkLog, "MavlinkLog")
{
    qmlRegisterUncreatableType<MavlinkLogSettings>("QGroundControl.SettingsManager", 1, 0, "MavlinkLogSettings", "Reference only");

    // Moved deprecated one-off settings to new SettingsGroup

    static const char* kMAVLinkLogGroup         = "MAVLinkLogGroup";
    static const char* kEmailAddressKey         = "Email";
    static const char* kDescriptionsKey         = "Description";
    static const char* kPx4URLKey               = "LogURL";
    static const char* kEnableAutoUploadKey     = "EnableAutoUpload";
    static const char* kEnableAutoStartKey      = "EnableAutoStart";
    static const char* kEnableDeletetKey        = "EnableDelete";
    static const char* kVideoURLKey             = "VideoURL";
    static const char* kWindSpeedKey            = "WindSpeed";
    static const char* kRateKey                 = "RateKey";
    static const char* kPublicLogKey            = "PublicLog";

    QSettings oldSettings;
    if (oldSettings.contains(kMAVLinkLogGroup)) {
        oldSettings.beginGroup(kMAVLinkLogGroup);

        if (oldSettings.contains(kEmailAddressKey)) {
            QString oldValue = oldSettings.value(kEmailAddressKey).toString();
            qobject_cast<SettingsFact*>(emailAddress())->setRawValue(oldValue);
        }
        if (oldSettings.contains(kDescriptionsKey)) {
            QString oldValue = oldSettings.value(kDescriptionsKey).toString();
            qobject_cast<SettingsFact*>(description())->setRawValue(oldValue);
        }
        if (oldSettings.contains(kPx4URLKey)) {
            QString oldValue = oldSettings.value(kPx4URLKey).toString();
            qobject_cast<SettingsFact*>(uploadURL())->setRawValue(oldValue);
        }
        if (oldSettings.contains(kVideoURLKey)) {
            QString oldValue = oldSettings.value(kVideoURLKey, QString()).toString();
            qobject_cast<SettingsFact*>(videoURL())->setRawValue(oldValue);
        }
        if (oldSettings.contains(kEnableAutoUploadKey)) {
            bool oldValue = oldSettings.value(kEnableAutoUploadKey, true).toBool();
            qobject_cast<SettingsFact*>(autoUploadEnabled())->setRawValue(oldValue);
        }
        if (oldSettings.contains(kEnableAutoStartKey)) {
            bool oldValue = oldSettings.value(kEnableAutoStartKey, false).toBool();
            qobject_cast<SettingsFact*>(autoStartEnabled())->setRawValue(oldValue);
        }
        if (oldSettings.contains(kEnableDeletetKey)) {
            bool oldValue = oldSettings.value(kEnableDeletetKey, false).toBool();
            qobject_cast<SettingsFact*>(deleteAferUpload())->setRawValue(oldValue);
        }
        if (oldSettings.contains(kWindSpeedKey)) {
            int oldValue = oldSettings.value(kWindSpeedKey, -1).toInt();
            qobject_cast<SettingsFact*>(windSpeed())->setRawValue(oldValue);
        }
        if (oldSettings.contains(kRateKey)) {
            QString oldValue = oldSettings.value(kRateKey).toString();
            qobject_cast<SettingsFact*>(rating())->setRawValue(oldValue);
        }
        if (oldSettings.contains(kPublicLogKey)) {
            bool oldValue = oldSettings.value(kPublicLogKey, true).toBool();
            qobject_cast<SettingsFact*>(isPulicLog())->setRawValue(oldValue);
        }
    }
}

DECLARE_SETTINGSFACT(MavlinkLogSettings, emailAddress)
DECLARE_SETTINGSFACT(MavlinkLogSettings, description)
DECLARE_SETTINGSFACT(MavlinkLogSettings, uploadURL)
DECLARE_SETTINGSFACT(MavlinkLogSettings, videoURL)
DECLARE_SETTINGSFACT(MavlinkLogSettings, autoUploadEnabled)
DECLARE_SETTINGSFACT(MavlinkLogSettings, autoStartEnabled)
DECLARE_SETTINGSFACT(MavlinkLogSettings, deleteAferUpload)
DECLARE_SETTINGSFACT(MavlinkLogSettings, windSpeed)
DECLARE_SETTINGSFACT(MavlinkLogSettings, rating)
DECLARE_SETTINGSFACT(MavlinkLogSettings, isPulicLog)
