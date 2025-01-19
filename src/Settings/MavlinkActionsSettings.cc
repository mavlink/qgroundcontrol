/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MavlinkActionsSettings.h"
#include "QGCApplication.h"

#include <QtQml/QQmlEngine>
#include <QtCore/QFile>
#include <QtCore/QSettings>

DECLARE_SETTINGGROUP(MavlinkActions, "MavlinkActions")
{
    qmlRegisterUncreatableType<MavlinkActionsSettings>("QGroundControl.SettingsManager", 1, 0, "MavlinkActionsSettings", "Reference only");

    // Notify the user of new Fly View mavlink actions support
    QSettings deprecatedSettings;
    static constexpr const char* deprecatedKey1 = "enableCustomActions";
    static constexpr const char* deprecatedKey2 = "customActionsDefinitions";
    deprecatedSettings.beginGroup("FlyView");
    if (deprecatedSettings.contains(deprecatedKey1) || deprecatedSettings.contains(deprecatedKey2)) {
        deprecatedSettings.remove(deprecatedKey1);
        deprecatedSettings.remove(deprecatedKey2);
        qgcApp()->showAppMessage(MavlinkActionsSettings::tr("Support for Fly View custom actions has changed. The location of the files has changed. You will need to setup up your settings again from Fly View Settings."));
    }

    // Notify the user of new Joystick custom actions support
    static constexpr const char* joystickFileName = "JoystickMavCommands.json";
    if (QFile(joystickFileName).exists()) {
        qgcApp()->showAppMessage(MavlinkActionsSettings::tr("Support for Joystick custom actions has changed. The format and location of the files has changed. New setting is available from Fly View Settings. File format is documented in user guide. Delete the %1 file to disable this warning").arg(joystickFileName));
    }
}

DECLARE_SETTINGSFACT(MavlinkActionsSettings, flyViewActionsFile)
DECLARE_SETTINGSFACT(MavlinkActionsSettings, joystickActionsFile)
