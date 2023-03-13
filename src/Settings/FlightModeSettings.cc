/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCApplication.h"
#include "FlightModeSettings.h"
#include "QGCMapEngine.h"
#include "AppSettings.h"
#include "SettingsManager.h"

#include <QQmlEngine>
#include <QtQml>

DECLARE_SETTINGGROUP(FlightMode, "FlightMode")
{
    qmlRegisterUncreatableType<FlightModeSettings>("QGroundControl.SettingsManager", 1, 0, "FlightModeSettings", "Reference only");
}

DECLARE_SETTINGSFACT(FlightModeSettings, px4HiddenFlightModes)
DECLARE_SETTINGSFACT(FlightModeSettings, apmHiddenFlightModes)
