/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "JoystickAxisSettings.h"

JoystickAxisSettings::JoystickAxisSettings(const QString &joystickName, const int axis, QObject* parent)
    : SettingsGroup("JoystickAxis", QString("JoystickSettings/%1/Axis%2").arg(joystickName).arg(axis), parent)
    , _joystickName(joystickName)
{

}

DECLARE_SETTINGSFACT(JoystickAxisSettings, reversed)
DECLARE_SETTINGSFACT(JoystickAxisSettings, function)
DECLARE_SETTINGSFACT(JoystickAxisSettings, min)
DECLARE_SETTINGSFACT(JoystickAxisSettings, max)
DECLARE_SETTINGSFACT(JoystickAxisSettings, center)
DECLARE_SETTINGSFACT(JoystickAxisSettings, deadband)
