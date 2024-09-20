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

class EsriMapProvider : public MapProvider
{
protected:
    EsriMapProvider(const QString &mapName, const QString &mapTypeId, quint32 averageSize, QGeoMapType::MapStyle mapType)
        : MapProvider(
            mapName,
            QStringLiteral(""),
            QStringLiteral(""),
            averageSize,
            mapType)
        , _mapTypeId(mapTypeId) {}

public:
    QByteArray getToken() const final;

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapTypeId;
    const QString _mapUrl = QStringLiteral("http://services.arcgisonline.com/ArcGIS/rest/services/%1/MapServer/tile/%2/%3/%4");
};

class EsriWorldStreetMapProvider : public EsriMapProvider
{
public:
    EsriWorldStreetMapProvider()
        : EsriMapProvider(
            QStringLiteral("Esri World Street"),
            QStringLiteral("World_Street_Map"),
            AVERAGE_TILE_SIZE,
            QGeoMapType::StreetMap) {}
};

class EsriWorldSatelliteMapProvider : public EsriMapProvider
{
public:
    EsriWorldSatelliteMapProvider()
        : EsriMapProvider(
            QStringLiteral("Esri World Satellite"),
            QStringLiteral("World_Imagery"),
            AVERAGE_TILE_SIZE,
            QGeoMapType::SatelliteMapDay) {}
};

class EsriTerrainMapProvider : public EsriMapProvider
{
public:
    EsriTerrainMapProvider()
        : EsriMapProvider(
            QStringLiteral("Esri Terrain"),
            QStringLiteral("World_Terrain_Base"),
            AVERAGE_TILE_SIZE,
            QGeoMapType::TerrainMap) {}
};
