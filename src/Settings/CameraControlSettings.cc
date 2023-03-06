/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CameraControlSettings.h"
#include "QGCApplication.h"

#include <QQmlEngine>
#include <QtQml>
#include <QVariantList>

#ifndef QGC_DISABLE_UVC
#include <QCameraInfo>
#endif

DECLARE_SETTINGGROUP(CameraControl, "CameraControl")
{
    qmlRegisterUncreatableType<CameraControlSettings>("QGroundControl.SettingsManager", 1, 0, "CameraControlSettings", "Reference only");
}

DECLARE_SETTINGSFACT(CameraControlSettings, EnableOnScreenControl)
DECLARE_SETTINGSFACT(CameraControlSettings, ControlType)
DECLARE_SETTINGSFACT(CameraControlSettings, CameraVFov)
DECLARE_SETTINGSFACT(CameraControlSettings, CameraHFov)
DECLARE_SETTINGSFACT(CameraControlSettings, CameraSlideSpeed)