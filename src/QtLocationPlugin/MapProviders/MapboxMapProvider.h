/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "MapProvider.h"

#define AVERAGE_MAPBOX_SAT_MAP     15739
#define AVERAGE_MAPBOX_STREET_MAP  5648

class MapboxMapProvider : public MapProvider
{
protected:
    MapboxMapProvider(const QString& mapName, const QString& mapType, quint32 averageSize, QGeoMapType::MapStyle mapStyle)
        : MapProvider(mapName, QStringLiteral("https://www.mapbox.com/"), QStringLiteral("jpg"), averageSize, mapStyle)
        , m_mapboxName(mapType) {}

private:
    QString _getURL(int x, int y, int zoom) const final;

    QString m_mapboxName;
};

class MapboxStreetMapProvider : public MapboxMapProvider
{
public:
    MapboxStreetMapProvider()
        : MapboxMapProvider("Mapbox Streets", QStringLiteral("streets-v10"), AVERAGE_MAPBOX_STREET_MAP,
                            QGeoMapType::StreetMap) {}
};

class MapboxLightMapProvider : public MapboxMapProvider
{
public:
    MapboxLightMapProvider()
        : MapboxMapProvider("Mapbox Light", QStringLiteral("light-v9"), AVERAGE_TILE_SIZE,
                            QGeoMapType::CustomMap) {}
};

class MapboxDarkMapProvider : public MapboxMapProvider
{
public:
    MapboxDarkMapProvider()
        : MapboxMapProvider("Mapbox Dark", QStringLiteral("dark-v9"), AVERAGE_TILE_SIZE,
                            QGeoMapType::CustomMap) {}
};

class MapboxSatelliteMapProvider : public MapboxMapProvider
{
public:
    MapboxSatelliteMapProvider()
        : MapboxMapProvider("Mapbox Satellite", QStringLiteral("satellite-v9"), AVERAGE_MAPBOX_SAT_MAP,
                            QGeoMapType::SatelliteMapDay) {}
};

class MapboxHybridMapProvider : public MapboxMapProvider
{
public:
    MapboxHybridMapProvider()
        : MapboxMapProvider("Mapbox Hybrid", QStringLiteral("satellite-streets-v10"), AVERAGE_MAPBOX_SAT_MAP,
                            QGeoMapType::HybridMap) {}
};

class MapboxBrightMapProvider : public MapboxMapProvider
{
public:
    MapboxBrightMapProvider()
        : MapboxMapProvider("Mapbox Bright", QStringLiteral("bright-v9"), AVERAGE_TILE_SIZE,
                            QGeoMapType::CustomMap) {}
};

class MapboxStreetsBasicMapProvider : public MapboxMapProvider
{
public:
    MapboxStreetsBasicMapProvider()
        : MapboxMapProvider("Mapbox StreetsBasic", QStringLiteral("basic-v9"), AVERAGE_TILE_SIZE,
                            QGeoMapType::StreetMap) {}
};

class MapboxOutdoorsMapProvider : public MapboxMapProvider
{
public:
    MapboxOutdoorsMapProvider()
        : MapboxMapProvider("Mapbox Outdoors", QStringLiteral("outdoors-v10"), AVERAGE_TILE_SIZE,
                            QGeoMapType::CustomMap) {}
};

class MapboxCustomMapProvider : public MapboxMapProvider
{
public:
    MapboxCustomMapProvider()
        : MapboxMapProvider("Mapbox Custom", QStringLiteral("mapbox.custom"), AVERAGE_TILE_SIZE,
                            QGeoMapType::CustomMap) {}
};
