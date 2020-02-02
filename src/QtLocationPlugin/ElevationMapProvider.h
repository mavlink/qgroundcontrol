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
//-----------------------------------------------------------------------------
static const double srtm1TileSize = 0.01;

class ElevationProvider : public MapProvider {
    Q_OBJECT
  public:
    ElevationProvider(const QString& imageFormat, quint32 averageSize,
                      QGeoMapType::MapStyle mapType, QObject* parent = nullptr);

    virtual bool _isElevationProvider() const override { return true; }
};

// -----------------------------------------------------------
// Airmap Elevation

class AirmapElevationProvider : public ElevationProvider {
    Q_OBJECT
  public:
    AirmapElevationProvider(QObject* parent = nullptr)
        : ElevationProvider(QStringLiteral("bin"), AVERAGE_AIRMAP_ELEV_SIZE,
                            QGeoMapType::StreetMap, parent) {}

    int long2tileX(const double lon, const int z) const override;

    int lat2tileY(const double lat, const int z) const override;

    QGCTileSet getTileCount(const int zoom, const double topleftLon,
                            const double topleftLat, const double bottomRightLon,
                            const double bottomRightLat) const override;

  protected:
    QString _getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) override;
};

