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

class JoystickAxisSettings;
class JoystickButtonActionSettings;

class JoystickSettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    JoystickSettings(const QString &joystickName, int axisCount, int buttonCount, QObject* parent = nullptr);

    DEFINE_SETTINGFACT(calibrated)
    DEFINE_SETTINGFACT(circleCorrection)
    DEFINE_SETTINGFACT(useDeadband)
    DEFINE_SETTINGFACT(negativeThrust)
    DEFINE_SETTINGFACT(throttleAccumulator)
    DEFINE_SETTINGFACT(axisFrequencyHz)
    DEFINE_SETTINGFACT(buttonFrequencyHz)
    DEFINE_SETTINGFACT(throttleModeCenterZero)
    DEFINE_SETTINGFACT(enableManualControlExtensions)
    DEFINE_SETTINGFACT(transmitterMode)
    DEFINE_SETTINGFACT(exponentialPct)
    DEFINE_SETTINGS_GROUP_ARRAY(JoystickAxisSettings)
    DEFINE_SETTINGS_GROUP_ARRAY(JoystickButtonActionSettings)

private:
    QString _joystickName;
    int _axisCount;
    int _buttonCount;
};
