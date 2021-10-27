/***************_qgcTranslatorSourceCode***********************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/// @file
/// @brief Application Settings

#pragma once
#include <QTranslator>

#include "SettingsGroup.h"
#include "QGCMAVLink.h"

/// Application Settings
class AppSettings : public SettingsGroup
{
    Q_OBJECT

public:
    AppSettings(QObject* parent = nullptr);

    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(offlineEditingFirmwareClass)
    DEFINE_SETTINGFACT(offlineEditingVehicleClass)
    DEFINE_SETTINGFACT(offlineEditingCruiseSpeed)
    DEFINE_SETTINGFACT(offlineEditingHoverSpeed)
    DEFINE_SETTINGFACT(offlineEditingAscentSpeed)
    DEFINE_SETTINGFACT(offlineEditingDescentSpeed)
    DEFINE_SETTINGFACT(batteryPercentRemainingAnnounce) // Important: This is only used to calculate battery swaps
    DEFINE_SETTINGFACT(defaultMissionItemAltitude)
    DEFINE_SETTINGFACT(telemetrySave)
    DEFINE_SETTINGFACT(telemetrySaveNotArmed)
    DEFINE_SETTINGFACT(audioMuted)
    DEFINE_SETTINGFACT(checkInternet)
    DEFINE_SETTINGFACT(virtualJoystick)
    DEFINE_SETTINGFACT(virtualJoystickAutoCenterThrottle)
    DEFINE_SETTINGFACT(appFontPointSize)
    DEFINE_SETTINGFACT(indoorPalette)
    DEFINE_SETTINGFACT(showLargeCompass)
    DEFINE_SETTINGFACT(savePath)
    DEFINE_SETTINGFACT(useChecklist)
    DEFINE_SETTINGFACT(enforceChecklist)
    DEFINE_SETTINGFACT(mapboxToken)
    DEFINE_SETTINGFACT(mapboxAccount)
    DEFINE_SETTINGFACT(mapboxStyle)
    DEFINE_SETTINGFACT(esriToken)
    DEFINE_SETTINGFACT(customURL)
    DEFINE_SETTINGFACT(vworldToken)
    DEFINE_SETTINGFACT(defaultFirmwareType)
    DEFINE_SETTINGFACT(gstDebugLevel)
    DEFINE_SETTINGFACT(followTarget)
    DEFINE_SETTINGFACT(enableTaisync)
    DEFINE_SETTINGFACT(enableTaisyncVideo)
    DEFINE_SETTINGFACT(enableMicrohard)
    DEFINE_SETTINGFACT(qLocaleLanguage)
    DEFINE_SETTINGFACT(disableAllPersistence)
    DEFINE_SETTINGFACT(usePairing)
    DEFINE_SETTINGFACT(saveCsvTelemetry)
    DEFINE_SETTINGFACT(firstRunPromptIdsShown)
    DEFINE_SETTINGFACT(forwardMavlink)
    DEFINE_SETTINGFACT(forwardMavlinkHostName)


    // Although this is a global setting it only affects ArduPilot vehicle since PX4 automatically starts the stream from the vehicle side
    DEFINE_SETTINGFACT(apmStartMavlinkStreams)

    Q_PROPERTY(QString missionSavePath      READ missionSavePath    NOTIFY savePathsChanged)
    Q_PROPERTY(QString parameterSavePath    READ parameterSavePath  NOTIFY savePathsChanged)
    Q_PROPERTY(QString telemetrySavePath    READ telemetrySavePath  NOTIFY savePathsChanged)
    Q_PROPERTY(QString logSavePath          READ logSavePath        NOTIFY savePathsChanged)
    Q_PROPERTY(QString videoSavePath        READ videoSavePath      NOTIFY savePathsChanged)
    Q_PROPERTY(QString photoSavePath        READ photoSavePath      NOTIFY savePathsChanged)
    Q_PROPERTY(QString crashSavePath        READ crashSavePath      NOTIFY savePathsChanged)

    Q_PROPERTY(QString planFileExtension        MEMBER planFileExtension        CONSTANT)
    Q_PROPERTY(QString missionFileExtension     MEMBER missionFileExtension     CONSTANT)
    Q_PROPERTY(QString waypointsFileExtension   MEMBER waypointsFileExtension   CONSTANT)
    Q_PROPERTY(QString parameterFileExtension   MEMBER parameterFileExtension   CONSTANT)
    Q_PROPERTY(QString telemetryFileExtension   MEMBER telemetryFileExtension   CONSTANT)
    Q_PROPERTY(QString kmlFileExtension         MEMBER kmlFileExtension         CONSTANT)
    Q_PROPERTY(QString shpFileExtension         MEMBER shpFileExtension         CONSTANT)
    Q_PROPERTY(QString logFileExtension         MEMBER logFileExtension         CONSTANT)

    QString missionSavePath     ();
    QString parameterSavePath   ();
    QString telemetrySavePath   ();
    QString logSavePath         ();
    QString videoSavePath       ();
    QString photoSavePath       ();
    QString crashSavePath       ();

    // Helper methods for working with firstRunPromptIds QVariant settings string list
    static QList<int> firstRunPromptsIdsVariantToList   (const QVariant& firstRunPromptIds);
    static QVariant   firstRunPromptsIdsListToVariant   (const QList<int>& rgIds);
    Q_INVOKABLE void  firstRunPromptIdsMarkIdAsShown    (int id);

    // Application wide file extensions
    static const char* parameterFileExtension;
    static const char* planFileExtension;
    static const char* missionFileExtension;
    static const char* waypointsFileExtension;
    static const char* fenceFileExtension;
    static const char* rallyPointFileExtension;
    static const char* telemetryFileExtension;
    static const char* kmlFileExtension;
    static const char* shpFileExtension;
    static const char* logFileExtension;

    // Child directories of savePath for specific file types
    static const char* parameterDirectory;
    static const char* telemetryDirectory;
    static const char* missionDirectory;
    static const char* logDirectory;
    static const char* videoDirectory;
    static const char* photoDirectory;
    static const char* crashDirectory;

    // Returns the current qLocaleLanguage setting bypassing the standard SettingsGroup path. This should only be used
    // by QGCApplication::setLanguage to query the language setting as early in the boot process as possible.
    // Specfically prior to any JSON files being loaded such that JSON file can be translated. Also since this
    // is a one-off mechanism custom build overrides for language are not currently supported.
    static QLocale::Language _qLocaleLanguageID(void);

signals:
    void savePathsChanged();

private slots:
    void _indoorPaletteChanged();
    void _checkSavePathDirectories();
    void _qLocaleLanguageChanged();

private:
    static QList<int> _rgReleaseLanguages;
    static QList<int> _rgPartialLanguages;
};
