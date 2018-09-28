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

#include <mpParser.h>

QGC_LOGGING_CATEGORY(AppSettingsLog, "AppSettingsLog")

const char* AppSettings::name =                                         "App";
const char* AppSettings::settingsGroup =                                ""; // settings are in root group

const char* AppSettings::offlineEditingFirmwareTypeSettingsName =       "OfflineEditingFirmwareType";
const char* AppSettings::offlineEditingVehicleTypeSettingsName =        "OfflineEditingVehicleType";
const char* AppSettings::offlineEditingCruiseSpeedSettingsName =        "OfflineEditingCruiseSpeed";
const char* AppSettings::offlineEditingHoverSpeedSettingsName =         "OfflineEditingHoverSpeed";
const char* AppSettings::offlineEditingAscentSpeedSettingsName =        "OfflineEditingAscentSpeed";
const char* AppSettings::offlineEditingDescentSpeedSettingsName =       "OfflineEditingDescentSpeed";
const char* AppSettings::batteryPercentRemainingAnnounceSettingsName =  "batteryPercentRemainingAnnounce";
const char* AppSettings::defaultMissionItemAltitudeSettingsName =       "DefaultMissionItemAltitude";
const char* AppSettings::unactivatedVehiclesTrajectoryPointsOpacitySettingsName = "UnactivatedVehiclesTrajectoryPointsOpacity";
const char* AppSettings::unactivatedVehiclesIconOpacitySettingsName =   "UnactivatedVehiclesIconOpacity";
const char* AppSettings::vehiclesTrajectoryPointsColorRSettingsName =   "VehiclesTrajectoryPointsColorR";
const char* AppSettings::vehiclesTrajectoryPointsColorGSettingsName =   "VehiclesTrajectoryPointsColorG";
const char* AppSettings::vehiclesTrajectoryPointsColorBSettingsName =   "VehiclesTrajectoryPointsColorB";
const char* AppSettings::vehiclesTrajectoryPointsColorASettingsName =   "VehiclesTrajectoryPointsColorA";
const char* AppSettings::vehiclesIconColorRSettingsName =               "VehiclesIconColorR";
const char* AppSettings::vehiclesIconColorGSettingsName =               "VehiclesIconColorG";
const char* AppSettings::vehiclesIconColorBSettingsName =               "VehiclesIconColorB";
const char* AppSettings::vehiclesIconColorASettingsName =               "VehiclesIconColorA";
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
const char* AppSettings::waypointsFileExtension =   "waypoints";
const char* AppSettings::fenceFileExtension =       "fence";
const char* AppSettings::rallyPointFileExtension =  "rally";
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
    : SettingsGroup                         (name, settingsGroup, parent)
    , _offlineEditingFirmwareTypeFact       (NULL)
    , _offlineEditingVehicleTypeFact        (NULL)
    , _offlineEditingCruiseSpeedFact        (NULL)
    , _offlineEditingHoverSpeedFact         (NULL)
    , _offlineEditingAscentSpeedFact        (NULL)
    , _offlineEditingDescentSpeedFact       (NULL)
    , _batteryPercentRemainingAnnounceFact  (NULL)
    , _defaultMissionItemAltitudeFact       (NULL)
    , _unactivatedVehiclesTrajectoryPointsOpacityFact (NULL)
    , _unactivatedVehiclesIconOpacityFact   (NULL)
    , _vehiclesTrajectoryPointsColorRFact   (NULL)
    , _vehiclesTrajectoryPointsColorGFact   (NULL)
    , _vehiclesTrajectoryPointsColorBFact   (NULL)
    , _vehiclesTrajectoryPointsColorAFact   (NULL)
    , _vehiclesIconColorRFact               (NULL)
    , _vehiclesIconColorGFact               (NULL)
    , _vehiclesIconColorBFact               (NULL)
    , _vehiclesIconColorAFact               (NULL)
    , _telemetrySaveFact                    (NULL)
    , _telemetrySaveNotArmedFact            (NULL)
    , _audioMutedFact                       (NULL)
    , _virtualJoystickFact                  (NULL)
    , _appFontPointSizeFact                 (NULL)
    , _indoorPaletteFact                    (NULL)
    , _showLargeCompassFact                 (NULL)
    , _savePathFact                         (NULL)
    , _autoLoadMissionsFact                 (NULL)
    , _useChecklistFact                     (NULL)
    , _mapboxTokenFact                      (NULL)
    , _esriTokenFact                        (NULL)
    , _defaultFirmwareTypeFact              (NULL)
    , _gstDebugFact                         (NULL)
    , _followTargetFact                     (NULL)
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

QColor parseColor(Vehicle *vehicle, const QString &rString, const QString &gString, const QString &bString, const QString &aString)
{
    int r = 255, g = 0, b = 0, a = 255;

    if (!vehicle)
    {
        return QColor(r, g, b, a);
    }

    MultiVehicleManager *multiVehicleManager = qgcApp()->toolbox()->multiVehicleManager();

    int vehicleId = vehicle->id();
    int vehicleCount = multiVehicleManager->vehicles()->count();
    bool isActivatedVehicle = vehicle->active();

    try
    {
        mup::ParserX parser;
        mup::Value vehicle_id(vehicleId);
        parser.DefineVar(_T("vehicle_id"), mup::Variable(&vehicle_id));
        mup::Value vehicle_count(vehicleCount);
        parser.DefineVar(_T("vehicle_count"), mup::Variable(&vehicle_count));
        mup::Value is_activated_vehicle(isActivatedVehicle);
        parser.DefineVar(_T("is_activated_vehicle"), mup::Variable(&is_activated_vehicle));
#if defined(_UNICODE)
        parser.SetExpr(rString.toStdWString());
        r = parser.Eval().GetInteger();
        parser.SetExpr(gString.toStdWString());
        g = parser.Eval().GetInteger();
        parser.SetExpr(bString.toStdWString());
        b = parser.Eval().GetInteger();
        parser.SetExpr(aString.toStdWString());
        a = parser.Eval().GetInteger();
#else
        parser.SetExpr(rString.toStdString());
        r = parser.Eval().GetInteger();
        parser.SetExpr(gString.toStdString());
        g = parser.Eval().GetInteger();
        parser.SetExpr(bString.toStdString());
        b = parser.Eval().GetInteger();
        parser.SetExpr(aString.toStdString());
        a = parser.Eval().GetInteger();
#endif
    }
    catch (mup::ParserError &e)
    {
#if defined(_UNICODE)
        qCDebug(AppSettingsLog()) << "muParser Error: " << QString::fromStdWString(e.GetMsg()) << "(error code: " << e.GetCode() << ")";
#else
        qCDebug(AppSettingsLog()) << "muParser Error: " << QString::fromStdString(e.GetMsg()) << "(error code: " << e.GetCode() << ")";
#endif
    }

    return QColor(r, g, b, a);
}

QColor AppSettings::getVehiclesTrajectoryPointsColor(Vehicle *vehicle)
{
    const QString rString = vehiclesTrajectoryPointsColorR()->cookedValue().toString();
    const QString gString = vehiclesTrajectoryPointsColorG()->cookedValue().toString();
    const QString bString = vehiclesTrajectoryPointsColorB()->cookedValue().toString();
    const QString aString = vehiclesTrajectoryPointsColorA()->cookedValue().toString();

    return parseColor(vehicle, rString, gString, bString, aString);
}

QColor AppSettings::getVehiclesIconColor(Vehicle *vehicle)
{
    const QString rString = vehiclesIconColorR()->cookedValue().toString();
    const QString gString = vehiclesIconColorG()->cookedValue().toString();
    const QString bString = vehiclesIconColorB()->cookedValue().toString();
    const QString aString = vehiclesIconColorA()->cookedValue().toString();

    return parseColor(vehicle, rString, gString, bString, aString);
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

Fact* AppSettings::offlineEditingFirmwareType(void)
{
    if (!_offlineEditingFirmwareTypeFact) {
        _offlineEditingFirmwareTypeFact = _createSettingsFact(offlineEditingFirmwareTypeSettingsName);
    }

    return _offlineEditingFirmwareTypeFact;
}

Fact* AppSettings::offlineEditingVehicleType(void)
{
    if (!_offlineEditingVehicleTypeFact) {
        _offlineEditingVehicleTypeFact = _createSettingsFact(offlineEditingVehicleTypeSettingsName);
    }

    return _offlineEditingVehicleTypeFact;
}

Fact* AppSettings::offlineEditingCruiseSpeed(void)
{
    if (!_offlineEditingCruiseSpeedFact) {
        _offlineEditingCruiseSpeedFact = _createSettingsFact(offlineEditingCruiseSpeedSettingsName);
    }
    return _offlineEditingCruiseSpeedFact;
}

Fact* AppSettings::offlineEditingHoverSpeed(void)
{
    if (!_offlineEditingHoverSpeedFact) {
        _offlineEditingHoverSpeedFact = _createSettingsFact(offlineEditingHoverSpeedSettingsName);
    }
    return _offlineEditingHoverSpeedFact;
}

Fact* AppSettings::offlineEditingAscentSpeed(void)
{
    if (!_offlineEditingAscentSpeedFact) {
        _offlineEditingAscentSpeedFact = _createSettingsFact(offlineEditingAscentSpeedSettingsName);
    }
    return _offlineEditingAscentSpeedFact;
}

Fact* AppSettings::offlineEditingDescentSpeed(void)
{
    if (!_offlineEditingDescentSpeedFact) {
        _offlineEditingDescentSpeedFact = _createSettingsFact(offlineEditingDescentSpeedSettingsName);
    }
    return _offlineEditingDescentSpeedFact;
}

Fact* AppSettings::batteryPercentRemainingAnnounce(void)
{
    if (!_batteryPercentRemainingAnnounceFact) {
        _batteryPercentRemainingAnnounceFact = _createSettingsFact(batteryPercentRemainingAnnounceSettingsName);
    }

    return _batteryPercentRemainingAnnounceFact;
}

Fact* AppSettings::defaultMissionItemAltitude(void)
{
    if (!_defaultMissionItemAltitudeFact) {
        _defaultMissionItemAltitudeFact = _createSettingsFact(defaultMissionItemAltitudeSettingsName);
    }

    return _defaultMissionItemAltitudeFact;
}

Fact* AppSettings::unactivatedVehiclesTrajectoryPointsOpacity(void)
{
    if (!_unactivatedVehiclesTrajectoryPointsOpacityFact) {
        _unactivatedVehiclesTrajectoryPointsOpacityFact = _createSettingsFact(unactivatedVehiclesTrajectoryPointsOpacitySettingsName);
    }

    return _unactivatedVehiclesTrajectoryPointsOpacityFact;
}

Fact* AppSettings::unactivatedVehiclesIconOpacity(void)
{
    if (!_unactivatedVehiclesIconOpacityFact) {
        _unactivatedVehiclesIconOpacityFact = _createSettingsFact(unactivatedVehiclesIconOpacitySettingsName);
    }

    return _unactivatedVehiclesIconOpacityFact;
}

Fact* AppSettings::vehiclesTrajectoryPointsColorR(void)
{
    if (!_vehiclesTrajectoryPointsColorRFact) {
        _vehiclesTrajectoryPointsColorRFact = _createSettingsFact(vehiclesTrajectoryPointsColorRSettingsName);
        connect(_vehiclesTrajectoryPointsColorRFact, &Fact::valueChanged, this, &AppSettings::vehiclesTrajectoryPointsColorChanged);
    }

    return _vehiclesTrajectoryPointsColorRFact;
}

Fact* AppSettings::vehiclesTrajectoryPointsColorG(void)
{
    if (!_vehiclesTrajectoryPointsColorGFact) {
        _vehiclesTrajectoryPointsColorGFact = _createSettingsFact(vehiclesTrajectoryPointsColorGSettingsName);
        connect(_vehiclesTrajectoryPointsColorGFact, &Fact::valueChanged, this, &AppSettings::vehiclesTrajectoryPointsColorChanged);
    }

    return _vehiclesTrajectoryPointsColorGFact;
}

Fact* AppSettings::vehiclesTrajectoryPointsColorB(void)
{
    if (!_vehiclesTrajectoryPointsColorBFact) {
        _vehiclesTrajectoryPointsColorBFact = _createSettingsFact(vehiclesTrajectoryPointsColorBSettingsName);
        connect(_vehiclesTrajectoryPointsColorBFact, &Fact::valueChanged, this, &AppSettings::vehiclesTrajectoryPointsColorChanged);
    }

    return _vehiclesTrajectoryPointsColorBFact;
}

Fact* AppSettings::vehiclesTrajectoryPointsColorA(void)
{
    if (!_vehiclesTrajectoryPointsColorAFact) {
        _vehiclesTrajectoryPointsColorAFact = _createSettingsFact(vehiclesTrajectoryPointsColorASettingsName);
        connect(_vehiclesTrajectoryPointsColorAFact, &Fact::valueChanged, this, &AppSettings::vehiclesTrajectoryPointsColorChanged);
    }

    return _vehiclesTrajectoryPointsColorAFact;
}

Fact* AppSettings::vehiclesIconColorR(void)
{
    if (!_vehiclesIconColorRFact) {
        _vehiclesIconColorRFact = _createSettingsFact(vehiclesIconColorRSettingsName);
        connect(_vehiclesIconColorRFact, &Fact::valueChanged, this, &AppSettings::vehiclesIconColorChanged);
    }

    return _vehiclesIconColorRFact;
}

Fact* AppSettings::vehiclesIconColorG(void)
{
    if (!_vehiclesIconColorGFact) {
        _vehiclesIconColorGFact = _createSettingsFact(vehiclesIconColorGSettingsName);
        connect(_vehiclesIconColorGFact, &Fact::valueChanged, this, &AppSettings::vehiclesIconColorChanged);
    }

    return _vehiclesIconColorGFact;
}

Fact* AppSettings::vehiclesIconColorB(void)
{
    if (!_vehiclesIconColorBFact) {
        _vehiclesIconColorBFact = _createSettingsFact(vehiclesIconColorBSettingsName);
        connect(_vehiclesIconColorBFact, &Fact::valueChanged, this, &AppSettings::vehiclesIconColorChanged);
    }

    return _vehiclesIconColorBFact;
}

Fact* AppSettings::vehiclesIconColorA(void)
{
    if (!_vehiclesIconColorAFact) {
        _vehiclesIconColorAFact = _createSettingsFact(vehiclesIconColorASettingsName);
        connect(_vehiclesIconColorAFact, &Fact::valueChanged, this, &AppSettings::vehiclesIconColorChanged);
    }

    return _vehiclesIconColorAFact;
}

Fact* AppSettings::telemetrySave(void)
{
    if (!_telemetrySaveFact) {
        _telemetrySaveFact = _createSettingsFact(telemetrySaveName);
    }

    return _telemetrySaveFact;
}

Fact* AppSettings::telemetrySaveNotArmed(void)
{
    if (!_telemetrySaveNotArmedFact) {
        _telemetrySaveNotArmedFact = _createSettingsFact(telemetrySaveNotArmedName);
    }

    return _telemetrySaveNotArmedFact;
}

Fact* AppSettings::audioMuted(void)
{
    if (!_audioMutedFact) {
        _audioMutedFact = _createSettingsFact(audioMutedName);
    }

    return _audioMutedFact;
}

Fact* AppSettings::useChecklist(void)
{
    if (!_useChecklistFact) {
        _useChecklistFact = _createSettingsFact(useChecklistName);
    }

    return _useChecklistFact;
}

Fact* AppSettings::appFontPointSize(void)
{
    if (!_appFontPointSizeFact) {
        _appFontPointSizeFact = _createSettingsFact(appFontPointSizeName);
    }

    return _appFontPointSizeFact;
}

Fact* AppSettings::virtualJoystick(void)
{
    if (!_virtualJoystickFact) {
        _virtualJoystickFact = _createSettingsFact(virtualJoystickName);
    }

    return _virtualJoystickFact;
}

Fact* AppSettings::gstDebug(void)
{
    if (!_gstDebugFact) {
        _gstDebugFact = _createSettingsFact(gstDebugName);
    }

    return _gstDebugFact;
}

Fact* AppSettings::indoorPalette(void)
{
    if (!_indoorPaletteFact) {
        _indoorPaletteFact = _createSettingsFact(indoorPaletteName);
        connect(_indoorPaletteFact, &Fact::rawValueChanged, this, &AppSettings::_indoorPaletteChanged);
    }

    return _indoorPaletteFact;
}

void AppSettings::_indoorPaletteChanged(void)
{
    qgcApp()->_loadCurrentStyleSheet();
    QGCPalette::setGlobalTheme(indoorPalette()->rawValue().toBool() ? QGCPalette::Dark : QGCPalette::Light);
}

Fact* AppSettings::showLargeCompass(void)
{
    if (!_showLargeCompassFact) {
        _showLargeCompassFact = _createSettingsFact(showLargeCompassName);
    }

    return _showLargeCompassFact;
}

Fact* AppSettings::savePath(void)
{
    if (!_savePathFact) {
        _savePathFact = _createSettingsFact(savePathName);
    }

    return _savePathFact;
}

QString AppSettings::missionSavePath(void)
{
    QString path = savePath()->rawValue().toString();
    if (!path.isEmpty() && QDir(path).exists()) {
        QDir dir(path);
        return dir.filePath(missionDirectory);
    }

    return QString();
}

QString AppSettings::parameterSavePath(void)
{
    QString path = savePath()->rawValue().toString();
    if (!path.isEmpty() && QDir(path).exists()) {
        QDir dir(path);
        return dir.filePath(parameterDirectory);
    }

    return QString();
}

QString AppSettings::telemetrySavePath(void)
{
    QString path = savePath()->rawValue().toString();
    if (!path.isEmpty() && QDir(path).exists()) {
        QDir dir(path);
        return dir.filePath(telemetryDirectory);
    }

    return QString();
}

QString AppSettings::logSavePath(void)
{
    QString path = savePath()->rawValue().toString();
    if (!path.isEmpty() && QDir(path).exists()) {
        QDir dir(path);
        return dir.filePath(logDirectory);
    }

    return QString();
}

QString AppSettings::videoSavePath(void)
{
    QString path = savePath()->rawValue().toString();
    if (!path.isEmpty() && QDir(path).exists()) {
        QDir dir(path);
        return dir.filePath(videoDirectory);
    }

    return QString();
}

QString AppSettings::crashSavePath(void)
{
    QString path = savePath()->rawValue().toString();
    if (!path.isEmpty() && QDir(path).exists()) {
        QDir dir(path);
        return dir.filePath(crashDirectory);
    }

    return QString();
}

Fact* AppSettings::autoLoadMissions(void)
{
    if (!_autoLoadMissionsFact) {
        _autoLoadMissionsFact = _createSettingsFact(autoLoadMissionsName);
    }

    return _autoLoadMissionsFact;
}

Fact* AppSettings::mapboxToken(void)
{
    if (!_mapboxTokenFact) {
        _mapboxTokenFact = _createSettingsFact(mapboxTokenName);
    }

    return _mapboxTokenFact;
}

Fact* AppSettings::esriToken(void)
{
    if (!_esriTokenFact) {
        _esriTokenFact = _createSettingsFact(esriTokenName);
    }

    return _esriTokenFact;
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

Fact* AppSettings::defaultFirmwareType(void)
{
    if (!_defaultFirmwareTypeFact) {
        _defaultFirmwareTypeFact = _createSettingsFact(defaultFirmwareTypeName);
    }

    return _defaultFirmwareTypeFact;
}

Fact* AppSettings::followTarget(void)
{
    if (!_followTargetFact) {
        _followTargetFact = _createSettingsFact(followTargetName);
    }

    return _followTargetFact;
}

