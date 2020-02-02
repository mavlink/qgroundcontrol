/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCApplication.h"
#include "FlightMapSettings.h"
#include "QGCMapEngine.h"
#include "AppSettings.h"
#include "SettingsManager.h"

#include <QQmlEngine>
#include <QtQml>

DECLARE_SETTINGGROUP(FlightMap, "FlightMap")
{
    qmlRegisterUncreatableType<FlightMapSettings>("QGroundControl.SettingsManager", 1, 0, "FlightMapSettings", "Reference only");
}

DECLARE_SETTINGSFACT(FlightMapSettings, mapProvider)
DECLARE_SETTINGSFACT(FlightMapSettings, mapType)
