/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "UnitsSettings.h"

#include <QQmlEngine>
#include <QtQml>

const char* UnitsSettings::name =                           "Units";
const char* UnitsSettings::settingsGroup =                  ""; // settings are in root group

const char* UnitsSettings::distanceUnitsSettingsName =      "DistanceUnits";
const char* UnitsSettings::areaUnitsSettingsName =          "AreaUnits";
const char* UnitsSettings::speedUnitsSettingsName =         "SpeedUnits";
const char* UnitsSettings::temperatureUnitsSettingsName =   "TemperatureUnits";

UnitsSettings::UnitsSettings(QObject* parent)
    : SettingsGroup(name, settingsGroup, parent)
    , _distanceUnitsFact(NULL)
    , _areaUnitsFact(NULL)
    , _speedUnitsFact(NULL)
    , _temperatureUnitsFact(NULL)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    qmlRegisterUncreatableType<UnitsSettings>("QGroundControl.SettingsManager", 1, 0, "UnitsSettings", "Reference only");
}

Fact* UnitsSettings::distanceUnits(void)
{
    if (!_distanceUnitsFact) {
        // Distance/Area/Speed units settings can't be loaded from json since it creates an infinite loop of meta data loading.
        QStringList     enumStrings;
        QVariantList    enumValues;

        enumStrings << "Feet" << "Meters";
        enumValues << QVariant::fromValue((uint32_t)DistanceUnitsFeet) << QVariant::fromValue((uint32_t)DistanceUnitsMeters);
        
        FactMetaData* metaData = new FactMetaData(FactMetaData::valueTypeUint32, this);
        metaData->setName(distanceUnitsSettingsName);
        metaData->setShortDescription(tr("Distance units"));
        metaData->setEnumInfo(enumStrings, enumValues);
        metaData->setRawDefaultValue(DistanceUnitsMeters);
        metaData->setQGCRebootRequired(true);

        _distanceUnitsFact = new SettingsFact(_settingsGroup, metaData, this);

    }

    return _distanceUnitsFact;

}

Fact* UnitsSettings::areaUnits(void)
{
    if (!_areaUnitsFact) {
        // Distance/Area/Speed units settings can't be loaded from json since it creates an infinite loop of meta data loading.
        QStringList     enumStrings;
        QVariantList    enumValues;

        enumStrings << "SquareFeet" << "SquareMeters" << "SquareKilometers" << "Hectares" << "Acres" << "SquareMiles";
        enumValues << QVariant::fromValue((uint32_t)AreaUnitsSquareFeet) << QVariant::fromValue((uint32_t)AreaUnitsSquareMeters) << QVariant::fromValue((uint32_t)AreaUnitsSquareKilometers) << QVariant::fromValue((uint32_t)AreaUnitsHectares) << QVariant::fromValue((uint32_t)AreaUnitsAcres) << QVariant::fromValue((uint32_t)AreaUnitsSquareMiles);

        FactMetaData* metaData = new FactMetaData(FactMetaData::valueTypeUint32, this);
        metaData->setName(areaUnitsSettingsName);
        metaData->setShortDescription(tr("Area units"));
        metaData->setEnumInfo(enumStrings, enumValues);
        metaData->setRawDefaultValue(AreaUnitsSquareMeters);
        metaData->setQGCRebootRequired(true);

        _areaUnitsFact = new SettingsFact(_settingsGroup, metaData, this);
    }

    return _areaUnitsFact;

}

Fact* UnitsSettings::speedUnits(void)
{
    if (!_speedUnitsFact) {
        // Distance/Area/Speed units settings can't be loaded from json since it creates an infinite loop of meta data loading.
        QStringList     enumStrings;
        QVariantList    enumValues;

        enumStrings << "Feet/second" << "Meters/second" << "Miles/hour" << "Kilometers/hour" << "Knots";
        enumValues << QVariant::fromValue((uint32_t)SpeedUnitsFeetPerSecond) << QVariant::fromValue((uint32_t)SpeedUnitsMetersPerSecond) << QVariant::fromValue((uint32_t)SpeedUnitsMilesPerHour) << QVariant::fromValue((uint32_t)SpeedUnitsKilometersPerHour) << QVariant::fromValue((uint32_t)SpeedUnitsKnots);

        FactMetaData* metaData = new FactMetaData(FactMetaData::valueTypeUint32, this);
        metaData->setName(speedUnitsSettingsName);
        metaData->setShortDescription(tr("Speed units"));
        metaData->setEnumInfo(enumStrings, enumValues);
        metaData->setRawDefaultValue(SpeedUnitsMetersPerSecond);
        metaData->setQGCRebootRequired(true);

        _speedUnitsFact = new SettingsFact(_settingsGroup, metaData, this);
    }

    return _speedUnitsFact;
}

Fact* UnitsSettings::temperatureUnits(void)
{
    if (!_temperatureUnitsFact) {
        // Units settings can't be loaded from json since it creates an infinite loop of meta data loading.
        QStringList     enumStrings;
        QVariantList    enumValues;

        enumStrings << "Celsius" << "Farenheit";
        enumValues << QVariant::fromValue((uint32_t)TemperatureUnitsCelsius) << QVariant::fromValue((uint32_t)TemperatureUnitsFarenheit);

        FactMetaData* metaData = new FactMetaData(FactMetaData::valueTypeUint32, this);
        metaData->setName(temperatureUnitsSettingsName);
        metaData->setShortDescription(tr("Temperature units"));
        metaData->setEnumInfo(enumStrings, enumValues);
        metaData->setRawDefaultValue(TemperatureUnitsCelsius);
        metaData->setQGCRebootRequired(true);

        _temperatureUnitsFact = new SettingsFact(_settingsGroup, metaData, this);
    }

    return _temperatureUnitsFact;
}
