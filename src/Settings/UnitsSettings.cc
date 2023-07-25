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

DECLARE_SETTINGSFACT_NO_FUNC(UnitsSettings, horizontalDistanceUnits)
{
    if (!_horizontalDistanceUnitsFact) {
        // Distance/Area/Speed units settings can't be loaded from json since it creates an infinite loop of meta data loading.
        QStringList     enumStrings;
        QVariantList    enumValues;
        enumStrings << "Feet" << "Meters";
        enumValues << QVariant::fromValue(static_cast<uint32_t>(HorizontalDistanceUnitsFeet))
                   << QVariant::fromValue(static_cast<uint32_t>(HorizontalDistanceUnitsMeters));
        FactMetaData* metaData = new FactMetaData(FactMetaData::valueTypeUint32, this);
        metaData->setName(horizontalDistanceUnitsName);
        metaData->setShortDescription("Distance units");
        metaData->setEnumInfo(enumStrings, enumValues);

        HorizontalDistanceUnits defaultHorizontalDistanceUnit = HorizontalDistanceUnitsMeters;
        switch(QLocale::system().measurementSystem()) {
            case QLocale::MetricSystem: {
                defaultHorizontalDistanceUnit = HorizontalDistanceUnitsMeters;
            } break;
            case QLocale::ImperialUSSystem:
            case QLocale::ImperialUKSystem:
                defaultHorizontalDistanceUnit = HorizontalDistanceUnitsFeet;
                break;
        }
        metaData->setRawDefaultValue(defaultHorizontalDistanceUnit);
        metaData->setQGCRebootRequired(true);
        _horizontalDistanceUnitsFact = new SettingsFact(_settingsGroup, metaData, this);
    }
    return _horizontalDistanceUnitsFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(UnitsSettings, verticalDistanceUnits)
{
    if (!_verticalDistanceUnitsFact) {
        // Distance/Area/Speed units settings can't be loaded from json since it creates an infinite loop of meta data loading.
        QStringList     enumStrings;
        QVariantList    enumValues;
        enumStrings << "Feet" << "Meters";
        enumValues << QVariant::fromValue(static_cast<uint32_t>(VerticalDistanceUnitsFeet))
                   << QVariant::fromValue(static_cast<uint32_t>(VerticalDistanceUnitsMeters));
        FactMetaData* metaData = new FactMetaData(FactMetaData::valueTypeUint32, this);
        metaData->setName(verticalDistanceUnitsName);
        metaData->setShortDescription("Altitude units");
        metaData->setEnumInfo(enumStrings, enumValues);
        VerticalDistanceUnits defaultVerticalAltitudeUnit = VerticalDistanceUnitsMeters;
        switch(QLocale::system().measurementSystem()) {
            case QLocale::MetricSystem: {
                defaultVerticalAltitudeUnit = VerticalDistanceUnitsMeters;
            } break;
            case QLocale::ImperialUSSystem:
            case QLocale::ImperialUKSystem:
                defaultVerticalAltitudeUnit = VerticalDistanceUnitsFeet;
                break;
        }
        metaData->setRawDefaultValue(defaultVerticalAltitudeUnit);
        metaData->setQGCRebootRequired(true);
        _verticalDistanceUnitsFact = new SettingsFact(_settingsGroup, metaData, this);
    }
    return _verticalDistanceUnitsFact;
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

        AreaUnits defaultAreaUnit = AreaUnitsSquareMeters;
        switch(QLocale::system().measurementSystem()) {
            case QLocale::MetricSystem: {
                defaultAreaUnit = AreaUnitsSquareMeters;
            } break;
            case QLocale::ImperialUSSystem:
            case QLocale::ImperialUKSystem:
                defaultAreaUnit = AreaUnitsSquareMiles;
                break;
        }
        metaData->setRawDefaultValue(defaultAreaUnit);
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

        SpeedUnits defaultSpeedUnit = SpeedUnitsMetersPerSecond;
        switch(QLocale::system().measurementSystem()) {
            case QLocale::MetricSystem: {
                defaultSpeedUnit = SpeedUnitsMetersPerSecond;
            } break;
            case QLocale::ImperialUSSystem:
            case QLocale::ImperialUKSystem:
                defaultSpeedUnit = SpeedUnitsMilesPerHour;
                break;
        }
        metaData->setRawDefaultValue(defaultSpeedUnit);
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

        TemperatureUnits defaultTemperatureUnit = TemperatureUnitsCelsius;
        switch(QLocale::system().measurementSystem()) {
            case QLocale::MetricSystem: {
                defaultTemperatureUnit = TemperatureUnitsCelsius;
            } break;
            case QLocale::ImperialUSSystem:
            case QLocale::ImperialUKSystem:
                defaultTemperatureUnit = TemperatureUnitsFarenheit;
                break;
        }
        metaData->setRawDefaultValue(defaultTemperatureUnit);
        metaData->setQGCRebootRequired(true);
        _temperatureUnitsFact = new SettingsFact(_settingsGroup, metaData, this);
    }
    return _temperatureUnitsFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(UnitsSettings, weightUnits)
{
    if (!_weightUnitsFact) {
        // Units settings can't be loaded from json since it creates an infinite loop of meta data loading.
        QStringList     enumStrings;
        QVariantList    enumValues;
        enumStrings << "Grams" << "Kilograms" << "Ounces" << "Pounds";
        enumValues
            << QVariant::fromValue(static_cast<uint32_t>(WeightUnitsGrams))
            << QVariant::fromValue(static_cast<uint32_t>(WeightUnitsKg))
            << QVariant::fromValue(static_cast<uint32_t>(WeightUnitsOz))
            << QVariant::fromValue(static_cast<uint32_t>(WeightUnitsLbs));
        FactMetaData* metaData = new FactMetaData(FactMetaData::valueTypeUint32, this);
        metaData->setName(weightUnitsName);
        metaData->setShortDescription(UnitsSettings::tr("Weight units"));
        metaData->setEnumInfo(enumStrings, enumValues);
        WeightUnits defaultWeightUnit = WeightUnitsGrams;
        switch(QLocale::system().measurementSystem()) {
            case QLocale::MetricSystem:
            case QLocale::ImperialUKSystem: {
                defaultWeightUnit = WeightUnitsGrams;
            } break;
            case QLocale::ImperialUSSystem:
                defaultWeightUnit = WeightUnitsOz;
                break;
        }
        metaData->setRawDefaultValue(defaultWeightUnit);
        metaData->setQGCRebootRequired(true);
        _weightUnitsFact = new SettingsFact(_settingsGroup, metaData, this);
    }
    return _weightUnitsFact;
}
