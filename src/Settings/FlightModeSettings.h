/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "SettingsGroup.h"

class FlightModeSettings : public SettingsGroup
{
    Q_OBJECT

public:
    FlightModeSettings(QObject* parent = nullptr);

    DEFINE_SETTING_NAME_GROUP()
    DEFINE_SETTINGFACT(px4HiddenFlightModesMultiRotor)
    DEFINE_SETTINGFACT(px4HiddenFlightModesFixedWing)
    DEFINE_SETTINGFACT(px4HiddenFlightModesVTOL)
    DEFINE_SETTINGFACT(px4HiddenFlightModesRoverBoat)
    DEFINE_SETTINGFACT(px4HiddenFlightModesSub)
    DEFINE_SETTINGFACT(px4HiddenFlightModesAirship)
    DEFINE_SETTINGFACT(apmHiddenFlightModesMultiRotor)
    DEFINE_SETTINGFACT(apmHiddenFlightModesFixedWing)
    DEFINE_SETTINGFACT(apmHiddenFlightModesVTOL)
    DEFINE_SETTINGFACT(apmHiddenFlightModesRoverBoat)
    DEFINE_SETTINGFACT(apmHiddenFlightModesSub)
    DEFINE_SETTINGFACT(apmHiddenFlightModesAirship)
};
