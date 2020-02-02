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

class EsriMapProvider : public MapProvider {
    Q_OBJECT

  public:
    EsriMapProvider(const quint32 averageSize, const QGeoMapType::MapStyle mapType, QObject* parent = nullptr);

    QNetworkRequest getTileURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) override;
};

class EsriWorldStreetMapProvider : public EsriMapProvider {
    Q_OBJECT

  public:
    EsriWorldStreetMapProvider(QObject* parent = nullptr)
        : EsriMapProvider(AVERAGE_TILE_SIZE, QGeoMapType::StreetMap, parent) {}

    QString _getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) override;
};

class EsriWorldSatelliteMapProvider : public EsriMapProvider {
    Q_OBJECT

  public:
    EsriWorldSatelliteMapProvider(QObject* parent = nullptr)
        : EsriMapProvider(AVERAGE_TILE_SIZE, QGeoMapType::SatelliteMapDay, parent) {}

    QString _getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) override;
};

class EsriTerrainMapProvider : public EsriMapProvider {
    Q_OBJECT

  public:
    EsriTerrainMapProvider(QObject* parent = nullptr)
        : EsriMapProvider(AVERAGE_TILE_SIZE, QGeoMapType::TerrainMap, parent) {}

    QString _getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) override;
};
