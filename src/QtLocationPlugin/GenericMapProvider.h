#pragma once

#include "MapProvider.h"
#include <cmath>

#include <QByteArray>
#include <QMutex>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QPoint>
#include <QString>

class StatkartMapProvider : public MapProvider {
    Q_OBJECT
  public:
    StatkartMapProvider(QObject* parent)
        : MapProvider(QString("https://www.norgeskart.no/"), QString("png"),
                      AVERAGE_TILE_SIZE, QGeoMapType::StreetMap, parent) {}

    QString _getURL(int x, int y, int zoom,
                    QNetworkAccessManager* networkManager);
};

class EniroMapProvider : public MapProvider {
    Q_OBJECT
  public:
    EniroMapProvider(QObject* parent)
        : MapProvider(QString("https://www.eniro.se/"), QString("png"),
                      AVERAGE_TILE_SIZE, QGeoMapType::StreetMap, parent) {}

    QString _getURL(int x, int y, int zoom,
                    QNetworkAccessManager* networkManager);
};

class MapQuestMapMapProvider : public MapProvider {
    Q_OBJECT
  public:
    MapQuestMapMapProvider(QObject* parent)
        : MapProvider(QString("https://mapquest.com"), QString("jpg"),
                      AVERAGE_TILE_SIZE, QGeoMapType::StreetMap, parent) {}

    QString _getURL(int x, int y, int zoom,
                    QNetworkAccessManager* networkManager);
};

class MapQuestSatMapProvider : public MapProvider {
    Q_OBJECT
  public:
    MapQuestSatMapProvider(QObject* parent)
        : MapProvider(QString("https://mapquest.com"), QString("jpg"),
                      AVERAGE_TILE_SIZE, QGeoMapType::SatelliteMapDay, parent) {
    }

    QString _getURL(int x, int y, int zoom,
                    QNetworkAccessManager* networkManager);
};

class VWorldStreetMapProvider : public MapProvider {
    Q_OBJECT
  public:
    VWorldStreetMapProvider(QObject* parent)
        : MapProvider(QString("www.vworld.kr"), QString("png"),
                      AVERAGE_TILE_SIZE, QGeoMapType::StreetMap, parent) {}

    QString _getURL(int x, int y, int zoom,
                    QNetworkAccessManager* networkManager);

  private:
    const QString _versionBingMaps = "563";
};

class VWorldSatMapProvider : public MapProvider {
    Q_OBJECT
  public:
    VWorldSatMapProvider(QObject* parent)
        : MapProvider(QString("www.vworld.kr"), QString("jpg"),
                      AVERAGE_TILE_SIZE, QGeoMapType::SatelliteMapDay, parent) {
    }

    QString _getURL(int x, int y, int zoom,
                    QNetworkAccessManager* networkManager);

  private:
    const QString _versionBingMaps = "563";
};

class TestMapProvider : public MapProvider {
    Q_OBJECT
  public:
    TestMapProvider(QObject* parent)
        : MapProvider(QString("https://map.openaerialmap.org"), QString("png"),
                      AVERAGE_TILE_SIZE, QGeoMapType::SatelliteMapDay, parent) {
    }

    QString _getURL(int x, int y, int zoom,
                    QNetworkAccessManager* networkManager);
};
