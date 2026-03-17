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

    DEFINE_SETTINGFACT(enableOnScreenControl)
    DEFINE_SETTINGFACT(clickAndDrag)
    DEFINE_SETTINGFACT(cameraVFov)
    DEFINE_SETTINGFACT(cameraHFov)
    DEFINE_SETTINGFACT(cameraSlideSpeed)
    DEFINE_SETTINGFACT(showAzimuthIndicatorOnMap)
    DEFINE_SETTINGFACT(toolbarIndicatorShowAzimuth)
    DEFINE_SETTINGFACT(toolbarIndicatorShowAcquireReleaseControl)
    DEFINE_SETTINGFACT(joystickButtonsSpeed)
    DEFINE_SETTINGFACT(zoomMaxSpeed)
    DEFINE_SETTINGFACT(zoomMinSpeed)
};
