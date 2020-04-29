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

    enum HorizontalDistanceUnits {
        HorizontalDistanceUnitsFeet = 0,
        HorizontalDistanceUnitsMeters
    };

    enum VerticalDistanceUnits {
        VerticalDistanceUnitsFeet = 0,
        VerticalDistanceUnitsMeters
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

    Q_ENUM(HorizontalDistanceUnits)
    Q_ENUM(VerticalDistanceUnits)
    Q_ENUM(AreaUnits)
    Q_ENUM(SpeedUnits)
    Q_ENUM(TemperatureUnits)
    Q_ENUM(WeightUnits)

    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(horizontalDistanceUnits)
    DEFINE_SETTINGFACT(verticalDistanceUnits)
    DEFINE_SETTINGFACT(areaUnits)
    DEFINE_SETTINGFACT(speedUnits)
    DEFINE_SETTINGFACT(temperatureUnits)
    DEFINE_SETTINGFACT(weightUnits)
};

#endif
