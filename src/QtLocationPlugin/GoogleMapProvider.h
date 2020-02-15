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

#include <QNetworkReply>
#include <QMutex>

class GoogleMapProvider : public MapProvider {
    Q_OBJECT

public:
    GoogleMapProvider(const QString& imageFormat, const quint32 averageSize,
                      const QGeoMapType::MapStyle _mapType, QObject* parent = nullptr);

    ~GoogleMapProvider();

    // Google Specific private slots
private slots:
    void _networkReplyError(QNetworkReply::NetworkError error);
    void _googleVersionCompleted();
    void _replyDestroyed();

protected:
    // Google Specific private methods
    void _getSecGoogleWords(const int x, const int y, QString& sec1, QString& sec2) const;
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

static const quint32 AVERAGE_GOOGLE_STREET_MAP  = 4913;
static const quint32 AVERAGE_GOOGLE_SAT_MAP     = 56887;
static const quint32 AVERAGE_GOOGLE_TERRAIN_MAP = 19391;

// -----------------------------------------------------------
// Google Street Map

class GoogleStreetMapProvider : public GoogleMapProvider {
    Q_OBJECT

public:
    GoogleStreetMapProvider(QObject* parent = nullptr)
        : GoogleMapProvider(QStringLiteral("png"), AVERAGE_GOOGLE_STREET_MAP, QGeoMapType::StreetMap, parent) {}

protected:
     QString _getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) override;
};

// -----------------------------------------------------------
// Google Street Map

class GoogleSatelliteMapProvider : public GoogleMapProvider {
    Q_OBJECT

public:
    GoogleSatelliteMapProvider(QObject* parent = nullptr)
        : GoogleMapProvider(QStringLiteral("jpg"), AVERAGE_GOOGLE_SAT_MAP,
                            QGeoMapType::SatelliteMapDay, parent) {}

protected:
    QString _getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) override;
};

// -----------------------------------------------------------
// Google Labels Map

class GoogleLabelsMapProvider : public GoogleMapProvider {
    Q_OBJECT

public:
    GoogleLabelsMapProvider(QObject* parent = nullptr)
        : GoogleMapProvider(QStringLiteral("png"), AVERAGE_TILE_SIZE, QGeoMapType::CustomMap, parent) {}

protected:
    QString _getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) override;
};

// -----------------------------------------------------------
// Google Terrain Map

class GoogleTerrainMapProvider : public GoogleMapProvider {
    Q_OBJECT

public:
    GoogleTerrainMapProvider(QObject* parent = nullptr)
        : GoogleMapProvider(QStringLiteral("png"), AVERAGE_GOOGLE_TERRAIN_MAP, QGeoMapType::TerrainMap, parent) {}

protected:
    QString _getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) override;
};

// -----------------------------------------------------------
// Google Hybrid Map

class GoogleHybridMapProvider : public GoogleMapProvider {
    Q_OBJECT

public:
    GoogleHybridMapProvider(QObject* parent = nullptr)
        : GoogleMapProvider(QStringLiteral("png"), AVERAGE_GOOGLE_SAT_MAP, QGeoMapType::HybridMap, parent) {}

protected:
    QString _getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) override;
};
