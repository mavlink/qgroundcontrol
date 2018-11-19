/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCApplication.h"
#include "FlightMapSettings.h"
#include "QGCMapEngine.h"
#include "AppSettings.h"
#include "SettingsManager.h"

#include <QQmlEngine>
#include <QtQml>

DECLARE_SETTINGGROUP(FlightMap, "FlightMap")
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    qmlRegisterUncreatableType<FlightMapSettings>("QGroundControl.SettingsManager", 1, 0, "FlightMapSettings", "Reference only");

    // Save the original version since we modify based on map provider
    _savedMapTypeStrings = _nameToMetaDataMap[mapTypeName]->enumStrings();
    _savedMapTypeValues  = _nameToMetaDataMap[mapTypeName]->enumValues();

#ifdef QGC_NO_GOOGLE_MAPS
    //-- Remove Google
    _excludeProvider(mapProviderGoogle);
#endif
    if(qgcApp()->toolbox()->settingsManager()->appSettings()->mapboxToken()->rawValue().toString().isEmpty()) {
        _excludeProvider(mapProviderMapbox);
    }
    if(qgcApp()->toolbox()->settingsManager()->appSettings()->esriToken()->rawValue().toString().isEmpty()) {
        _excludeProvider(mapProviderEsri);
    }
    _newMapProvider(mapProvider()->rawValue());
}

DECLARE_SETTINGSFACT(FlightMapSettings, mapType)

DECLARE_SETTINGSFACT_NO_FUNC(FlightMapSettings, mapProvider)
{
    if (!_mapProviderFact) {
        _mapProviderFact = _createSettingsFact(mapProviderName);
        connect(_mapProviderFact, &Fact::rawValueChanged, this, &FlightMapSettings::_newMapProvider);
    }
    return _mapProviderFact;
}

void FlightMapSettings::_excludeProvider(MapProvider_t provider)
{
    FactMetaData* metaData = _nameToMetaDataMap[mapProviderName];
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
    FactMetaData* metaData = _nameToMetaDataMap[mapTypeName];

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
    case mapProviderEniro:
        _removeEnumValue(mapTypeStreet, enumStrings, enumValues);
        _removeEnumValue(mapTypeSatellite, enumStrings, enumValues);
        _removeEnumValue(mapTypeHybrid, enumStrings, enumValues);
        break;
    case mapProviderEsri:
        _removeEnumValue(mapTypeHybrid, enumStrings, enumValues);
        break;
    case mapProviderVWorld:
        _removeEnumValue(mapTypeHybrid, enumStrings, enumValues);
        _removeEnumValue(mapTypeTerrain, enumStrings, enumValues);
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
