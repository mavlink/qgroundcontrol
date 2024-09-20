/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "MapProvider.h"

static constexpr const quint32 AVERAGE_MAPBOX_SAT_MAP     = 15739;
static constexpr const quint32 AVERAGE_MAPBOX_STREET_MAP  = 5648;

class MapboxMapProvider : public MapProvider
{
protected:
    MapboxMapProvider(const QString &mapName, const QString &mapTypeId, quint32 averageSize, QGeoMapType::MapStyle mapType)
        : MapProvider(
            mapName,
            QStringLiteral("https://www.mapbox.com/"),
            QStringLiteral("jpg"),
            averageSize,
            mapType)
        , _mapTypeId(mapTypeId) {}

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapTypeId;
};

class MapboxStreetMapProvider : public MapboxMapProvider
{
public:
    MapboxStreetMapProvider()
        : MapboxMapProvider(
            QStringLiteral("Mapbox Streets"),
            QStringLiteral("streets-v10"),
            AVERAGE_MAPBOX_STREET_MAP,
            QGeoMapType::StreetMap) {}
};

class MapboxLightMapProvider : public MapboxMapProvider
{
public:
    MapboxLightMapProvider()
        : MapboxMapProvider(
            QStringLiteral("Mapbox Light"),
            QStringLiteral("light-v9"),
            AVERAGE_TILE_SIZE,
            QGeoMapType::CustomMap) {}
};

class MapboxDarkMapProvider : public MapboxMapProvider
{
public:
    MapboxDarkMapProvider()
        : MapboxMapProvider(
            QStringLiteral("Mapbox Dark"),
            QStringLiteral("dark-v9"),
            AVERAGE_TILE_SIZE,
            QGeoMapType::CustomMap) {}
};

class MapboxSatelliteMapProvider : public MapboxMapProvider
{
public:
    MapboxSatelliteMapProvider()
        : MapboxMapProvider(
            QStringLiteral("Mapbox Satellite"),
            QStringLiteral("satellite-v9"),
            AVERAGE_MAPBOX_SAT_MAP,
            QGeoMapType::SatelliteMapDay) {}
};

class MapboxHybridMapProvider : public MapboxMapProvider
{
public:
    MapboxHybridMapProvider()
        : MapboxMapProvider(
            QStringLiteral("Mapbox Hybrid"),
            QStringLiteral("satellite-streets-v10"),
            AVERAGE_MAPBOX_SAT_MAP,
            QGeoMapType::HybridMap) {}
};

class MapboxBrightMapProvider : public MapboxMapProvider
{
public:
    MapboxBrightMapProvider()
        : MapboxMapProvider(
            QStringLiteral("Mapbox Bright"),
            QStringLiteral("bright-v9"),
            AVERAGE_TILE_SIZE,
            QGeoMapType::CustomMap) {}
};

class MapboxStreetsBasicMapProvider : public MapboxMapProvider
{
public:
    MapboxStreetsBasicMapProvider()
        : MapboxMapProvider(
            QStringLiteral("Mapbox StreetsBasic"),
            QStringLiteral("basic-v9"),
            AVERAGE_TILE_SIZE,
            QGeoMapType::StreetMap) {}
};

class MapboxOutdoorsMapProvider : public MapboxMapProvider
{
public:
    MapboxOutdoorsMapProvider()
        : MapboxMapProvider(
            QStringLiteral("Mapbox Outdoors"),
            QStringLiteral("outdoors-v10"),
            AVERAGE_TILE_SIZE,
            QGeoMapType::CustomMap) {}
};

class MapboxCustomMapProvider : public MapboxMapProvider
{
public:
    MapboxCustomMapProvider()
        : MapboxMapProvider(
            QStringLiteral("Mapbox Custom"),
            QStringLiteral("mapbox.custom"),
            AVERAGE_TILE_SIZE,
            QGeoMapType::CustomMap) {}
};
