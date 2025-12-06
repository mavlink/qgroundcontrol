/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "GimbalControllerSettings.h"

DECLARE_SETTINGGROUP(GimbalController, "GimbalController")
{
}

DECLARE_SETTINGSFACT(GimbalControllerSettings, EnableOnScreenControl)
DECLARE_SETTINGSFACT(GimbalControllerSettings, ControlType)
DECLARE_SETTINGSFACT(GimbalControllerSettings, CameraVFov)
DECLARE_SETTINGSFACT(GimbalControllerSettings, CameraHFov)
DECLARE_SETTINGSFACT(GimbalControllerSettings, CameraSlideSpeed)
DECLARE_SETTINGSFACT(GimbalControllerSettings, showAzimuthIndicatorOnMap)
DECLARE_SETTINGSFACT(GimbalControllerSettings, toolbarIndicatorShowAzimuth)
DECLARE_SETTINGSFACT(GimbalControllerSettings, toolbarIndicatorShowAcquireReleaseControl)
DECLARE_SETTINGSFACT(GimbalControllerSettings, joystickButtonsSpeed)
DECLARE_SETTINGSFACT(GimbalControllerSettings, zoomMaxSpeed)
DECLARE_SETTINGSFACT(GimbalControllerSettings, zoomMinSpeed)
