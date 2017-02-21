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

const char* UnitsSettings::unitsSettingsGroupName =     "units";
const char* UnitsSettings::distanceUnitsSettingsName =  "DistanceUnits";
const char* UnitsSettings::areaUnitsSettingsName =      "AreaUnits";
const char* UnitsSettings::speedUnitsSettingsName =     "SpeedUnits";

UnitsSettings::UnitsSettings(QObject* parent)
    : SettingsGroup(unitsSettingsGroupName, QString() /* root settings group */, parent)
    , _distanceUnitsFact(NULL)
    , _areaUnitsFact(NULL)
    , _speedUnitsFact(NULL)
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
        metaData->setEnumInfo(enumStrings, enumValues);
        metaData->setRawDefaultValue(DistanceUnitsMeters);

        _distanceUnitsFact = new SettingsFact(QString() /* no settings group */, metaData, this);

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
        metaData->setEnumInfo(enumStrings, enumValues);
        metaData->setRawDefaultValue(AreaUnitsSquareMeters);

        _areaUnitsFact = new SettingsFact(QString() /* no settings group */, metaData, this);
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
        metaData->setEnumInfo(enumStrings, enumValues);
        metaData->setRawDefaultValue(SpeedUnitsMetersPerSecond);

        _speedUnitsFact = new SettingsFact(QString() /* no settings group */, metaData, this);
    }

    return _speedUnitsFact;
}
