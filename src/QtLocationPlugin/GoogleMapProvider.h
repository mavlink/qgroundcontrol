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

// h: roads only
// m: standard roadmap
// p: terrain
// r: somehow altered roadmap
// s: satellite only
// t: terrain only
// y: hybrid (s,h)
// traffic
// transit
// bike
// mt.google.com: mt0, mt1, mt2 mt3
// size: 256x256
// maxZoom: 20

static constexpr const quint32 AVERAGE_GOOGLE_STREET_MAP  = 4913;
static constexpr const quint32 AVERAGE_GOOGLE_SAT_MAP     = 56887;
static constexpr const quint32 AVERAGE_GOOGLE_TERRAIN_MAP = 19391;

class GoogleMapProvider : public MapProvider
{
    Q_OBJECT

protected:
    GoogleMapProvider(const QString& versionRequest, const QString& version, const QString& imageFormat, quint32 averageSize,
                      QGeoMapType::MapStyle mapType, QObject* parent = nullptr)
        : MapProvider(QStringLiteral("https://www.google.com/maps/preview"), imageFormat, averageSize, mapType, parent)
        , _versionRequest(versionRequest)
        , _version(version) {}

private:
    void _getSecGoogleWords(int x, int y, QString& sec1, QString& sec2) const;
    QString _getURL(int x, int y, int zoom) const final;

    const QString _versionRequest;
    const QString _version;
    const QString _mapUrl = QStringLiteral("http://mt%1.google.com/vt/%2=%3&hl=%4&x=%5%6&y=%7&z=%8&s=%9&scale=%10");
    const QString _secGoogleWord = QStringLiteral("Galileo");
    const QString _scale = QStringLiteral("1");
};

class GoogleStreetMapProvider : public GoogleMapProvider
{
    Q_OBJECT

public:
    GoogleStreetMapProvider(QObject* parent = nullptr)
        : GoogleMapProvider(QStringLiteral("lyrs"), QStringLiteral("m"), QStringLiteral("png"), AVERAGE_GOOGLE_STREET_MAP, QGeoMapType::StreetMap, parent) {}
};

class GoogleSatelliteMapProvider : public GoogleMapProvider
{
    Q_OBJECT

public:
    GoogleSatelliteMapProvider(QObject* parent = nullptr)
        : GoogleMapProvider(QStringLiteral("lyrs"), QStringLiteral("s"), QStringLiteral("jpg"), AVERAGE_GOOGLE_SAT_MAP,
                            QGeoMapType::SatelliteMapDay, parent) {}
};

class GoogleLabelsMapProvider : public GoogleMapProvider
{
    Q_OBJECT

public:
    GoogleLabelsMapProvider(QObject* parent = nullptr)
        : GoogleMapProvider(QStringLiteral("lyrs"), QStringLiteral("h"), QStringLiteral("png"), AVERAGE_TILE_SIZE, QGeoMapType::CustomMap, parent) {}
};

class GoogleTerrainMapProvider : public GoogleMapProvider
{
    Q_OBJECT

public:
    GoogleTerrainMapProvider(QObject* parent = nullptr)
        : GoogleMapProvider(QStringLiteral("v"), QStringLiteral("t,r"), QStringLiteral("png"), AVERAGE_GOOGLE_TERRAIN_MAP, QGeoMapType::TerrainMap, parent) {}
};

class GoogleHybridMapProvider : public GoogleMapProvider
{
    Q_OBJECT

public:
    GoogleHybridMapProvider(QObject* parent = nullptr)
        : GoogleMapProvider(QStringLiteral("lyrs"), QStringLiteral("y"), QStringLiteral("png"), AVERAGE_GOOGLE_SAT_MAP, QGeoMapType::HybridMap, parent) {}
};
