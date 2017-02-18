/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SettingsManager.h"
#include "QGCApplication.h"

QGC_LOGGING_CATEGORY(SettingsManagerLog, "SettingsManagerLog")

const char* SettingsManager::offlineEditingFirmwareTypeSettingsName =       "OfflineEditingFirmwareType";
const char* SettingsManager::offlineEditingVehicleTypeSettingsName =        "OfflineEditingVehicleType";
const char* SettingsManager::offlineEditingCruiseSpeedSettingsName =        "OfflineEditingCruiseSpeed";
const char* SettingsManager::offlineEditingHoverSpeedSettingsName =         "OfflineEditingHoverSpeed";
const char* SettingsManager::distanceUnitsSettingsName =                    "DistanceUnits";
const char* SettingsManager::areaUnitsSettingsName =                        "AreaUnits";
const char* SettingsManager::speedUnitsSettingsName =                       "SpeedUnits";
const char* SettingsManager::batteryPercentRemainingAnnounceSettingsName =  "batteryPercentRemainingAnnounce";
const char* SettingsManager::defaultMissionItemAltitudeSettingsName =       "DefaultMissionItemAltitude";

SettingsManager::SettingsManager(QGCApplication* app)
    : QGCTool(app)
    , _offlineEditingFirmwareTypeFact(NULL)
    , _offlineEditingVehicleTypeFact(NULL)
    , _offlineEditingCruiseSpeedFact(NULL)
    , _offlineEditingHoverSpeedFact(NULL)
    , _distanceUnitsFact(NULL)
    , _areaUnitsFact(NULL)
    , _speedUnitsFact(NULL)
    , _batteryPercentRemainingAnnounceFact(NULL)
    , _defaultMissionItemAltitudeFact(NULL)
{

}

void SettingsManager::setToolbox(QGCToolbox *toolbox)
{
    QGCTool::setToolbox(toolbox);
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    qmlRegisterUncreatableType<SettingsManager>("QGroundControl.SettingsManager", 1, 0, "SettingsManager", "Reference only");

    _nameToMetaDataMap = FactMetaData::createMapFromJsonFile(":/json/SettingsManager.json", this);
}

SettingsFact* SettingsManager::_createSettingsFact(const QString& name)
{
    return new SettingsFact(QString() /* no settings group */, _nameToMetaDataMap[name], this);
}

Fact* SettingsManager::offlineEditingFirmwareType(void)
{
    if (!_offlineEditingFirmwareTypeFact) {
        _offlineEditingFirmwareTypeFact = _createSettingsFact(offlineEditingFirmwareTypeSettingsName);
    }

    return _offlineEditingFirmwareTypeFact;
}

Fact* SettingsManager::offlineEditingVehicleType(void)
{
    if (!_offlineEditingVehicleTypeFact) {
        _offlineEditingVehicleTypeFact = _createSettingsFact(offlineEditingVehicleTypeSettingsName);
    }

    return _offlineEditingVehicleTypeFact;
}

Fact* SettingsManager::offlineEditingCruiseSpeed(void)
{
    if (!_offlineEditingCruiseSpeedFact) {
        _offlineEditingCruiseSpeedFact = _createSettingsFact(offlineEditingCruiseSpeedSettingsName);
    }
    return _offlineEditingCruiseSpeedFact;
}

Fact* SettingsManager::offlineEditingHoverSpeed(void)
{
    if (!_offlineEditingHoverSpeedFact) {
        _offlineEditingHoverSpeedFact = _createSettingsFact(offlineEditingHoverSpeedSettingsName);
    }
    return _offlineEditingHoverSpeedFact;
}

Fact* SettingsManager::batteryPercentRemainingAnnounce(void)
{
    if (!_batteryPercentRemainingAnnounceFact) {
        _batteryPercentRemainingAnnounceFact = _createSettingsFact(batteryPercentRemainingAnnounceSettingsName);
    }

    return _batteryPercentRemainingAnnounceFact;
}

Fact* SettingsManager::defaultMissionItemAltitude(void)
{
    if (!_defaultMissionItemAltitudeFact) {
        _defaultMissionItemAltitudeFact = _createSettingsFact(defaultMissionItemAltitudeSettingsName);
    }

    return _defaultMissionItemAltitudeFact;
}

Fact* SettingsManager::distanceUnits(void)
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

Fact* SettingsManager::areaUnits(void)
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

Fact* SettingsManager::speedUnits(void)
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
