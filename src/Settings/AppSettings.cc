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

const char* AppSettings::appSettingsGroupName =                         "App";
const char* AppSettings::offlineEditingFirmwareTypeSettingsName =       "OfflineEditingFirmwareType";
const char* AppSettings::offlineEditingVehicleTypeSettingsName =        "OfflineEditingVehicleType";
const char* AppSettings::offlineEditingCruiseSpeedSettingsName =        "OfflineEditingCruiseSpeed";
const char* AppSettings::offlineEditingHoverSpeedSettingsName =         "OfflineEditingHoverSpeed";
const char* AppSettings::batteryPercentRemainingAnnounceSettingsName =  "batteryPercentRemainingAnnounce";
const char* AppSettings::defaultMissionItemAltitudeSettingsName =       "DefaultMissionItemAltitude";
const char* AppSettings::telemetrySaveName =                            "PromptFLightDataSave";
const char* AppSettings::telemetrySaveNotArmedName =                    "PromptFLightDataSaveNotArmed";
const char* AppSettings::audioMutedName =                               "AudioMuted";
const char* AppSettings::virtualJoystickName =                          "VirtualTabletJoystick";
const char* AppSettings::appFontPointSizeName =                         "BaseDeviceFontPointSize";
const char* AppSettings::indoorPaletteName =                            "StyleIsDark";
const char* AppSettings::showLargeCompassName =                         "ShowLargeCompass";
const char* AppSettings::savePathName =                                 "SavePath";
const char* AppSettings::autoLoadMissionsName =                         "AutoLoadMissions";
const char* AppSettings::automaticMissionUploadName =                   "AutomaticMissionUpload";

const char* AppSettings::parameterFileExtension =   "params";
const char* AppSettings::planFileExtension =        "plan";
const char* AppSettings::missionFileExtension =     "mission";
const char* AppSettings::waypointsFileExtension =   "waypoints";
const char* AppSettings::fenceFileExtension =       "fence";
const char* AppSettings::rallyPointFileExtension =  "rally";
const char* AppSettings::telemetryFileExtension =   "tlog";

const char* AppSettings::parameterDirectory =   "Parameters";
const char* AppSettings::telemetryDirectory =   "Telemetry";
const char* AppSettings::missionDirectory =     "Missions";

AppSettings::AppSettings(QObject* parent)
    : SettingsGroup(appSettingsGroupName, QString() /* root settings group */, parent)
    , _offlineEditingFirmwareTypeFact(NULL)
    , _offlineEditingVehicleTypeFact(NULL)
    , _offlineEditingCruiseSpeedFact(NULL)
    , _offlineEditingHoverSpeedFact(NULL)
    , _batteryPercentRemainingAnnounceFact(NULL)
    , _defaultMissionItemAltitudeFact(NULL)
    , _telemetrySaveFact(NULL)
    , _telemetrySaveNotArmedFact(NULL)
    , _audioMutedFact(NULL)
    , _virtualJoystickFact(NULL)
    , _appFontPointSizeFact(NULL)
    , _indoorPaletteFact(NULL)
    , _showLargeCompassFact(NULL)
    , _savePathFact(NULL)
    , _autoLoadMissionsFact(NULL)
    , _automaticMissionUpload(NULL)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    qmlRegisterUncreatableType<AppSettings>("QGroundControl.SettingsManager", 1, 0, "AppSettings", "Reference only");
    QGCPalette::setGlobalTheme(indoorPalette()->rawValue().toBool() ? QGCPalette::Dark : QGCPalette::Light);

    // Instantiate savePath so we can check for override and setup default path if needed

    SettingsFact* savePathFact = qobject_cast<SettingsFact*>(savePath());
    QString appName = qgcApp()->applicationName();
    if (savePathFact->rawValue().toString().isEmpty() && _nameToMetaDataMap[savePathName]->rawDefaultValue().toString().isEmpty()) {
#ifdef __mobile__
        QDir rootDir = QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation));
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
    QString fullPath;

    QString path = savePath()->rawValue().toString();
    if (!path.isEmpty() && QDir(path).exists()) {
        QDir dir(path);
        return dir.filePath(missionDirectory);
    }

    return fullPath;
}

QString AppSettings::parameterSavePath(void)
{
    QString fullPath;

    QString path = savePath()->rawValue().toString();
    if (!path.isEmpty() && QDir(path).exists()) {
        QDir dir(path);
        return dir.filePath(parameterDirectory);
    }

    return fullPath;
}

QString AppSettings::telemetrySavePath(void)
{
    QString fullPath;

    QString path = savePath()->rawValue().toString();
    if (!path.isEmpty() && QDir(path).exists()) {
        QDir dir(path);
        return dir.filePath(telemetryDirectory);
    }

    return fullPath;
}

Fact* AppSettings::autoLoadMissions(void)
{
    if (!_autoLoadMissionsFact) {
        _autoLoadMissionsFact = _createSettingsFact(autoLoadMissionsName);
    }

    return _autoLoadMissionsFact;
}

Fact* AppSettings::automaticMissionUpload(void)
{
    if (!_automaticMissionUpload) {
        _automaticMissionUpload = _createSettingsFact(automaticMissionUploadName);
    }

    return _automaticMissionUpload;
}

