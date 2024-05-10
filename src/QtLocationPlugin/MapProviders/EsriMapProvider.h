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

class EsriMapProvider : public MapProvider
{
protected:
    EsriMapProvider(const QString &mapName, quint32 averageSize, QGeoMapType::MapStyle mapStyle, const QString &mapUrl)
    : MapProvider(mapName, QString(), QString(), averageSize, mapStyle)
    , m_url(mapUrl) {}

    QString _getURL(int x, int y, int zoom) const final { return m_url.arg(zoom).arg(y).arg(x); }
    QByteArray getToken() const final;

private:
    const QString m_url;
};

class EsriWorldStreetMapProvider : public EsriMapProvider
{
public:
    EsriWorldStreetMapProvider()
        : EsriMapProvider(
            "Esri World Street",
            AVERAGE_TILE_SIZE,
            QGeoMapType::StreetMap,
            QStringLiteral("http://services.arcgisonline.com/ArcGIS/rest/services/World_Street_Map/MapServer/tile/%1/%2/%3")) {}
};

class EsriWorldSatelliteMapProvider : public EsriMapProvider
{
public:
    EsriWorldSatelliteMapProvider()
        : EsriMapProvider(
            "Esri World Satellite",
            AVERAGE_TILE_SIZE,
            QGeoMapType::SatelliteMapDay,
            QStringLiteral("http://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/%1/%2/%3")) {}
};

class EsriTerrainMapProvider : public EsriMapProvider
{
public:
    EsriTerrainMapProvider()
        : EsriMapProvider(
            "Esri Terrain",
            AVERAGE_TILE_SIZE,
            QGeoMapType::TerrainMap,
            QStringLiteral("http://server.arcgisonline.com/ArcGIS/rest/services/World_Terrain_Base/MapServer/tile/%1/%2/%3")) {}
};
