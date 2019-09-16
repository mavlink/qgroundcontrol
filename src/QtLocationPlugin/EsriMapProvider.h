#pragma once

#include "MapProvider.h"

#include <QByteArray>
#include <QMutex>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QPoint>
#include <QString>

class EsriMapProvider : public MapProvider {
    Q_OBJECT
  public:
    using MapProvider::MapProvider;

    QNetworkRequest getTileURL(int x, int y, int zoom,
                               QNetworkAccessManager* networkManager);
};

class EsriWorldStreetMapProvider : public EsriMapProvider {
    Q_OBJECT
  public:
    EsriWorldStreetMapProvider(QObject* parent)
        : EsriMapProvider(QString(""), QString(""), AVERAGE_TILE_SIZE,
                          QGeoMapType::StreetMap, parent) {}
    QString _getURL(int x, int y, int zoom,
                    QNetworkAccessManager* networkManager);
};

class EsriWorldSatelliteMapProvider : public EsriMapProvider {
    Q_OBJECT
  public:
    EsriWorldSatelliteMapProvider(QObject* parent)
        : EsriMapProvider(QString(""), QString(""), AVERAGE_TILE_SIZE,
                          QGeoMapType::SatelliteMapDay, parent) {}
    QString _getURL(int x, int y, int zoom,
                    QNetworkAccessManager* networkManager);
};

class EsriTerrainMapProvider : public EsriMapProvider {
    Q_OBJECT
  public:
    EsriTerrainMapProvider(QObject* parent)
        : EsriMapProvider(QString(""), QString(""), AVERAGE_TILE_SIZE,
                          QGeoMapType::TerrainMap, parent) {}
    QString _getURL(int x, int y, int zoom,
                    QNetworkAccessManager* networkManager);
};
