/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef AppSettings_H
#define AppSettings_H

#include "SettingsGroup.h"
#include "QGCMAVLink.h"

class AppSettings : public SettingsGroup
{
    Q_OBJECT
    
public:
    AppSettings(QObject* parent = NULL);

    Q_PROPERTY(Fact* offlineEditingFirmwareType         READ offlineEditingFirmwareType         CONSTANT)
    Q_PROPERTY(Fact* offlineEditingVehicleType          READ offlineEditingVehicleType          CONSTANT)
    Q_PROPERTY(Fact* offlineEditingCruiseSpeed          READ offlineEditingCruiseSpeed          CONSTANT)
    Q_PROPERTY(Fact* offlineEditingHoverSpeed           READ offlineEditingHoverSpeed           CONSTANT)
    Q_PROPERTY(Fact* offlineEditingAscentSpeed          READ offlineEditingAscentSpeed          CONSTANT)
    Q_PROPERTY(Fact* offlineEditingDescentSpeed         READ offlineEditingDescentSpeed         CONSTANT)
    Q_PROPERTY(Fact* batteryPercentRemainingAnnounce    READ batteryPercentRemainingAnnounce    CONSTANT)
    Q_PROPERTY(Fact* defaultMissionItemAltitude         READ defaultMissionItemAltitude         CONSTANT)
    Q_PROPERTY(Fact* telemetrySave                      READ telemetrySave                      CONSTANT)
    Q_PROPERTY(Fact* telemetrySaveNotArmed              READ telemetrySaveNotArmed              CONSTANT)
    Q_PROPERTY(Fact* audioMuted                         READ audioMuted                         CONSTANT)
    Q_PROPERTY(Fact* virtualJoystick                    READ virtualJoystick                    CONSTANT)
    Q_PROPERTY(Fact* appFontPointSize                   READ appFontPointSize                   CONSTANT)
    Q_PROPERTY(Fact* indoorPalette                      READ indoorPalette                      CONSTANT)
    Q_PROPERTY(Fact* showLargeCompass                   READ showLargeCompass                   CONSTANT)
    Q_PROPERTY(Fact* savePath                           READ savePath                           CONSTANT)
    Q_PROPERTY(Fact* autoLoadMissions                   READ autoLoadMissions                   CONSTANT)
    Q_PROPERTY(Fact* useChecklist                       READ useChecklist                       CONSTANT)
    Q_PROPERTY(Fact* mapboxToken                        READ mapboxToken                        CONSTANT)
    Q_PROPERTY(Fact* esriToken                          READ esriToken                          CONSTANT)
    Q_PROPERTY(Fact* defaultFirmwareType                READ defaultFirmwareType                CONSTANT)
    Q_PROPERTY(Fact* gstDebug                           READ gstDebug                           CONSTANT)
    Q_PROPERTY(Fact* followTarget                       READ followTarget                       CONSTANT)
    Q_PROPERTY(Fact* splitConcavePolygons               READ splitConcavePolygons               CONSTANT)

    Q_PROPERTY(QString missionSavePath      READ missionSavePath    NOTIFY savePathsChanged)
    Q_PROPERTY(QString parameterSavePath    READ parameterSavePath  NOTIFY savePathsChanged)
    Q_PROPERTY(QString telemetrySavePath    READ telemetrySavePath  NOTIFY savePathsChanged)
    Q_PROPERTY(QString logSavePath          READ logSavePath        NOTIFY savePathsChanged)
    Q_PROPERTY(QString videoSavePath        READ videoSavePath      NOTIFY savePathsChanged)
    Q_PROPERTY(QString crashSavePath        READ crashSavePath      NOTIFY savePathsChanged)

    Q_PROPERTY(QString planFileExtension        MEMBER planFileExtension        CONSTANT)
    Q_PROPERTY(QString missionFileExtension     MEMBER missionFileExtension     CONSTANT)
    Q_PROPERTY(QString waypointsFileExtension   MEMBER waypointsFileExtension   CONSTANT)
    Q_PROPERTY(QString parameterFileExtension   MEMBER parameterFileExtension   CONSTANT)
    Q_PROPERTY(QString telemetryFileExtension   MEMBER telemetryFileExtension   CONSTANT)
    Q_PROPERTY(QString kmlFileExtension         MEMBER kmlFileExtension         CONSTANT)
    Q_PROPERTY(QString logFileExtension         MEMBER logFileExtension         CONSTANT)

    Fact* offlineEditingFirmwareType        (void);
    Fact* offlineEditingVehicleType         (void);
    Fact* offlineEditingCruiseSpeed         (void);
    Fact* offlineEditingHoverSpeed          (void);
    Fact* offlineEditingAscentSpeed         (void);
    Fact* offlineEditingDescentSpeed        (void);
    Fact* batteryPercentRemainingAnnounce   (void);
    Fact* defaultMissionItemAltitude        (void);
    Fact* telemetrySave                     (void);
    Fact* telemetrySaveNotArmed             (void);
    Fact* audioMuted                        (void);
    Fact* virtualJoystick                   (void);
    Fact* appFontPointSize                  (void);
    Fact* indoorPalette                     (void);
    Fact* showLargeCompass                  (void);
    Fact* savePath                          (void);
    Fact* autoLoadMissions                  (void);
    Fact* useChecklist                      (void);
    Fact* mapboxToken                       (void);
    Fact* esriToken                         (void);
    Fact* defaultFirmwareType               (void);
    Fact* gstDebug                          (void);
    Fact* followTarget                      (void);
    Fact* splitConcavePolygons              (void);

    QString missionSavePath     (void);
    QString parameterSavePath   (void);
    QString telemetrySavePath   (void);
    QString logSavePath         (void);
    QString videoSavePath       (void);
    QString crashSavePath       (void);

    static MAV_AUTOPILOT offlineEditingFirmwareTypeFromFirmwareType(MAV_AUTOPILOT firmwareType);
    static MAV_TYPE offlineEditingVehicleTypeFromVehicleType(MAV_TYPE vehicleType);

    static const char* name;
    static const char* settingsGroup;

    static const char* offlineEditingFirmwareTypeSettingsName;
    static const char* offlineEditingVehicleTypeSettingsName;
    static const char* offlineEditingCruiseSpeedSettingsName;
    static const char* offlineEditingHoverSpeedSettingsName;
    static const char* offlineEditingAscentSpeedSettingsName;
    static const char* offlineEditingDescentSpeedSettingsName;
    static const char* batteryPercentRemainingAnnounceSettingsName;
    static const char* defaultMissionItemAltitudeSettingsName;
    static const char* telemetrySaveName;
    static const char* telemetrySaveNotArmedName;
    static const char* audioMutedName;
    static const char* virtualJoystickName;
    static const char* appFontPointSizeName;
    static const char* indoorPaletteName;
    static const char* showLargeCompassName;
    static const char* savePathName;
    static const char* autoLoadMissionsName;
    static const char* useChecklistName;
    static const char* mapboxTokenName;
    static const char* esriTokenName;
    static const char* defaultFirmwareTypeName;
    static const char* gstDebugName;
    static const char* followTargetName;
    static const char* splitConcavePolygonsName;

    // Application wide file extensions
    static const char* parameterFileExtension;
    static const char* planFileExtension;
    static const char* missionFileExtension;
    static const char* waypointsFileExtension;
    static const char* fenceFileExtension;
    static const char* rallyPointFileExtension;
    static const char* telemetryFileExtension;
    static const char* kmlFileExtension;
    static const char* logFileExtension;

    // Child directories of savePath for specific file types
    static const char* parameterDirectory;
    static const char* telemetryDirectory;
    static const char* missionDirectory;
    static const char* logDirectory;
    static const char* videoDirectory;
    static const char* crashDirectory;

signals:
    void savePathsChanged(void);

private slots:
    void _indoorPaletteChanged(void);
    void _checkSavePathDirectories(void);

private:
    SettingsFact* _offlineEditingFirmwareTypeFact;
    SettingsFact* _offlineEditingVehicleTypeFact;
    SettingsFact* _offlineEditingCruiseSpeedFact;
    SettingsFact* _offlineEditingHoverSpeedFact;
    SettingsFact* _offlineEditingAscentSpeedFact;
    SettingsFact* _offlineEditingDescentSpeedFact;
    SettingsFact* _batteryPercentRemainingAnnounceFact;
    SettingsFact* _defaultMissionItemAltitudeFact;
    SettingsFact* _telemetrySaveFact;
    SettingsFact* _telemetrySaveNotArmedFact;
    SettingsFact* _audioMutedFact;
    SettingsFact* _virtualJoystickFact;
    SettingsFact* _appFontPointSizeFact;
    SettingsFact* _indoorPaletteFact;
    SettingsFact* _showLargeCompassFact;
    SettingsFact* _savePathFact;
    SettingsFact* _autoLoadMissionsFact;
    SettingsFact* _useChecklistFact;
    SettingsFact* _mapboxTokenFact;
    SettingsFact* _esriTokenFact;
    SettingsFact* _defaultFirmwareTypeFact;
    SettingsFact* _gstDebugFact;
    SettingsFact* _followTargetFact;
    SettingsFact* _splitConcavePolygonsFact;
};

#endif
