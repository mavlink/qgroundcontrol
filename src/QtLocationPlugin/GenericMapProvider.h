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

class StatkartMapProvider : public MapProvider {
    Q_OBJECT
  public:
    StatkartMapProvider(QObject* parent = nullptr)
        : MapProvider(QStringLiteral("https://www.norgeskart.no/"), QStringLiteral("png"),
                      AVERAGE_TILE_SIZE, QGeoMapType::StreetMap, parent) {}

    QString _getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) override;
};

class EniroMapProvider : public MapProvider {
    Q_OBJECT
  public:
    EniroMapProvider(QObject* parent = nullptr)
        : MapProvider(QStringLiteral("https://www.eniro.se/"), QStringLiteral("png"),
                      AVERAGE_TILE_SIZE, QGeoMapType::StreetMap, parent) {}

    QString _getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) override;
};

class MapQuestMapMapProvider : public MapProvider {
    Q_OBJECT
  public:
    MapQuestMapMapProvider(QObject* parent = nullptr)
        : MapProvider(QStringLiteral("https://mapquest.com"), QStringLiteral("jpg"),
                      AVERAGE_TILE_SIZE, QGeoMapType::StreetMap, parent) {}

    QString _getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) override;
};

class MapQuestSatMapProvider : public MapProvider {
    Q_OBJECT
  public:
    MapQuestSatMapProvider(QObject* parent = nullptr)
        : MapProvider(QStringLiteral("https://mapquest.com"), QStringLiteral("jpg"),
                      AVERAGE_TILE_SIZE, QGeoMapType::SatelliteMapDay, parent) {}

    QString _getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) override;
};

class VWorldStreetMapProvider : public MapProvider {
    Q_OBJECT
  public:
    VWorldStreetMapProvider(QObject* parent = nullptr)
        : MapProvider(QStringLiteral("www.vworld.kr"), QStringLiteral("png"),
                      AVERAGE_TILE_SIZE, QGeoMapType::StreetMap, parent) {}

    QString _getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) override;

  private:
    const QString _versionBingMaps = QStringLiteral("563");
};

class VWorldSatMapProvider : public MapProvider {
    Q_OBJECT
  public:
    VWorldSatMapProvider(QObject* parent = nullptr)
        : MapProvider(QStringLiteral("www.vworld.kr"), QStringLiteral("jpg"),
                      AVERAGE_TILE_SIZE, QGeoMapType::SatelliteMapDay, parent) {}

    QString _getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) override;

  private:
    const QString _versionBingMaps = QStringLiteral("563");
};
