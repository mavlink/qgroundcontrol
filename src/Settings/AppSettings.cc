/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AppSettings.h"
#include "QGCPalette.h"
#include "QGCApplication.h"
#include "QGCMAVLink.h"
#include "LinkManager.h"

#ifdef Q_OS_ANDROID
#include "AndroidInterface.h"
#endif

#include <QtQml/QQmlEngine>
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtCore/QSettings>

// Release languages are 90%+ complete
QList<QLocale::Language> AppSettings::_rgReleaseLanguages = {
    QLocale::English,
    QLocale::Azerbaijani,    
    QLocale::Chinese,
    QLocale::Japanese,
    QLocale::Korean,
    QLocale::Portuguese,
    QLocale::Russian,
};

// Partial languages are 40%+ complete
QList<QLocale::Language> AppSettings::_rgPartialLanguages = {
    QLocale::Ukrainian,
};

AppSettings::LanguageInfo_t AppSettings::_rgLanguageInfo[] = {
    { QLocale::AnyLanguage,     "System" },                     // Must be first
    { QLocale::Azerbaijani,     "Azerbaijani (Azerbaijani)" },
    { QLocale::Bulgarian,       "български (Bulgarian)" },
    { QLocale::Chinese,         "中文 (Chinese)" },
    { QLocale::Dutch,           "Nederlands (Dutch)" },
    { QLocale::English,         "English" },
    { QLocale::Finnish,         "Suomi (Finnish)" },
    { QLocale::French,          "Français (French)" },
    { QLocale::German,          "Deutsche (German)" },
    { QLocale::Greek,           "Ελληνικά (Greek)" },
    { QLocale::Hebrew,          "עברית (Hebrew)" },
    { QLocale::Italian,         "Italiano (Italian)" },
    { QLocale::Japanese,        "日本語 (Japanese)" },
    { QLocale::Korean,          "한국어 (Korean)" },
    { QLocale::NorwegianBokmal, "Norsk (Norwegian)" },
    { QLocale::Polish,          "Polskie (Polish)" },
    { QLocale::Portuguese,      "Português (Portuguese)" },
    { QLocale::Russian,         "Pусский (Russian)" },
    { QLocale::Spanish,         "Español (Spanish)" },
    { QLocale::Swedish,         "Svenska (Swedish)" },
    { QLocale::Turkish,         "Türk (Turkish)" }
};

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
    QString appName = QCoreApplication::applicationName();
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    // Mobile builds always use the runtime generated location for savePath.
    bool userHasModifiedSavePath = false;
#else
    bool userHasModifiedSavePath = !savePathFact->rawValue().toString().isEmpty() || !_nameToMetaDataMap[savePathName]->rawDefaultValue().toString().isEmpty();
#endif

    if (!userHasModifiedSavePath) {
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    #ifdef Q_OS_IOS
        // This will expose the directories directly to the File iOs app
        QDir rootDir = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        savePathFact->setRawValue(rootDir.absolutePath());
    #else
        QString rootDirPath;
        #ifdef Q_OS_ANDROID
        if (androidSaveToSDCard()->rawValue().toBool()) {
                rootDirPath = AndroidInterface::getSDCardPath();
            qDebug() << "AndroidInterface::getSDCardPath();" << rootDirPath;
                if (rootDirPath.isEmpty() || !QDir(rootDirPath).exists()) {
                    rootDirPath.clear();
                    qgcApp()->showAppMessage(AppSettings::tr("Save to SD card specified for application data. But no SD card present. Using internal storage."));
                } else if (!QFileInfo(rootDirPath).isWritable()) {
                    rootDirPath.clear();
                    qgcApp()->showAppMessage(AppSettings::tr("Save to SD card specified for application data. But SD card is write protected. Using internal storage."));
                }
            }
        #endif
        if (rootDirPath.isEmpty()) {
            rootDirPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
        }
        savePathFact->setRawValue(QDir(rootDirPath).filePath(appName));
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
}

DECLARE_SETTINGSFACT(AppSettings, offlineEditingFirmwareClass)
DECLARE_SETTINGSFACT(AppSettings, offlineEditingVehicleClass)
DECLARE_SETTINGSFACT(AppSettings, offlineEditingCruiseSpeed)
DECLARE_SETTINGSFACT(AppSettings, offlineEditingHoverSpeed)
DECLARE_SETTINGSFACT(AppSettings, offlineEditingAscentSpeed)
DECLARE_SETTINGSFACT(AppSettings, offlineEditingDescentSpeed)
DECLARE_SETTINGSFACT(AppSettings, batteryPercentRemainingAnnounce)
DECLARE_SETTINGSFACT(AppSettings, defaultMissionItemAltitude)
DECLARE_SETTINGSFACT(AppSettings, audioMuted)
DECLARE_SETTINGSFACT(AppSettings, virtualJoystick)
DECLARE_SETTINGSFACT(AppSettings, virtualJoystickAutoCenterThrottle)
DECLARE_SETTINGSFACT(AppSettings, appFontPointSize)
DECLARE_SETTINGSFACT(AppSettings, savePath)
DECLARE_SETTINGSFACT(AppSettings, androidSaveToSDCard)
DECLARE_SETTINGSFACT(AppSettings, useChecklist)
DECLARE_SETTINGSFACT(AppSettings, enforceChecklist)
DECLARE_SETTINGSFACT(AppSettings, enableMultiVehiclePanel)
DECLARE_SETTINGSFACT(AppSettings, mapboxToken)
DECLARE_SETTINGSFACT(AppSettings, mapboxAccount)
DECLARE_SETTINGSFACT(AppSettings, mapboxStyle)
DECLARE_SETTINGSFACT(AppSettings, esriToken)
DECLARE_SETTINGSFACT(AppSettings, customURL)
DECLARE_SETTINGSFACT(AppSettings, vworldToken)
DECLARE_SETTINGSFACT(AppSettings, gstDebugLevel)
DECLARE_SETTINGSFACT(AppSettings, followTarget)
DECLARE_SETTINGSFACT(AppSettings, disableAllPersistence)
DECLARE_SETTINGSFACT(AppSettings, firstRunPromptIdsShown)
DECLARE_SETTINGSFACT(AppSettings, loginAirLink)
DECLARE_SETTINGSFACT(AppSettings, passAirLink)

DECLARE_SETTINGSFACT_NO_FUNC(AppSettings, indoorPalette)
{
    if (!_indoorPaletteFact) {
        _indoorPaletteFact = _createSettingsFact(indoorPaletteName);
        connect(_indoorPaletteFact, &Fact::rawValueChanged, this, &AppSettings::_indoorPaletteChanged);
    }
    return _indoorPaletteFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(AppSettings, qLocaleLanguage)
{
    if (!_qLocaleLanguageFact) {
        _qLocaleLanguageFact = _createSettingsFact(qLocaleLanguageName);
        connect(_qLocaleLanguageFact, &Fact::rawValueChanged, this, &AppSettings::_qLocaleLanguageChanged);

        FactMetaData*   metaData            = _qLocaleLanguageFact->metaData();
        QStringList     rgEnumStrings;
        QVariantList    rgEnumValues;

        // System is always an available selection
        rgEnumStrings.append(_rgLanguageInfo[0].languageName);
        rgEnumValues.append(_rgLanguageInfo[0].languageId);

        for (const auto& languageInfo: _rgLanguageInfo) {
            if (_rgReleaseLanguages.contains(languageInfo.languageId)) {
                rgEnumStrings.append(languageInfo.languageName);
                rgEnumValues.append(languageInfo.languageId);
            }
        }
        for (const auto& languageInfo: _rgLanguageInfo) {
            if (_rgPartialLanguages.contains(languageInfo.languageId)) {
                rgEnumStrings.append(QString(languageInfo.languageName) + AppSettings::tr(" (Partial)"));
                rgEnumValues.append(languageInfo.languageId);
            }
        }
#ifdef QGC_DAILY_BUILD
        // Only daily builds include full set of languages for testing purposes
        for (const auto& languageInfo: _rgLanguageInfo) {
            if (!_rgReleaseLanguages.contains(languageInfo.languageId) && !_rgPartialLanguages.contains(languageInfo.languageId)) {
                rgEnumStrings.append(QString(languageInfo.languageName) + AppSettings::tr(" (Test Only)"));
                rgEnumValues.append(languageInfo.languageId);
            }
        }
#endif
        metaData->setEnumInfo(rgEnumStrings, rgEnumValues);

        if (_qLocaleLanguageFact->enumIndex() == -1) {
            _qLocaleLanguageFact->setRawValue(QLocale::AnyLanguage);
        }
    }
    return _qLocaleLanguageFact;
}

void AppSettings::_qLocaleLanguageChanged()
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
        savePathDir.mkdir(mavlinkActionsDirectory);
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

QString AppSettings::mavlinkActionsSavePath(void)
{
    QString path = savePath()->rawValue().toString();
    if (!path.isEmpty() && QDir(path).exists()) {
        QDir dir(path);
        return dir.filePath(mavlinkActionsDirectory);
    }
    return QString();
}

QList<int> AppSettings::firstRunPromptsIdsVariantToList(const QVariant& firstRunPromptIds)
{
    QList<int> rgIds;

    QStringList strIdList = firstRunPromptIds.toString().split(",", Qt::SkipEmptyParts);

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

/// Returns the current qLocaleLanguage setting bypassing the standard SettingsGroup path. It also validates
/// that the value is a supported language. This should only be used by QGCApplication::setLanguage to query 
/// the language setting as early in the boot process as possible. Specfically prior to any JSON files being 
/// loaded such that JSON file can be translated. Also since this is a one-off mechanism custom build overrides 
/// for language are not currently supported.
QLocale::Language AppSettings::_qLocaleLanguageEarlyAccess(void)
{
    QSettings settings;

    // Note that the AppSettings group has no group name
    QLocale::Language localeLanguage = static_cast<QLocale::Language>(settings.value(qLocaleLanguageName).toInt());
    for (auto& languageInfo: _rgLanguageInfo) {
        if (languageInfo.languageId == localeLanguage) {
            return localeLanguage;
        }
    }

    localeLanguage = QLocale::AnyLanguage;
    settings.setValue(qLocaleLanguageName, localeLanguage);

    return localeLanguage;
}
