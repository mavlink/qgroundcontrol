/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AppSettings.h"
#include "QGCPalette.h"
#include "QGCApplication.h"

#include <QQmlEngine>
#include <QtQml>
#include <QStandardPaths>

const char* AppSettings::name =                                         "App";
const char* AppSettings::settingsGroup =                                ""; // settings are in root group

const char* AppSettings::offlineEditingFirmwareTypeName =       "OfflineEditingFirmwareType";
const char* AppSettings::offlineEditingVehicleTypeName =        "OfflineEditingVehicleType";
const char* AppSettings::offlineEditingCruiseSpeedName =        "OfflineEditingCruiseSpeed";
const char* AppSettings::offlineEditingHoverSpeedName =         "OfflineEditingHoverSpeed";
const char* AppSettings::offlineEditingAscentSpeedName =        "OfflineEditingAscentSpeed";
const char* AppSettings::offlineEditingDescentSpeedName =       "OfflineEditingDescentSpeed";
const char* AppSettings::batteryPercentRemainingAnnounceName =  "batteryPercentRemainingAnnounce";
const char* AppSettings::defaultMissionItemAltitudeName =       "DefaultMissionItemAltitude";
const char* AppSettings::telemetrySaveName =                            "PromptFLightDataSave";
const char* AppSettings::telemetrySaveNotArmedName =                    "PromptFLightDataSaveNotArmed";
const char* AppSettings::audioMutedName =                               "AudioMuted";
const char* AppSettings::virtualJoystickName =                          "VirtualTabletJoystick";
const char* AppSettings::appFontPointSizeName =                         "BaseDeviceFontPointSize";
const char* AppSettings::indoorPaletteName =                            "StyleIsDark";
const char* AppSettings::showLargeCompassName =                         "ShowLargeCompass";
const char* AppSettings::savePathName =                                 "SavePath";
const char* AppSettings::autoLoadMissionsName =                         "AutoLoadMissions";
const char* AppSettings::useChecklistName =                             "UseChecklist";
const char* AppSettings::mapboxTokenName =                              "MapboxToken";
const char* AppSettings::esriTokenName =                                "EsriToken";
const char* AppSettings::defaultFirmwareTypeName =                      "DefaultFirmwareType";
const char* AppSettings::gstDebugName =                                 "GstreamerDebugLevel";
const char* AppSettings::followTargetName =                             "FollowTarget";

const char* AppSettings::parameterFileExtension =   "params";
const char* AppSettings::planFileExtension =        "plan";
const char* AppSettings::missionFileExtension =     "mission";
const char* AppSettings::waypointsFileExtension =   "waypoints";;
const char* AppSettings::telemetryFileExtension =   "tlog";
const char* AppSettings::kmlFileExtension =         "kml";
const char* AppSettings::logFileExtension =         "ulg";

const char* AppSettings::parameterDirectory =       "Parameters";
const char* AppSettings::telemetryDirectory =       "Telemetry";
const char* AppSettings::missionDirectory =         "Missions";
const char* AppSettings::logDirectory =             "Logs";
const char* AppSettings::videoDirectory =           "Video";
const char* AppSettings::crashDirectory =           "CrashLogs";

AppSettings::AppSettings(QObject* parent)
    : SettingsGroup(name, settingsGroup, parent)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    qmlRegisterUncreatableType<AppSettings>("QGroundControl.SettingsManager", 1, 0, "AppSettings", "Reference only");
    QGCPalette::setGlobalTheme(indoorPalette()->rawValue().toBool() ? QGCPalette::Dark : QGCPalette::Light);

    // Instantiate savePath so we can check for override and setup default path if needed

    SettingsFact* savePathFact = qobject_cast<SettingsFact*>(savePath());
    QString appName = qgcApp()->applicationName();
    if (savePathFact->rawValue().toString().isEmpty() && _nameToMetaDataMap[savePathName]->rawDefaultValue().toString().isEmpty()) {
#ifdef __mobile__
#ifdef __ios__
        QDir rootDir = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
#else
        QDir rootDir = QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation));
#endif
        savePathFact->setVisible(false);
#else
        QDir rootDir = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
#endif
        savePathFact->setRawValue(rootDir.filePath(appName));
    }

    connect(savePathFact, &Fact::rawValueChanged, this, &AppSettings::savePathsChanged);
    connect(savePathFact, &Fact::rawValueChanged, this, &AppSettings::_checkSavePathDirectories);

    _checkSavePathDirectories();
}

void AppSettings::_checkSavePathDirectories(void)
{
    QDir savePathDir(savePath()->rawValue().toString());
    if (!savePathDir.exists()) {
        QDir().mkpath(savePathDir.absolutePath());
    }
    if (savePathDir.exists()) {
        savePathDir.mkdir(parameterDirectory);
        savePathDir.mkdir(telemetryDirectory);
        savePathDir.mkdir(missionDirectory);
        savePathDir.mkdir(logDirectory);
        savePathDir.mkdir(videoDirectory);
        savePathDir.mkdir(crashDirectory);
    }
}

void AppSettings::_indoorPaletteChanged(void)
{
    qgcApp()->_loadCurrentStyleSheet();
    QGCPalette::setGlobalTheme(indoorPalette()->rawValue().toBool() ? QGCPalette::Dark : QGCPalette::Light);
}

MAV_AUTOPILOT AppSettings::offlineEditingFirmwareTypeFromFirmwareType(MAV_AUTOPILOT firmwareType)
{
    if (firmwareType != MAV_AUTOPILOT_PX4 && firmwareType != MAV_AUTOPILOT_ARDUPILOTMEGA) {
        firmwareType = MAV_AUTOPILOT_GENERIC;
    }
    return firmwareType;
}

MAV_TYPE AppSettings::offlineEditingVehicleTypeFromVehicleType(MAV_TYPE vehicleType)
{
    if (QGCMAVLink::isRover(vehicleType)) {
        return MAV_TYPE_GROUND_ROVER;
    } else if (QGCMAVLink::isSub(vehicleType)) {
        return MAV_TYPE_SUBMARINE;
    } else if (QGCMAVLink::isVTOL(vehicleType)) {
        return MAV_TYPE_VTOL_QUADROTOR;
    } else if (QGCMAVLink::isFixedWing(vehicleType)) {
        return MAV_TYPE_FIXED_WING;
    } else {
        return MAV_TYPE_QUADROTOR;
    }
}


