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
    Q_OBJECT

protected:
    EsriMapProvider(const QString &mapName, quint32 averageSize, QGeoMapType::MapStyle mapType, QObject* parent = nullptr)
        : MapProvider(QString(), QString(), averageSize, mapType, parent)
        , _mapName(mapName) {}

public:
    QNetworkRequest getTileURL(int x, int y, int zoom) const final;

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapName;
    const QString _mapUrl = QStringLiteral("http://services.arcgisonline.com/ArcGIS/rest/services/%1/MapServer/tile/%2/%3/%4");
};

class EsriWorldStreetMapProvider : public EsriMapProvider
{
    Q_OBJECT

public:
    EsriWorldStreetMapProvider(QObject* parent = nullptr)
        : EsriMapProvider(QStringLiteral("World_Street_Map"), AVERAGE_TILE_SIZE, QGeoMapType::StreetMap, parent) {}
};

class EsriWorldSatelliteMapProvider : public EsriMapProvider
{
    Q_OBJECT

public:
    EsriWorldSatelliteMapProvider(QObject* parent = nullptr)
        : EsriMapProvider(QStringLiteral("World_Imagery"), AVERAGE_TILE_SIZE, QGeoMapType::SatelliteMapDay, parent) {}
};

class EsriTerrainMapProvider : public EsriMapProvider
{
    Q_OBJECT

public:
    EsriTerrainMapProvider(QObject* parent = nullptr)
        : EsriMapProvider(QStringLiteral("World_Terrain_Base"), AVERAGE_TILE_SIZE, QGeoMapType::TerrainMap, parent) {}
};
