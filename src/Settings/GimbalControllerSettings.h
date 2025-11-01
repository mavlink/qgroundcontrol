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

class GimbalControllerSettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
public:
    GimbalControllerSettings(QObject* parent = nullptr);
    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(EnableOnScreenControl)
    DEFINE_SETTINGFACT(ControlType)
    DEFINE_SETTINGFACT(CameraVFov)
    DEFINE_SETTINGFACT(CameraHFov)
    DEFINE_SETTINGFACT(CameraSlideSpeed)
    DEFINE_SETTINGFACT(showAzimuthIndicatorOnMap)
    DEFINE_SETTINGFACT(toolbarIndicatorShowAzimuth)
    DEFINE_SETTINGFACT(toolbarIndicatorShowAcquireReleaseControl)
    DEFINE_SETTINGFACT(joystickButtonsSpeed)
    DEFINE_SETTINGFACT(zoomMaxSpeed)
    DEFINE_SETTINGFACT(zoomMinSpeed)
};
