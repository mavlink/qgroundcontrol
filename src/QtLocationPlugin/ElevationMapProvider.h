#pragma once

#include "MapProvider.h"

#include <QByteArray>
#include <QMutex>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QPoint>
#include <QString>

const quint32 AVERAGE_AIRMAP_ELEV_SIZE = 2786;
//-----------------------------------------------------------------------------
const double srtm1TileSize = 0.01;

class ElevationProvider : public MapProvider {
    Q_OBJECT
  public:
    ElevationProvider(QString imageFormat, quint32 averageSize,
                      QGeoMapType::MapStyle mapType, QObject* parent);

    ~ElevationProvider();

	bool _isElevationProvider(){return true;}

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
                            QGeoMapType::StreetMap, parent) {}

    int long2tileX(double lon, int z);
    int lat2tileY(double lat, int z);


  protected:
    QString _getURL(int x, int y, int zoom,
                    QNetworkAccessManager* networkManager);
};

