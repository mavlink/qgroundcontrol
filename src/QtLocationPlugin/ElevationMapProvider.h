#pragma once

#include "MapProvider.h"
#include <cmath>

#include <QByteArray>
#include <QMutex>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QPoint>
#include <QString>

static const quint32 AVERAGE_AIRMAP_ELEV_SIZE = 2786;
static const quint32 AVERAGE_APSTR3_ELEV_SIZE = 100000; // Around 1 MB?

class ElevationProvider : public MapProvider {
    Q_OBJECT
  public:
    ElevationProvider(const QString& imageFormat, quint32 averageSize,
                      QGeoMapType::MapStyle mapType, const QString &referrer, 
                      QObject* parent = nullptr);

    virtual bool _isElevationProvider() const override { return true; }
};

// -----------------------------------------------------------
// Airmap Elevation

class AirmapElevationProvider : public ElevationProvider {
    Q_OBJECT
  public:
    AirmapElevationProvider(QObject* parent = nullptr)
        : ElevationProvider(QStringLiteral("bin"), AVERAGE_AIRMAP_ELEV_SIZE,
                            QGeoMapType::StreetMap, QStringLiteral("https://api.airmap.com/"), parent) {}

    int long2tileX(const double lon, const int z) const override;

    int lat2tileY(const double lat, const int z) const override;

    QGCTileSet getTileCount(const int zoom, const double topleftLon,
                            const double topleftLat, const double bottomRightLon,
                            const double bottomRightLat) const override;

    // Airmap needs to serialize the tiles, because they are received in json format. This way we can work with
    // them in the map tiles database
    bool serializeTilesNeeded() override { return true; }
    QByteArray serializeTile(QByteArray image) override;

  protected:
    QString _getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) override;
};

// -----------------------------------------------------------
// Ardupilot Terrain Server Elevation SRTM1
// https://terrain.ardupilot.org/SRTM1/

class ApStr1ElevationProvider : public ElevationProvider {
    Q_OBJECT
  public:
    ApStr1ElevationProvider(QObject* parent = nullptr)
        : ElevationProvider(QStringLiteral("hgt"), AVERAGE_APSTR3_ELEV_SIZE,
                            QGeoMapType::StreetMap, QStringLiteral("https://terrain.ardupilot.org/SRTM1/"), parent) {}

    int long2tileX(const double lon, const int z) const override;

    int lat2tileY(const double lat, const int z) const override;

    QGCTileSet getTileCount(const int zoom, const double topleftLon,
                            const double topleftLat, const double bottomRightLon,
                            const double bottomRightLat) const override;

    bool unzippingTilesNeeded() override { return true; }
    QByteArray unzipTile(QByteArray response) override;

  protected:
    QString _getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) override;
};
