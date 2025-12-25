#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "SettingsGroup.h"

class JoystickButtonActionSettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    JoystickButtonActionSettings(const QString &joystickName, int buttonIndex, QObject* parent = nullptr);

    DEFINE_SETTINGFACT(actionName)
    DEFINE_SETTINGFACT(repeat)

private:
    QString _joystickName;
};
