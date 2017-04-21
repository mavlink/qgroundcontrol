/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FlightMapSettings.h"
#include "QGCMapEngine.h"

#include <QQmlEngine>
#include <QtQml>

const char* FlightMapSettings::flightMapSettingsGroupName =  "FlightMap";
const char* FlightMapSettings::mapProviderSettingsName =     "MapProvider";
const char* FlightMapSettings::mapTypeSettingsName =         "MapType";
const char* FlightMapSettings::_settingsGroupName =          "FlightMap";

FlightMapSettings::FlightMapSettings(QObject* parent)
    : SettingsGroup(flightMapSettingsGroupName, QString(_settingsGroupName) /* root settings group */, parent)
    , _mapProviderFact(NULL)
    , _mapTypeFact(NULL)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    qmlRegisterUncreatableType<FlightMapSettings>("QGroundControl.SettingsManager", 1, 0, "FlightMapSettings", "Reference only");

    // Save the original version since we modify based on map provider
    _savedMapTypeStrings = _nameToMetaDataMap[mapTypeSettingsName]->enumStrings();
    _savedMapTypeValues  = _nameToMetaDataMap[mapTypeSettingsName]->enumValues();

#ifdef QGC_NO_GOOGLE_MAPS
    //-- Remove Google
    _excludeProvider(mapProviderGoogle);
#endif
    if(getQGCMapEngine()->getMapBoxToken().isEmpty()) {
        _excludeProvider(mapProviderMapBox);
    }
    if(getQGCMapEngine()->getEsriToken().isEmpty()) {
        _excludeProvider(mapProviderEsri);
    }
    _newMapProvider(mapProvider()->rawValue());
}

Fact* FlightMapSettings::mapProvider(void)
{
    if (!_mapProviderFact) {
        _mapProviderFact = _createSettingsFact(mapProviderSettingsName);
        connect(_mapProviderFact, &Fact::rawValueChanged, this, &FlightMapSettings::_newMapProvider);
    }

    return _mapProviderFact;
}

Fact* FlightMapSettings::mapType(void)
{
    if (!_mapTypeFact) {
        _mapTypeFact = _createSettingsFact(mapTypeSettingsName);
    }

    return _mapTypeFact;
}

void FlightMapSettings::_excludeProvider(MapProvider_t provider)
{
    FactMetaData* metaData = _nameToMetaDataMap[mapProviderSettingsName];
    QVariantList enumValues = metaData->enumValues();
    QStringList enumStrings = metaData->enumStrings();
    _removeEnumValue(provider, enumStrings, enumValues);
    metaData->setEnumInfo(enumStrings, enumValues);
}

void FlightMapSettings::_removeEnumValue(int value, QStringList& enumStrings, QVariantList& enumValues)
{
    bool found = false;
    int removeIndex;
    for (removeIndex=0; removeIndex<enumValues.count(); removeIndex++) {
        if (enumValues[removeIndex].toInt() == value) {
            found = true;
            break;
        }
    }

    if (found) {
        enumValues.removeAt(removeIndex);
        enumStrings.removeAt(removeIndex);
    }
}

void FlightMapSettings::_newMapProvider(QVariant value)
{
    FactMetaData* metaData = _nameToMetaDataMap[mapTypeSettingsName];

    QStringList enumStrings = _savedMapTypeStrings;
    QVariantList enumValues = _savedMapTypeValues;

    switch (value.toInt()) {
    case mapProviderBing:
        _removeEnumValue(mapTypeTerrain, enumStrings, enumValues);
        break;
    case mapProviderGoogle:
        _removeEnumValue(mapTypeHybrid, enumStrings, enumValues);
        break;
    case mapProviderStarkart:
        _removeEnumValue(mapTypeStreet, enumStrings, enumValues);
        _removeEnumValue(mapTypeSatellite, enumStrings, enumValues);
        _removeEnumValue(mapTypeHybrid, enumStrings, enumValues);
        break;
    }
    metaData->setEnumInfo(enumStrings, enumValues);
    emit mapTypeChanged();

    // Check that map type is still valid for this new map provider

    bool found = false;
    int currentMapType = mapType()->rawValue().toInt();
    for (int i=0; i<enumValues.count(); i++) {
        if (currentMapType == enumValues[i].toInt()) {
            found = true;
            break;
        }
    }
    if (!found) {
        mapType()->setRawValue(0);
    }
}
