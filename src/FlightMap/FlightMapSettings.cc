/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "FlightMapSettings.h"

#include <QSettings>
#include <QtQml>

const char* FlightMapSettings::_defaultMapProvider      = "Bing";                 // Bing is default since it support full street/satellite/hybrid set
const char* FlightMapSettings::_settingsGroup           = "FlightMapSettings";
const char* FlightMapSettings::_mapProviderKey          = "MapProvider";
const char* FlightMapSettings::_mapTypeKey              = "MapType";

FlightMapSettings::FlightMapSettings(QGCApplication* app)
    : QGCTool(app)
    , _mapProvider(_defaultMapProvider)
{
}

void FlightMapSettings::setToolbox(QGCToolbox *toolbox)
{
    QGCTool::setToolbox(toolbox);
    qmlRegisterUncreatableType<FlightMapSettings> ("QGroundControl", 1, 0, "FlightMapSetting", "Reference only");
    _supportedMapProviders << "Bing";
#ifndef QGC_NO_GOOGLE_MAPS
    _supportedMapProviders << "Google";
#endif
    _loadSettings();
}

void FlightMapSettings::_storeSettings(void)
{
    QSettings settings;
    settings.beginGroup(_settingsGroup);
    settings.setValue(_mapProviderKey, _supportedMapProviders.contains(_mapProvider) ? _mapProvider : _defaultMapProvider);
}

void FlightMapSettings::_loadSettings(void)
{
#ifdef QGC_NO_GOOGLE_MAPS
    _mapProvider = _defaultMapProvider;
#else
    QSettings settings;
    settings.beginGroup(_settingsGroup);
    _mapProvider = settings.value(_mapProviderKey, _defaultMapProvider).toString();
    if (!_supportedMapProviders.contains(_mapProvider)) {
        _mapProvider = _defaultMapProvider;
    }
#endif
    _setMapTypesForCurrentProvider();
}

QString FlightMapSettings::mapProvider(void)
{
    return _mapProvider;
}

void FlightMapSettings::setMapProvider(const QString& mapProvider)
{
    if (_supportedMapProviders.contains(mapProvider)) {
        _mapProvider = mapProvider;
        _storeSettings();
        _setMapTypesForCurrentProvider();
        emit mapProviderChanged(mapProvider);
    }
}

void FlightMapSettings::_setMapTypesForCurrentProvider(void)
{
    _mapTypes.clear();
#ifdef QGC_NO_GOOGLE_MAPS
    _mapTypes << "Street Map" << "Satellite Map" << "Hybrid Map";
#else
    if (_mapProvider == "Bing") {
        _mapTypes << "Street Map" << "Satellite Map" << "Hybrid Map";
    } else if (_mapProvider == "Google") {
        _mapTypes << "Street Map" << "Satellite Map" << "Terrain Map";
    }
#endif
    emit mapTypesChanged(_mapTypes);
}

QString FlightMapSettings::mapType(void)
{
    QSettings settings;
    settings.beginGroup(_settingsGroup);
    settings.beginGroup(_mapProvider);
    return settings.value(_mapTypeKey, "Satellite Map").toString();
}

void FlightMapSettings::setMapType(const QString& mapType)
{
    QSettings settings;
    settings.beginGroup(_settingsGroup);
    settings.beginGroup(_mapProvider);
    settings.setValue(_mapTypeKey, mapType);
    emit mapTypeChanged(mapType);
}

void FlightMapSettings::saveMapSetting (const QString &mapName, const QString& key, const QString& value)
{
    QSettings settings;
    settings.beginGroup(_settingsGroup);
    settings.beginGroup(mapName);
    settings.setValue(key, value);
}

QString FlightMapSettings::loadMapSetting (const QString &mapName, const QString& key, const QString& defaultValue)
{
    QSettings settings;
    settings.beginGroup(_settingsGroup);
    settings.beginGroup(mapName);
    return settings.value(key, defaultValue).toString();
}

void FlightMapSettings::saveBoolMapSetting (const QString &mapName, const QString& key, bool value)
{
    QSettings settings;
    settings.beginGroup(_settingsGroup);
    settings.beginGroup(mapName);
    settings.setValue(key, value);
}

bool FlightMapSettings::loadBoolMapSetting (const QString &mapName, const QString& key, bool defaultValue)
{
    QSettings settings;
    settings.beginGroup(_settingsGroup);
    settings.beginGroup(mapName);
    return settings.value(key, defaultValue).toBool();
}
