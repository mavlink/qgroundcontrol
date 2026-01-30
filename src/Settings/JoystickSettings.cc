/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "JoystickSettings.h"

JoystickSettings::JoystickSettings(const QString &joystickName, int axisCount, int buttonCount, QObject* parent)
    : SettingsGroup("Joystick", QString("JoystickSettingsV2/%1").arg(joystickName), parent)
    , _joystickName(joystickName)
    , _axisCount(axisCount)
    , _buttonCount(buttonCount)
{

}

DECLARE_SETTINGSFACT(JoystickSettings, calibrated)
DECLARE_SETTINGSFACT(JoystickSettings, circleCorrection)
DECLARE_SETTINGSFACT(JoystickSettings, useDeadband)
DECLARE_SETTINGSFACT(JoystickSettings, negativeThrust)
DECLARE_SETTINGSFACT(JoystickSettings, throttleSmoothing)
DECLARE_SETTINGSFACT(JoystickSettings, axisFrequencyHz)
DECLARE_SETTINGSFACT(JoystickSettings, buttonFrequencyHz)
DECLARE_SETTINGSFACT(JoystickSettings, throttleModeCenterZero)
DECLARE_SETTINGSFACT(JoystickSettings, transmitterMode)
DECLARE_SETTINGSFACT(JoystickSettings, exponentialPct)
DECLARE_SETTINGSFACT(JoystickSettings, enableManualControlAux1)
DECLARE_SETTINGSFACT(JoystickSettings, enableManualControlAux2)
DECLARE_SETTINGSFACT(JoystickSettings, enableManualControlAux3)
DECLARE_SETTINGSFACT(JoystickSettings, enableManualControlAux4)
DECLARE_SETTINGSFACT(JoystickSettings, enableManualControlAux5)
DECLARE_SETTINGSFACT(JoystickSettings, enableManualControlAux6)
DECLARE_SETTINGSFACT(JoystickSettings, enableManualControlPitchExtension)
DECLARE_SETTINGSFACT(JoystickSettings, enableManualControlRollExtension)
