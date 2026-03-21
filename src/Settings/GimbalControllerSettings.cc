#include "GimbalControllerSettings.h"

DECLARE_SETTINGGROUP(GimbalController, "GimbalController")
{
    // Setting names were changed from PascalCase to camelCase
    // Migrate old settings to new names
    QSettings settings;
    settings.beginGroup(_name);
    static const QMap<QString, QString> renamedKeys = {
        { "EnableOnScreenControl", "enableOnScreenControl" },
        { "ControlType",           "clickAndDrag" },
        { "CameraVFov",            "cameraVFov" },
        { "CameraHFov",            "cameraHFov" },
        { "CameraSlideSpeed",      "cameraSlideSpeed" },
    };
    for (auto it = renamedKeys.constBegin(); it != renamedKeys.constEnd(); ++it) {
        if (settings.contains(it.key())) {
            settings.setValue(it.value(), settings.value(it.key()));
            settings.remove(it.key());
        }
    }
    settings.endGroup();
}

DECLARE_SETTINGSFACT(GimbalControllerSettings, enableOnScreenControl)
DECLARE_SETTINGSFACT(GimbalControllerSettings, clickAndDrag)
DECLARE_SETTINGSFACT(GimbalControllerSettings, cameraVFov)
DECLARE_SETTINGSFACT(GimbalControllerSettings, cameraHFov)
DECLARE_SETTINGSFACT(GimbalControllerSettings, cameraSlideSpeed)
DECLARE_SETTINGSFACT(GimbalControllerSettings, showAzimuthIndicatorOnMap)
DECLARE_SETTINGSFACT(GimbalControllerSettings, toolbarIndicatorShowAzimuth)
DECLARE_SETTINGSFACT(GimbalControllerSettings, toolbarIndicatorShowAcquireReleaseControl)
DECLARE_SETTINGSFACT(GimbalControllerSettings, joystickButtonsSpeed)
DECLARE_SETTINGSFACT(GimbalControllerSettings, zoomMaxSpeed)
DECLARE_SETTINGSFACT(GimbalControllerSettings, zoomMinSpeed)
