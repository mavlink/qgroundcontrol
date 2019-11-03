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

    // Used to create a new TerrainTile associated to the provider
    virtual TerrainTile* newTerrainTile(QByteArray buf) = 0;

  protected:
    // Define the url to Request
    virtual QString _getURL(int x, int y, int zoom,
                            QNetworkAccessManager* networkManager) = 0;
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

    TerrainTile* newTerrainTile(QByteArray buf);
    QByteArray   serialize(QByteArray buf);

  protected:
    QString _getURL(int x, int y, int zoom,
                    QNetworkAccessManager* networkManager);
};

// -----------------------------------------------------------
// Geotiff Elevation

class GeotiffElevationProvider : public ElevationProvider {
    Q_OBJECT
  public:
    GeotiffElevationProvider(QObject* parent)
        : ElevationProvider(QString("bin"), AVERAGE_AIRMAP_ELEV_SIZE,
                            QGeoMapType::CustomMap, parent) {}

    TerrainTile* newTerrainTile(QByteArray buf);
    QByteArray   serialize(QByteArray buf);

  protected:
    QString _getURL(int x, int y, int zoom,
                    QNetworkAccessManager* networkManager);

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

    int        long2tileX(double lon, int z);
    int        lat2tileY(double lat, int z);
    QGCTileSet getTileCount(int zoom, double topleftLon, double topleftLat,
                            double bottomRightLon, double bottomRightLat);

    TerrainTile* newTerrainTile(QByteArray buf);
    QByteArray   serialize(QByteArray buf);

  protected:
    QString _getURL(int x, int y, int zoom,
                    QNetworkAccessManager* networkManager);

  private:
    double tilex2long(const int x,const int z) const;
    double tiley2lat(const int y, const int z) const;
};
