#include "JoystickButtonActionSettings.h"

JoystickButtonActionSettings::JoystickButtonActionSettings(const QString &joystickName, const int buttonIndex, QObject* parent)
    : SettingsGroup("JoystickButtonAction", QString("JoystickSettings/%1/ButtonAction%2").arg(joystickName).arg(buttonIndex), parent)
    , _joystickName(joystickName)
{

}

DECLARE_SETTINGSFACT(JoystickButtonActionSettings, actionName)
DECLARE_SETTINGSFACT(JoystickButtonActionSettings, repeat)
