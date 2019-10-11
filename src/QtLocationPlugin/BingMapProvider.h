#pragma once

#include "MapProvider.h"

#include <QByteArray>
#include <QMutex>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QPoint>
#include <QString>

class BingMapProvider : public MapProvider {
    Q_OBJECT
  public:
    BingMapProvider(QString imageFormat, quint32 averageSize,
                    QGeoMapType::MapStyle mapType, QObject* parent);

    ~BingMapProvider();

  protected:
    // Define the url to Request
    virtual QString _getURL(int x, int y, int zoom,
                            QNetworkAccessManager* networkManager) = 0;

    const QString _versionBingMaps = "563";
};

const quint32 AVERAGE_BING_STREET_MAP = 1297;
const quint32 AVERAGE_BING_SAT_MAP    = 19597;

// -----------------------------------------------------------
// Bing Road Map

class BingRoadMapProvider : public BingMapProvider {
    Q_OBJECT
  public:
    BingRoadMapProvider(QObject* parent)
        : BingMapProvider(QString("png"), AVERAGE_BING_STREET_MAP, QGeoMapType::StreetMap,
                          parent) {}

  protected:
    QString _getURL(int x, int y, int zoom,
                    QNetworkAccessManager* networkManager);
};

// -----------------------------------------------------------
// Bing Satellite Map

class BingSatelliteMapProvider : public BingMapProvider {
    Q_OBJECT
  public:
    BingSatelliteMapProvider(QObject* parent)
        : BingMapProvider(QString("jpg"), AVERAGE_BING_SAT_MAP, QGeoMapType::SatelliteMapDay,
                          parent) {}

  protected:
    QString _getURL(int x, int y, int zoom,
                    QNetworkAccessManager* networkManager);
};

// -----------------------------------------------------------
// Bing Hybrid Map

class BingHybridMapProvider : public BingMapProvider {
    Q_OBJECT
  public:
    BingHybridMapProvider(QObject* parent)
        : BingMapProvider(QString("jpg"),AVERAGE_BING_SAT_MAP, QGeoMapType::HybridMap,
                          parent) {}

  protected:
    QString _getURL(int x, int y, int zoom,
                    QNetworkAccessManager* networkManager);
};
