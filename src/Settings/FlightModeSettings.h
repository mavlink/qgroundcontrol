#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "SettingsGroup.h"

class FlightModeSettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
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
    DEFINE_SETTINGFACT(requireModeChangeConfirmation)
};
