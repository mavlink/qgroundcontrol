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

#include <QDir>

#define APP_SETTINGS_DEFINE_FACT(name) \
public: \
    Q_PROPERTY(Fact* name READ name CONSTANT) \
    Fact* name() \
    { \
        if (!_ ## name ## Fact) { \
            _ ## name ## Fact = _createSettingsFact(name ## Name); \
        } \
        return _ ## name ## Fact; \
    }; \
    static const char* name ## Name; \
private: \
    SettingsFact* _ ## name ## Fact = nullptr;

#define APP_SETTINGS_DEFINE_PATH(name) \
public: \
    Q_PROPERTY(QString name ## SavePath READ name ## SavePath NOTIFY savePathsChanged) \
    QString name ## SavePath() \
    { \
        QString path = savePath()->rawValue().toString(); \
        if (!path.isEmpty() && QDir(path).exists()) { \
            QDir dir(path); \
            return dir.filePath(name ## Directory); \
        } \
        return QString(); \
    }; \
    static const char* name ## Directory; \
private:

#define APP_SETTINGS_DEFINE_EXTENSION(name) \
public: \
    Q_PROPERTY(QString name ## FileExtension MEMBER name ## FileExtension CONSTANT)\
    static const char* name ## FileExtension;

class AppSettings : public SettingsGroup
{
    Q_OBJECT

public:
    AppSettings(QObject* parent = NULL);

    // Create Facts
    APP_SETTINGS_DEFINE_FACT(offlineEditingFirmwareType)
    APP_SETTINGS_DEFINE_FACT(offlineEditingVehicleType)
    APP_SETTINGS_DEFINE_FACT(offlineEditingCruiseSpeed)
    APP_SETTINGS_DEFINE_FACT(offlineEditingHoverSpeed)
    APP_SETTINGS_DEFINE_FACT(offlineEditingAscentSpeed)
    APP_SETTINGS_DEFINE_FACT(offlineEditingDescentSpeed)
    APP_SETTINGS_DEFINE_FACT(batteryPercentRemainingAnnounce)
    APP_SETTINGS_DEFINE_FACT(defaultMissionItemAltitude)
    APP_SETTINGS_DEFINE_FACT(telemetrySave)
    APP_SETTINGS_DEFINE_FACT(telemetrySaveNotArmed)
    APP_SETTINGS_DEFINE_FACT(audioMuted)
    APP_SETTINGS_DEFINE_FACT(virtualJoystick)
    APP_SETTINGS_DEFINE_FACT(appFontPointSize)
    APP_SETTINGS_DEFINE_FACT(indoorPalette)
    APP_SETTINGS_DEFINE_FACT(showLargeCompass)
    APP_SETTINGS_DEFINE_FACT(savePath)
    APP_SETTINGS_DEFINE_FACT(autoLoadMissions)
    APP_SETTINGS_DEFINE_FACT(useChecklist)
    APP_SETTINGS_DEFINE_FACT(mapboxToken)
    APP_SETTINGS_DEFINE_FACT(esriToken)
    APP_SETTINGS_DEFINE_FACT(defaultFirmwareType)
    APP_SETTINGS_DEFINE_FACT(gstDebug)
    APP_SETTINGS_DEFINE_FACT(followTarget)

    // Child directories of savePath for specific file types
    APP_SETTINGS_DEFINE_PATH(mission)
    APP_SETTINGS_DEFINE_PATH(parameter)
    APP_SETTINGS_DEFINE_PATH(telemetry)
    APP_SETTINGS_DEFINE_PATH(log)
    APP_SETTINGS_DEFINE_PATH(video)
    APP_SETTINGS_DEFINE_PATH(crash)

    // Application wide file extensions
    APP_SETTINGS_DEFINE_EXTENSION(plan)
    APP_SETTINGS_DEFINE_EXTENSION(mission)
    APP_SETTINGS_DEFINE_EXTENSION(waypoints)
    APP_SETTINGS_DEFINE_EXTENSION(parameter)
    APP_SETTINGS_DEFINE_EXTENSION(telemetry)
    APP_SETTINGS_DEFINE_EXTENSION(kml)
    APP_SETTINGS_DEFINE_EXTENSION(log)

public:

    static MAV_AUTOPILOT offlineEditingFirmwareTypeFromFirmwareType(MAV_AUTOPILOT firmwareType);
    static MAV_TYPE offlineEditingVehicleTypeFromVehicleType(MAV_TYPE vehicleType);

    static const char* name;
    static const char* settingsGroup;

signals:
    void savePathsChanged(void);

private slots:
    void _indoorPaletteChanged(void);
    void _checkSavePathDirectories(void);
};

#endif
