#include "Viewer3DSettings.h"

DECLARE_SETTINGGROUP(Viewer3D, "Viewer3D")
{
#ifdef Q_OS_ANDROID
    // 3D view is disabled on Android (see QGCCorePlugin::adjustSettingMetaData);
    // hide the whole 3D View settings page
    setUserVisible(false);
#endif
}

DECLARE_SETTINGSFACT(Viewer3DSettings, enabled)
DECLARE_SETTINGSFACT(Viewer3DSettings, mapProvider)
DECLARE_SETTINGSFACT(Viewer3DSettings, osmFilePath)
DECLARE_SETTINGSFACT(Viewer3DSettings, buildingLevelHeight)
DECLARE_SETTINGSFACT(Viewer3DSettings, altitudeBias)
DECLARE_SETTINGSFACT(Viewer3DSettings, keepSceneAlive)
