/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

#include "FlightMapSettings.h"

#include <QSettings>
#include <QtQml>

const char* FlightMapSettings::_defaultMapProvider =    "Bing";                 // Bing is default since it support full street/satellite/hybrid set
const char* FlightMapSettings::_settingsGroup =         "FlightMapSettings";
const char* FlightMapSettings::_mapProviderKey =        "MapProvider";
const char* FlightMapSettings::_mapTypeKey =            "MapType";

FlightMapSettings::FlightMapSettings(QGCApplication* app)
    : QGCTool(app)
    , _mapProvider(_defaultMapProvider)
{
}

void FlightMapSettings::setToolbox(QGCToolbox *toolbox)
{
    QGCTool::setToolbox(toolbox);

    qmlRegisterUncreatableType<FlightMapSettings> ("QGroundControl", 1, 0, "FlightMapSetting", "Reference only");

    _supportedMapProviders << "Bing" << "Google"; // << "OpenStreetMap";

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
    QSettings settings;

    settings.beginGroup(_settingsGroup);
    _mapProvider = settings.value(_mapProviderKey, _defaultMapProvider).toString();

    if (!_supportedMapProviders.contains(_mapProvider)) {
        _mapProvider = _defaultMapProvider;
    }

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

    if (_mapProvider == "Bing") {
        _mapTypes << "Street Map" << "Satellite Map" << "Hybrid Map";
    } else if (_mapProvider == "Google") {
        _mapTypes << "Street Map" << "Satellite Map" << "Terrain Map";
    /*
    } else if (_mapProvider == "OpenStreetMap") {
        _mapTypes << "Street Map";
    */
    }

    emit mapTypesChanged(_mapTypes);
}

QString FlightMapSettings::mapTypeForMapName(const QString& mapName)
{
    QSettings settings;

    settings.beginGroup(_settingsGroup);
    settings.beginGroup(mapName);
    settings.beginGroup(_mapProvider);
    return settings.value(_mapTypeKey, "Satellite Map").toString();
}

void FlightMapSettings::setMapTypeForMapName(const QString& mapName, const QString& mapType)
{
    QSettings settings;

    settings.beginGroup(_settingsGroup);
    settings.beginGroup(mapName);
    settings.beginGroup(_mapProvider);
    settings.setValue(_mapTypeKey, mapType);
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
