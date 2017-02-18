/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef SettingsManager_H
#define SettingsManager_H

#include "QGCLoggingCategory.h"
#include "Joystick.h"
#include "MultiVehicleManager.h"
#include "QGCToolbox.h"

#include <QVariantList>

Q_DECLARE_LOGGING_CATEGORY(SettingsManagerLog)

/// Provides access to all app settings
class SettingsManager : public QGCTool
{
    Q_OBJECT
    
public:
    SettingsManager(QGCApplication* app);

    enum DistanceUnits {
        DistanceUnitsFeet = 0,
        DistanceUnitsMeters
    };

    enum AreaUnits {
        AreaUnitsSquareFeet = 0,
        AreaUnitsSquareMeters,
        AreaUnitsSquareKilometers,
        AreaUnitsHectares,
        AreaUnitsAcres,
        AreaUnitsSquareMiles,
    };

    enum SpeedUnits {
        SpeedUnitsFeetPerSecond = 0,
        SpeedUnitsMetersPerSecond,
        SpeedUnitsMilesPerHour,
        SpeedUnitsKilometersPerHour,
        SpeedUnitsKnots,
    };

    Q_ENUMS(DistanceUnits)
    Q_ENUMS(AreaUnits)
    Q_ENUMS(SpeedUnits)

    Q_PROPERTY(Fact*    offlineEditingFirmwareType      READ offlineEditingFirmwareType         CONSTANT)
    Q_PROPERTY(Fact*    offlineEditingVehicleType       READ offlineEditingVehicleType          CONSTANT)
    Q_PROPERTY(Fact*    offlineEditingCruiseSpeed       READ offlineEditingCruiseSpeed          CONSTANT)
    Q_PROPERTY(Fact*    offlineEditingHoverSpeed        READ offlineEditingHoverSpeed           CONSTANT)
    Q_PROPERTY(Fact*    distanceUnits                   READ distanceUnits                      CONSTANT)
    Q_PROPERTY(Fact*    areaUnits                       READ areaUnits                          CONSTANT)
    Q_PROPERTY(Fact*    speedUnits                      READ speedUnits                         CONSTANT)
    Q_PROPERTY(Fact*    batteryPercentRemainingAnnounce READ batteryPercentRemainingAnnounce    CONSTANT)
    Q_PROPERTY(Fact*    defaultMissionItemAltitude      READ defaultMissionItemAltitude         CONSTANT)

    Fact* offlineEditingFirmwareType     (void);
    Fact* offlineEditingVehicleType      (void);
    Fact* offlineEditingCruiseSpeed      (void);
    Fact* offlineEditingHoverSpeed       (void);
    Fact* distanceUnits                  (void);
    Fact* areaUnits                      (void);
    Fact* speedUnits                     (void);
    Fact* batteryPercentRemainingAnnounce(void);
    Fact* defaultMissionItemAltitude     (void);

    // Override from QGCTool
    virtual void setToolbox(QGCToolbox *toolbox);

    static const char* offlineEditingFirmwareTypeSettingsName;
    static const char* offlineEditingVehicleTypeSettingsName;
    static const char* offlineEditingCruiseSpeedSettingsName;
    static const char* offlineEditingHoverSpeedSettingsName;
    static const char* distanceUnitsSettingsName;
    static const char* areaUnitsSettingsName;
    static const char* speedUnitsSettingsName;
    static const char* batteryPercentRemainingAnnounceSettingsName;
    static const char* defaultMissionItemAltitudeSettingsName;

public slots:

signals:

private slots:
    
private:
    SettingsFact* _createSettingsFact(const QString& name);

    QMap<QString, FactMetaData*> _nameToMetaDataMap;

    SettingsFact* _offlineEditingFirmwareTypeFact;
    SettingsFact* _offlineEditingVehicleTypeFact;
    SettingsFact* _offlineEditingCruiseSpeedFact;
    SettingsFact* _offlineEditingHoverSpeedFact;
    SettingsFact* _distanceUnitsFact;
    SettingsFact* _areaUnitsFact;
    SettingsFact* _speedUnitsFact;
    SettingsFact* _batteryPercentRemainingAnnounceFact;
    SettingsFact* _defaultMissionItemAltitudeFact;
};

#endif
