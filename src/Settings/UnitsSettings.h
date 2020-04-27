/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
    UnitsSettings(QObject* parent = nullptr);

    enum DistanceUnits {
        DistanceUnitsFeet = 0,
        DistanceUnitsMeters
    };

    enum AltitudeUnits {
        AltitudeUnitsFeet = 0,
        AltitudeUnitsMeters
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

    enum WeightUnits {
        WeightUnitsGrams = 0,
        WeightUnitsKg,
        WeightUnitsOz,
        WeightUnitsLbs
    };

    Q_ENUM(DistanceUnits)
    Q_ENUM(AreaUnits)
    Q_ENUM(AltitudeUnits)
    Q_ENUM(SpeedUnits)
    Q_ENUM(TemperatureUnits)
    Q_ENUM(WeightUnits)

    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(distanceUnits)
    DEFINE_SETTINGFACT(altitudeUnits)
    DEFINE_SETTINGFACT(areaUnits)
    DEFINE_SETTINGFACT(speedUnits)
    DEFINE_SETTINGFACT(temperatureUnits)
    DEFINE_SETTINGFACT(weightUnits)
};

#endif
