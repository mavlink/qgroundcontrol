/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/
#pragma once

#include "SettingsGroup.h"

class GimbalControllerSettings : public SettingsGroup
{
    Q_OBJECT

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
};