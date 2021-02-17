/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AppSettings.h"
#include "QGCPalette.h"
#include "QGCApplication.h"
#include "ParameterManager.h"

#include <QQmlEngine>
#include <QtQml>
#include <QStandardPaths>

const char* AppSettings::parameterFileExtension =   "params";
const char* AppSettings::planFileExtension =        "plan";
const char* AppSettings::missionFileExtension =     "mission";
const char* AppSettings::waypointsFileExtension =   "waypoints";
const char* AppSettings::fenceFileExtension =       "fence";
const char* AppSettings::rallyPointFileExtension =  "rally";
const char* AppSettings::telemetryFileExtension =   "tlog";
const char* AppSettings::kmlFileExtension =         "kml";
const char* AppSettings::shpFileExtension =         "shp";
const char* AppSettings::logFileExtension =         "ulg";

const char* AppSettings::parameterDirectory =       QT_TRANSLATE_NOOP("AppSettings", "Parameters");
const char* AppSettings::telemetryDirectory =       QT_TRANSLATE_NOOP("AppSettings", "Telemetry");
const char* AppSettings::missionDirectory =         QT_TRANSLATE_NOOP("AppSettings", "Missions");
const char* AppSettings::logDirectory =             QT_TRANSLATE_NOOP("AppSettings", "Logs");
const char* AppSettings::videoDirectory =           QT_TRANSLATE_NOOP("AppSettings", "Video");
const char* AppSettings::photoDirectory =           QT_TRANSLATE_NOOP("AppSettings", "Photo");
const char* AppSettings::crashDirectory =           QT_TRANSLATE_NOOP("AppSettings", "CrashLogs");

DECLARE_SETTINGGROUP(App, "")
{
    qmlRegisterUncreatableType<AppSettings>("QGroundControl.SettingsManager", 1, 0, "AppSettings", "Reference only");
    QGCPalette::setGlobalTheme(indoorPalette()->rawValue().toBool() ? QGCPalette::Dark : QGCPalette::Light);

    QSettings settings;

    // These two "type" keys were changed to "class" values
    static const char* deprecatedFirmwareTypeKey    = "offlineEditingFirmwareType";
    static const char* deprecatedVehicleTypeKey     = "offlineEditingVehicleType";
    if (settings.contains(deprecatedFirmwareTypeKey)) {
        settings.setValue(deprecatedFirmwareTypeKey, QGCMAVLink::firmwareClass(static_cast<MAV_AUTOPILOT>(settings.value(deprecatedFirmwareTypeKey).toInt())));
    }
    if (settings.contains(deprecatedVehicleTypeKey)) {
        settings.setValue(deprecatedVehicleTypeKey, QGCMAVLink::vehicleClass(static_cast<MAV_TYPE>(settings.value(deprecatedVehicleTypeKey).toInt())));
    }

    QStringList deprecatedKeyNames  = { "virtualJoystickCentralized",           "offlineEditingFirmwareType",   "offlineEditingVehicleType" };
    QStringList newKeyNames         = { "virtualJoystickAutoCenterThrottle",    "offlineEditingFirmwareClass",  "offlineEditingVehicleClass" };
    settings.beginGroup(_settingsGroup);
    for (int i=0; i<deprecatedKeyNames.count(); i++) {
        if (settings.contains(deprecatedKeyNames[i])) {
            settings.setValue(newKeyNames[i], settings.value(deprecatedKeyNames[i]));
            settings.remove(deprecatedKeyNames[i]);
        }
    }

    // Instantiate savePath so we can check for override and setup default path if needed

    SettingsFact* savePathFact = qobject_cast<SettingsFact*>(savePath());
    QString appName = qgcApp()->applicationName();
#ifdef __mobile__
    // Mobile builds always use the runtime generated location for savePath.
    bool userHasModifiedSavePath = false;
#else
    bool userHasModifiedSavePath = !savePathFact->rawValue().toString().isEmpty() || !_nameToMetaDataMap[savePathName]->rawDefaultValue().toString().isEmpty();
#endif

    if (!userHasModifiedSavePath) {
#ifdef __mobile__
    #ifdef __ios__
        // This will expose the directories directly to the File iOs app
        QDir rootDir = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        savePathFact->setRawValue(rootDir.absolutePath());
    #else
        QDir rootDir = QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation));
        savePathFact->setRawValue(rootDir.filePath(appName));
    #endif
        savePathFact->setVisible(false);
#else
        QDir rootDir = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        savePathFact->setRawValue(rootDir.filePath(appName));
#endif
    }

    connect(savePathFact, &Fact::rawValueChanged, this, &AppSettings::savePathsChanged);
    connect(savePathFact, &Fact::rawValueChanged, this, &AppSettings::_checkSavePathDirectories);

    _checkSavePathDirectories();
    //-- Keep track of language changes
    SettingsFact* languageFact = qobject_cast<SettingsFact*>(language());
    connect(languageFact, &Fact::rawValueChanged, this, &AppSettings::_languageChanged);
}

DECLARE_SETTINGSFACT(AppSettings, offlineEditingFirmwareClass)
DECLARE_SETTINGSFACT(AppSettings, offlineEditingVehicleClass)
DECLARE_SETTINGSFACT(AppSettings, offlineEditingCruiseSpeed)
DECLARE_SETTINGSFACT(AppSettings, offlineEditingHoverSpeed)
DECLARE_SETTINGSFACT(AppSettings, offlineEditingAscentSpeed)
DECLARE_SETTINGSFACT(AppSettings, offlineEditingDescentSpeed)
DECLARE_SETTINGSFACT(AppSettings, batteryPercentRemainingAnnounce)
DECLARE_SETTINGSFACT(AppSettings, defaultMissionItemAltitude)
DECLARE_SETTINGSFACT(AppSettings, telemetrySave)
DECLARE_SETTINGSFACT(AppSettings, telemetrySaveNotArmed)
DECLARE_SETTINGSFACT(AppSettings, audioMuted)
DECLARE_SETTINGSFACT(AppSettings, checkInternet)
DECLARE_SETTINGSFACT(AppSettings, virtualJoystick)
DECLARE_SETTINGSFACT(AppSettings, virtualJoystickAutoCenterThrottle)
DECLARE_SETTINGSFACT(AppSettings, appFontPointSize)
DECLARE_SETTINGSFACT(AppSettings, showLargeCompass)
DECLARE_SETTINGSFACT(AppSettings, savePath)
DECLARE_SETTINGSFACT(AppSettings, useChecklist)
DECLARE_SETTINGSFACT(AppSettings, enforceChecklist)
DECLARE_SETTINGSFACT(AppSettings, mapboxToken)
DECLARE_SETTINGSFACT(AppSettings, mapboxAccount)
DECLARE_SETTINGSFACT(AppSettings, mapboxStyle)
DECLARE_SETTINGSFACT(AppSettings, esriToken)
DECLARE_SETTINGSFACT(AppSettings, defaultFirmwareType)
DECLARE_SETTINGSFACT(AppSettings, gstDebugLevel)
DECLARE_SETTINGSFACT(AppSettings, followTarget)
DECLARE_SETTINGSFACT(AppSettings, apmStartMavlinkStreams)
DECLARE_SETTINGSFACT(AppSettings, enableTaisync)
DECLARE_SETTINGSFACT(AppSettings, enableTaisyncVideo)
DECLARE_SETTINGSFACT(AppSettings, enableMicrohard)
DECLARE_SETTINGSFACT(AppSettings, language)
DECLARE_SETTINGSFACT(AppSettings, disableAllPersistence)
DECLARE_SETTINGSFACT(AppSettings, usePairing)
DECLARE_SETTINGSFACT(AppSettings, saveCsvTelemetry)
DECLARE_SETTINGSFACT(AppSettings, firstRunPromptIdsShown)
DECLARE_SETTINGSFACT(AppSettings, forwardMavlink)
DECLARE_SETTINGSFACT(AppSettings, forwardMavlinkHostName)
DECLARE_SETTINGSFACT(AppSettings, useComponentInformationQuery)

DECLARE_SETTINGSFACT_NO_FUNC(AppSettings, indoorPalette)
{
    if (!_indoorPaletteFact) {
        _indoorPaletteFact = _createSettingsFact(indoorPaletteName);
        connect(_indoorPaletteFact, &Fact::rawValueChanged, this, &AppSettings::_indoorPaletteChanged);
    }
    return _indoorPaletteFact;
}

void AppSettings::_languageChanged()
{
    qgcApp()->setLanguage();
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
        savePathDir.mkdir(photoDirectory);
        savePathDir.mkdir(crashDirectory);
    }
}

void AppSettings::_indoorPaletteChanged(void)
{
    QGCPalette::setGlobalTheme(indoorPalette()->rawValue().toBool() ? QGCPalette::Dark : QGCPalette::Light);
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

QString AppSettings::photoSavePath(void)
{
    QString path = savePath()->rawValue().toString();
    if (!path.isEmpty() && QDir(path).exists()) {
        QDir dir(path);
        return dir.filePath(photoDirectory);
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

QList<int> AppSettings::firstRunPromptsIdsVariantToList(const QVariant& firstRunPromptIds)
{
    QList<int> rgIds;

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    QStringList strIdList = firstRunPromptIds.toString().split(",", QString::SkipEmptyParts);
#else
    QStringList strIdList = firstRunPromptIds.toString().split(",", Qt::SkipEmptyParts);
#endif

    for (const QString& strId: strIdList) {
        rgIds.append(strId.toInt());
    }
    return rgIds;
}

QVariant AppSettings::firstRunPromptsIdsListToVariant(const QList<int>& rgIds)
{
    QStringList strList;
    for (int id: rgIds) {
        strList.append(QString::number(id));
    }
    return QVariant(strList.join(","));
}

void AppSettings::firstRunPromptIdsMarkIdAsShown(int id)
{
    QList<int> rgIds = firstRunPromptsIdsVariantToList(firstRunPromptIdsShown()->rawValue());
    if (!rgIds.contains(id)) {
        rgIds.append(id);
        firstRunPromptIdsShown()->setRawValue(firstRunPromptsIdsListToVariant(rgIds));
    }
}

int AppSettings::_languageID(void)
{
    // Hack to provide language settings as early in the boot process as possible. Must be know
    // prior to loading any json files.
    QSettings settings;
    return settings.value("language", 0).toInt();
}
