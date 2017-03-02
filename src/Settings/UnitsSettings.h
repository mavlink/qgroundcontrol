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

    Q_ENUMS(DistanceUnits)
    Q_ENUMS(AreaUnits)
    Q_ENUMS(SpeedUnits)

    Q_PROPERTY(Fact* distanceUnits                      READ distanceUnits                      CONSTANT)
    Q_PROPERTY(Fact* areaUnits                          READ areaUnits                          CONSTANT)
    Q_PROPERTY(Fact* speedUnits                         READ speedUnits                         CONSTANT)

    Fact* distanceUnits                     (void);
    Fact* areaUnits                         (void);
    Fact* speedUnits                        (void);

    static const char* unitsSettingsGroupName;

    static const char* distanceUnitsSettingsName;
    static const char* areaUnitsSettingsName;
    static const char* speedUnitsSettingsName;

private:
    SettingsFact* _distanceUnitsFact;
    SettingsFact* _areaUnitsFact;
    SettingsFact* _speedUnitsFact;
};

#endif
