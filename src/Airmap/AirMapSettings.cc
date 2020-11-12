/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirMapSettings.h"
#include "QGCApplication.h"

#include <QQmlEngine>
#include <QtQml>

DECLARE_SETTINGGROUP(AirMap, "AirMap")
{
    qmlRegisterUncreatableType<AirMapSettings>("QGroundControl.SettingsManager", 1, 0, "AirMapSettings", "Reference only");
}

DECLARE_SETTINGSFACT(AirMapSettings, usePersonalApiKey)
DECLARE_SETTINGSFACT(AirMapSettings, apiKey)
DECLARE_SETTINGSFACT(AirMapSettings, clientID)
DECLARE_SETTINGSFACT(AirMapSettings, userName)
DECLARE_SETTINGSFACT(AirMapSettings, password)
DECLARE_SETTINGSFACT(AirMapSettings, enableAirMap)
DECLARE_SETTINGSFACT(AirMapSettings, enableAirspace)
DECLARE_SETTINGSFACT(AirMapSettings, enableTelemetry)
