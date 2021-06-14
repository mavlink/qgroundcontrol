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

class JapanStdMapProvider : public MapProvider {
    Q_OBJECT
  public:
    JapanStdMapProvider(QObject* parent = nullptr)
        : MapProvider(QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/std"), QStringLiteral("png"),
                      AVERAGE_TILE_SIZE, QGeoMapType::StreetMap, parent) {}

    QString _getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) override;
};

class JapanSeamlessMapProvider : public MapProvider {
    Q_OBJECT
  public:
    JapanSeamlessMapProvider(QObject* parent = nullptr)
        : MapProvider(QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/seamlessphoto"), QStringLiteral("jpg"),
                      AVERAGE_TILE_SIZE, QGeoMapType::StreetMap, parent) {}

    QString _getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) override;
};

class JapanAnaglyphMapProvider : public MapProvider {
    Q_OBJECT
  public:
    JapanAnaglyphMapProvider(QObject* parent = nullptr)
        : MapProvider(QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/anaglyphmap_color"), QStringLiteral("png"),
                      AVERAGE_TILE_SIZE, QGeoMapType::StreetMap, parent) {}

    QString _getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) override;
};

class JapanSlopeMapProvider : public MapProvider {
    Q_OBJECT
  public:
    JapanSlopeMapProvider(QObject* parent = nullptr)
        : MapProvider(QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/slopemap"), QStringLiteral("png"),
                      AVERAGE_TILE_SIZE, QGeoMapType::StreetMap, parent) {}

    QString _getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) override;
};

class JapanReliefMapProvider : public MapProvider {
    Q_OBJECT
  public:
    JapanReliefMapProvider(QObject* parent = nullptr)
        : MapProvider(QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/relief"), QStringLiteral("png"),
                      AVERAGE_TILE_SIZE, QGeoMapType::StreetMap, parent) {}

    QString _getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) override;
};

class LINZBasemapMapProvider : public MapProvider {
    Q_OBJECT
  public:
    LINZBasemapMapProvider(QObject* parent = nullptr)
        : MapProvider(QStringLiteral("https://basemaps.linz.govt.nz/v1/tiles/aerial"), QStringLiteral("png"),
                      AVERAGE_TILE_SIZE, QGeoMapType::SatelliteMapDay, parent) {}

    QString _getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) override;
};

class CustomURLMapProvider : public MapProvider {
    Q_OBJECT
  public:
    CustomURLMapProvider(QObject* parent = nullptr)
        : MapProvider(QStringLiteral(""), QStringLiteral(""),
                      AVERAGE_TILE_SIZE, QGeoMapType::CustomMap, parent) {}

    QString _getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) override;
};


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
        : MapProvider(QStringLiteral("www.vworld.kr"), QStringLiteral("jpeg"),
                      AVERAGE_TILE_SIZE, QGeoMapType::SatelliteMapDay, parent) {}

    QString _getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) override;

  private:
    const QString _versionBingMaps = QStringLiteral("563");
};
