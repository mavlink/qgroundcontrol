#pragma once

#include "MapProvider.h"

#include <QByteArray>
#include <QMutex>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QPoint>
#include <QString>

class GoogleMapProvider : public MapProvider {
    Q_OBJECT
  public:
    GoogleMapProvider(QString imageFormat, quint32 averageSize,
                      QGeoMapType::MapStyle mapType, QObject* parent);

    ~GoogleMapProvider();

    // Google Specific private slots
  private slots:
    void _networkReplyError(QNetworkReply::NetworkError error);
    void _googleVersionCompleted();
    void _replyDestroyed();

  protected:
    // Define the url to Request
    virtual QString _getURL(int x, int y, int zoom,
                            QNetworkAccessManager* networkManager) = 0;

    // Google Specific private methods
    void _getSecGoogleWords(int x, int y, QString& sec1, QString& sec2);
    void _tryCorrectGoogleVersions(QNetworkAccessManager* networkManager);

    // Google Specific attributes
    bool           _googleVersionRetrieved;
    QNetworkReply* _googleReply;
    QMutex         _googleVersionMutex;
    QString        _versionGoogleMap;
    QString        _versionGoogleSatellite;
    QString        _versionGoogleLabels;
    QString        _versionGoogleTerrain;
    QString        _versionGoogleHybrid;
    QString        _secGoogleWord;
};

// NoMap = 0,
// StreetMap,
// SatelliteMapDay,
// SatelliteMapNight,
// TerrainMap,
// HybridMap,
// TransitMap,
// GrayStreetMap,
// PedestrianMap,
// CarNavigationMap,
// CycleMap,
// CustomMap = 100

const quint32 AVERAGE_GOOGLE_STREET_MAP  = 4913;
const quint32 AVERAGE_GOOGLE_SAT_MAP     = 56887;
const quint32 AVERAGE_GOOGLE_TERRAIN_MAP = 19391;

// -----------------------------------------------------------
// Google Street Map

class GoogleStreetMapProvider : public GoogleMapProvider {
    Q_OBJECT
  public:
    GoogleStreetMapProvider(QObject* parent)
        : GoogleMapProvider(QString("png"), AVERAGE_GOOGLE_STREET_MAP,
                            QGeoMapType::StreetMap, parent) {}

  protected:
    QString _getURL(int x, int y, int zoom,
                    QNetworkAccessManager* networkManager);
};

// -----------------------------------------------------------
// Google Street Map

class GoogleSatelliteMapProvider : public GoogleMapProvider {
    Q_OBJECT
  public:
    GoogleSatelliteMapProvider(QObject* parent)
        : GoogleMapProvider(QString("jpg"), AVERAGE_GOOGLE_SAT_MAP,
                            QGeoMapType::SatelliteMapDay, parent) {}

  protected:
    QString _getURL(int x, int y, int zoom,
                    QNetworkAccessManager* networkManager);
};

// -----------------------------------------------------------
// Google Labels Map

class GoogleLabelsMapProvider : public GoogleMapProvider {
    Q_OBJECT
  public:
    GoogleLabelsMapProvider(QObject* parent)
        : GoogleMapProvider(QString("png"), AVERAGE_TILE_SIZE,
                            QGeoMapType::CustomMap, parent) {}

  protected:
    QString _getURL(int x, int y, int zoom,
                    QNetworkAccessManager* networkManager);
};

// -----------------------------------------------------------
// Google Terrain Map

class GoogleTerrainMapProvider : public GoogleMapProvider {
    Q_OBJECT
  public:
    GoogleTerrainMapProvider(QObject* parent)
        : GoogleMapProvider(QString("png"), AVERAGE_GOOGLE_TERRAIN_MAP,
                            QGeoMapType::TerrainMap, parent) {}

  protected:
    QString _getURL(int x, int y, int zoom,
                    QNetworkAccessManager* networkManager);
};

// -----------------------------------------------------------
// Google Hybrid Map

class GoogleHybridMapProvider : public GoogleMapProvider {
    Q_OBJECT
  public:
    GoogleHybridMapProvider(QObject* parent)
        : GoogleMapProvider(QString("png"), AVERAGE_GOOGLE_SAT_MAP,
                            QGeoMapType::HybridMap, parent) {}

  protected:
    QString _getURL(int x, int y, int zoom,
                    QNetworkAccessManager* networkManager);
};
