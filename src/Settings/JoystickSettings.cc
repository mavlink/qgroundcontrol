/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "JoystickSettings.h"
#include "JoystickAxisSettings.h"
#include "JoystickButtonActionSettings.h"

JoystickSettings::JoystickSettings(const QString &joystickName, int axisCount, int buttonCount, QObject* parent)
    : SettingsGroup("Joystick", QString("JoystickSettings/%1").arg(joystickName), parent)
    , _joystickName(joystickName)
    , _axisCount(axisCount)
    , _buttonCount(buttonCount)
{

}

DECLARE_SETTINGSFACT(JoystickSettings, calibrated)
DECLARE_SETTINGSFACT(JoystickSettings, circleCorrection)
DECLARE_SETTINGSFACT(JoystickSettings, useDeadband)
DECLARE_SETTINGSFACT(JoystickSettings, negativeThrust)
DECLARE_SETTINGSFACT(JoystickSettings, throttleAccumulator)
DECLARE_SETTINGSFACT(JoystickSettings, axisFrequencyHz)
DECLARE_SETTINGSFACT(JoystickSettings, buttonFrequencyHz)
DECLARE_SETTINGSFACT(JoystickSettings, throttleModeCenterZero)
DECLARE_SETTINGSFACT(JoystickSettings, enableManualControlExtensions)
DECLARE_SETTINGSFACT(JoystickSettings, transmitterMode)
DECLARE_SETTINGSFACT(JoystickSettings, exponentialPct)
DECLARE_SETTINGS_GROUP_ARRAY(_settingsGroup, JoystickSettings, JoystickAxisSettings, _axisCount)
DECLARE_SETTINGS_GROUP_ARRAY(_settingsGroup, JoystickSettings, JoystickButtonActionSettings, _buttonCount)
