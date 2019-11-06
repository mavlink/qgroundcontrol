#pragma once

#include "MapProvider.h"
#include <cmath>
#include "TerrainTile.h"

#include <QByteArray>
#include <QMutex>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QPoint>
#include <QString>

static const quint32 AVERAGE_AIRMAP_ELEV_SIZE = 2786;
static const quint32 AVERAGE_AW30_ELEV_SIZE  = 25000000;

//-----------------------------------------------------------------------------
static const double srtm1TileSize = 0.01;

class ElevationProvider : public MapProvider {
    Q_OBJECT
  public:
    ElevationProvider(const QString& imageFormat, quint32 averageSize,
                      QGeoMapType::MapStyle mapType, QObject* parent = nullptr);

    ~ElevationProvider();

    virtual bool _isElevationProvider() const override{ return true; }
};

// -----------------------------------------------------------
// Airmap Elevation

class AirmapElevationProvider : public ElevationProvider {
    Q_OBJECT
  public:
    AirmapElevationProvider(QObject* parent)
        : ElevationProvider(QString("bin"), AVERAGE_AIRMAP_ELEV_SIZE,
                            QGeoMapType::CustomMap, parent) {}

    int        long2tileX(const double lon, const int z) const override;
    int        lat2tileY(const double lat, const int z) const override;
    QGCTileSet getTileCount(const int zoom, const double topleftLon, const double topleftLat,
                            const double bottomRightLon, const double bottomRightLat) const override;

    TerrainTile* newTerrainTile(QByteArray buf) override;
    QByteArray   serialize(QByteArray buf) override;

  protected:
    QString _getURL(const int x,const int y,const int zoom,
                    QNetworkAccessManager* networkManager) override;
};

// -----------------------------------------------------------
// Geotiff Elevation

class GeotiffElevationProvider : public ElevationProvider {
    Q_OBJECT
  public:
    GeotiffElevationProvider(QObject* parent)
        : ElevationProvider(QString("bin"), AVERAGE_AIRMAP_ELEV_SIZE,
                            QGeoMapType::CustomMap, parent) {}

    TerrainTile* newTerrainTile(QByteArray buf) override;
    QByteArray   serialize(QByteArray buf) override;

  protected:
    QString _getURL(const int x, const int y, const int zoom,
                    QNetworkAccessManager* networkManager) override;

  private:
    double tilex2long(const int x,const int z) const;
    double tiley2lat(const int y, const int z) const;
};

// -----------------------------------------------------------
// AW3D Elevation

class AW3DElevationProvider : public ElevationProvider {
    Q_OBJECT
  public:
    AW3DElevationProvider(QObject* parent)
        : ElevationProvider(QString("bin"), AVERAGE_AW30_ELEV_SIZE,
                            QGeoMapType::CustomMap, parent) {}

    int        long2tileX(const double lon, const int z) const override;
    int        lat2tileY(const double lat, const int z) const override;
    QGCTileSet getTileCount(const int zoom, const double topleftLon, const double topleftLat,
                            const double bottomRightLon, const double bottomRightLat)const override;

    TerrainTile* newTerrainTile(QByteArray buf) override;
    QByteArray   serialize(QByteArray buf) override;

  protected:
    QString _getURL(const int x, const int y, const int zoom,
                    QNetworkAccessManager* networkManager) override;

  private:
    double tilex2long(const int x,const int z) const;
    double tiley2lat(const int y, const int z) const;
};
