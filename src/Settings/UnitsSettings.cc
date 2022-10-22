/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "UnitsSettings.h"

#include <QQmlEngine>
#include <QtQml>

DECLARE_SETTINGGROUP(Units, "Units")
{
    qmlRegisterUncreatableType<UnitsSettings>("QGroundControl.SettingsManager", 1, 0, "UnitsSettings", "Reference only");
}

DECLARE_SETTINGSFACT(UnitsSettings, horizontalDistanceUnits);
DECLARE_SETTINGSFACT(UnitsSettings, verticalDistanceUnits);
DECLARE_SETTINGSFACT(UnitsSettings, areaUnits);
DECLARE_SETTINGSFACT(UnitsSettings, speedUnits);
DECLARE_SETTINGSFACT(UnitsSettings, temperatureUnits);
DECLARE_SETTINGSFACT(UnitsSettings, weightUnits);
