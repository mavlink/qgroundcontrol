/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FlightModeSettings.h"

#include <QtQml/QQmlEngine>

DECLARE_SETTINGGROUP(FlightMode, "FlightMode")
{
    qmlRegisterUncreatableType<FlightModeSettings>("QGroundControl.SettingsManager", 1, 0, "FlightModeSettings", "Reference only");
}

DECLARE_SETTINGSFACT(FlightModeSettings, px4HiddenFlightModesMultiRotor)
DECLARE_SETTINGSFACT(FlightModeSettings, px4HiddenFlightModesFixedWing)
DECLARE_SETTINGSFACT(FlightModeSettings, px4HiddenFlightModesVTOL)
DECLARE_SETTINGSFACT(FlightModeSettings, px4HiddenFlightModesRoverBoat)
DECLARE_SETTINGSFACT(FlightModeSettings, px4HiddenFlightModesSub)
DECLARE_SETTINGSFACT(FlightModeSettings, px4HiddenFlightModesAirship)
DECLARE_SETTINGSFACT(FlightModeSettings, apmHiddenFlightModesMultiRotor)
DECLARE_SETTINGSFACT(FlightModeSettings, apmHiddenFlightModesFixedWing)
DECLARE_SETTINGSFACT(FlightModeSettings, apmHiddenFlightModesVTOL)
DECLARE_SETTINGSFACT(FlightModeSettings, apmHiddenFlightModesRoverBoat)
DECLARE_SETTINGSFACT(FlightModeSettings, apmHiddenFlightModesSub)
DECLARE_SETTINGSFACT(FlightModeSettings, apmHiddenFlightModesAirship)
