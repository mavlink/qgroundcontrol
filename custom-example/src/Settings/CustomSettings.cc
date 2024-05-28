/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CustomSettings.h"

DECLARE_SETTINGGROUP(Custom, "")
{
    qmlRegisterUncreatableType<AppSettings>("QGroundControl.SettingsManager", 1, 0, "CustomSettings", "Reference only");
}

DECLARE_SETTINGSFACT(AppSettings, customSetting)
