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

const quint32 AVERAGE_AIRMAP_ELEV_SIZE = 2786;
const quint32 AVERAGE_AW30_ELEV_SIZE  = 25000000;

//-----------------------------------------------------------------------------
const double srtm1TileSize = 0.01;

class ElevationProvider : public MapProvider {
    Q_OBJECT
  public:
    ElevationProvider(QString imageFormat, quint32 averageSize,
                      QGeoMapType::MapStyle mapType, QObject* parent);

    ~ElevationProvider();

    bool _isElevationProvider() { return true; }

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

    int        long2tileX(double lon, int z);
    int        lat2tileY(double lat, int z);
    QGCTileSet getTileCount(int zoom, double topleftLon, double topleftLat,
                            double bottomRightLon, double bottomRightLat);

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

    // int long2tileX(double lon, int z);
    // int lat2tileY(double lat, int z);
    // QGCTileSet getTileCount(int zoom, double topleftLon,
    //                                 double topleftLat, double bottomRightLon,
    //                                 double bottomRightLat);

    TerrainTile* newTerrainTile(QByteArray buf);
    QByteArray   serialize(QByteArray buf);

  protected:
    QString _getURL(int x, int y, int zoom,
                    QNetworkAccessManager* networkManager);

  private:
    double tilex2long(int x, int z);
    double tiley2lat(int y, int z);
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
    double tilex2long(int x, int z);
    double tiley2lat(int y, int z);
};
