/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/
#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "SettingsGroup.h"

class JoystickAxisSettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    JoystickAxisSettings(const QString &joystickName, int axis, QObject* parent = nullptr);

    DEFINE_SETTINGFACT(reversed)
    DEFINE_SETTINGFACT(function)
    DEFINE_SETTINGFACT(min)
    DEFINE_SETTINGFACT(max)
    DEFINE_SETTINGFACT(center)
    DEFINE_SETTINGFACT(deadband)

private:
    QString _joystickName;
};
