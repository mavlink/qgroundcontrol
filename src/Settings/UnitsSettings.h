/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef UnitsSettings_H
#define UnitsSettings_H

#include "SettingsGroup.h"

class UnitsSettings : public SettingsGroup
{
    Q_OBJECT
    
public:
    UnitsSettings(QObject* parent = NULL);

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

    enum TemperatureUnits {
        TemperatureUnitsCelsius = 0,
        TemperatureUnitsFarenheit,
    };

    Q_ENUM(DistanceUnits)
    Q_ENUM(AreaUnits)
    Q_ENUM(SpeedUnits)
    Q_ENUM(TemperatureUnits)

    Q_PROPERTY(Fact* distanceUnits                      READ distanceUnits                      CONSTANT)
    Q_PROPERTY(Fact* areaUnits                          READ areaUnits                          CONSTANT)
    Q_PROPERTY(Fact* speedUnits                         READ speedUnits                         CONSTANT)
    Q_PROPERTY(Fact* temperatureUnits                   READ temperatureUnits                   CONSTANT)

    Fact* distanceUnits                     (void);
    Fact* areaUnits                         (void);
    Fact* speedUnits                        (void);
    Fact* temperatureUnits                  (void);

    static const char* name;
    static const char* settingsGroup;

    static const char* distanceUnitsSettingsName;
    static const char* areaUnitsSettingsName;
    static const char* speedUnitsSettingsName;
    static const char* temperatureUnitsSettingsName;

private:
    SettingsFact* _distanceUnitsFact;
    SettingsFact* _areaUnitsFact;
    SettingsFact* _speedUnitsFact;
    SettingsFact* _temperatureUnitsFact;
};

#endif
