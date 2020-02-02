/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "UnitsSettings.h"

#include <QQmlEngine>
#include <QtQml>

DECLARE_SETTINGGROUP(Units, "Units")
{
    qmlRegisterUncreatableType<UnitsSettings>("QGroundControl.SettingsManager", 1, 0, "UnitsSettings", "Reference only");
}

DECLARE_SETTINGSFACT_NO_FUNC(UnitsSettings, distanceUnits)
{
    if (!_distanceUnitsFact) {
        // Distance/Area/Speed units settings can't be loaded from json since it creates an infinite loop of meta data loading.
        QStringList     enumStrings;
        QVariantList    enumValues;
        enumStrings << "Feet" << "Meters";
        enumValues << QVariant::fromValue(static_cast<uint32_t>(DistanceUnitsFeet)) << QVariant::fromValue(static_cast<uint32_t>(DistanceUnitsMeters));
        FactMetaData* metaData = new FactMetaData(FactMetaData::valueTypeUint32, this);
        metaData->setName(distanceUnitsName);
        metaData->setShortDescription("Distance units");
        metaData->setEnumInfo(enumStrings, enumValues);
        metaData->setRawDefaultValue(DistanceUnitsMeters);
        metaData->setQGCRebootRequired(true);
        _distanceUnitsFact = new SettingsFact(_settingsGroup, metaData, this);
    }
    return _distanceUnitsFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(UnitsSettings, areaUnits)
{
    if (!_areaUnitsFact) {
        // Distance/Area/Speed units settings can't be loaded from json since it creates an infinite loop of meta data loading.
        QStringList     enumStrings;
        QVariantList    enumValues;
        enumStrings << "SquareFeet" << "SquareMeters" << "SquareKilometers" << "Hectares" << "Acres" << "SquareMiles";
        enumValues <<
            QVariant::fromValue(static_cast<uint32_t>(AreaUnitsSquareFeet)) <<
            QVariant::fromValue(static_cast<uint32_t>(AreaUnitsSquareMeters)) <<
            QVariant::fromValue(static_cast<uint32_t>(AreaUnitsSquareKilometers)) <<
            QVariant::fromValue(static_cast<uint32_t>(AreaUnitsHectares)) <<
            QVariant::fromValue(static_cast<uint32_t>(AreaUnitsAcres)) <<
            QVariant::fromValue(static_cast<uint32_t>(AreaUnitsSquareMiles));
        FactMetaData* metaData = new FactMetaData(FactMetaData::valueTypeUint32, this);
        metaData->setName(areaUnitsName);
        metaData->setShortDescription("Area units");
        metaData->setEnumInfo(enumStrings, enumValues);
        metaData->setRawDefaultValue(AreaUnitsSquareMeters);
        metaData->setQGCRebootRequired(true);
        _areaUnitsFact = new SettingsFact(_settingsGroup, metaData, this);
    }
    return _areaUnitsFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(UnitsSettings, speedUnits)
{
    if (!_speedUnitsFact) {
        // Distance/Area/Speed units settings can't be loaded from json since it creates an infinite loop of meta data loading.
        QStringList     enumStrings;
        QVariantList    enumValues;
        enumStrings << "Feet/second" << "Meters/second" << "Miles/hour" << "Kilometers/hour" << "Knots";
        enumValues <<
            QVariant::fromValue(static_cast<uint32_t>(SpeedUnitsFeetPerSecond)) <<
            QVariant::fromValue(static_cast<uint32_t>(SpeedUnitsMetersPerSecond)) <<
            QVariant::fromValue(static_cast<uint32_t>(SpeedUnitsMilesPerHour)) <<
            QVariant::fromValue(static_cast<uint32_t>(SpeedUnitsKilometersPerHour)) <<
            QVariant::fromValue(static_cast<uint32_t>(SpeedUnitsKnots));
        FactMetaData* metaData = new FactMetaData(FactMetaData::valueTypeUint32, this);
        metaData->setName(speedUnitsName);
        metaData->setShortDescription("Speed units");
        metaData->setEnumInfo(enumStrings, enumValues);
        metaData->setRawDefaultValue(SpeedUnitsMetersPerSecond);
        metaData->setQGCRebootRequired(true);
        _speedUnitsFact = new SettingsFact(_settingsGroup, metaData, this);
    }
    return _speedUnitsFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(UnitsSettings, temperatureUnits)
{
    if (!_temperatureUnitsFact) {
        // Units settings can't be loaded from json since it creates an infinite loop of meta data loading.
        QStringList     enumStrings;
        QVariantList    enumValues;
        enumStrings << "Celsius" << "Farenheit";
        enumValues << QVariant::fromValue(static_cast<uint32_t>(TemperatureUnitsCelsius)) << QVariant::fromValue(static_cast<uint32_t>(TemperatureUnitsFarenheit));
        FactMetaData* metaData = new FactMetaData(FactMetaData::valueTypeUint32, this);
        metaData->setName(temperatureUnitsName);
        metaData->setShortDescription("Temperature units");
        metaData->setEnumInfo(enumStrings, enumValues);
        metaData->setRawDefaultValue(TemperatureUnitsCelsius);
        metaData->setQGCRebootRequired(true);
        _temperatureUnitsFact = new SettingsFact(_settingsGroup, metaData, this);
    }
    return _temperatureUnitsFact;
}
