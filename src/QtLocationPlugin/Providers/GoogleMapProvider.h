/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
protected:
    GoogleMapProvider(const QString &mapName, const QString &versionRequest, const QString &version, const QString &imageFormat, quint32 averageSize,
                      QGeoMapType::MapStyle mapType)
        : MapProvider(
            mapName,
            QStringLiteral("https://www.google.com/maps/preview"),
            imageFormat,
            averageSize,
            mapType)
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
public:
    GoogleStreetMapProvider()
        : GoogleMapProvider(
            QStringLiteral("Google Street Map"),
            QStringLiteral("lyrs"),
            QStringLiteral("m"),
            QStringLiteral("png"),
            AVERAGE_GOOGLE_STREET_MAP,
            QGeoMapType::StreetMap) {}
};

class GoogleSatelliteMapProvider : public GoogleMapProvider
{
public:
    GoogleSatelliteMapProvider()
        : GoogleMapProvider(
            QStringLiteral("Google Satellite"),
            QStringLiteral("lyrs"),
            QStringLiteral("s"),
            QStringLiteral("jpg"),
            AVERAGE_GOOGLE_SAT_MAP,
            QGeoMapType::SatelliteMapDay) {}
};

class GoogleLabelsMapProvider : public GoogleMapProvider
{
public:
    GoogleLabelsMapProvider()
        : GoogleMapProvider(
            QStringLiteral("Google Labels"),
            QStringLiteral("lyrs"),
            QStringLiteral("h"),
            QStringLiteral("png"),
            AVERAGE_TILE_SIZE,
            QGeoMapType::CustomMap) {}
};

class GoogleTerrainMapProvider : public GoogleMapProvider
{
public:
    GoogleTerrainMapProvider()
        : GoogleMapProvider(
            QStringLiteral("Google Terrain"),
            QStringLiteral("v"),
            QStringLiteral("t,r"),
            QStringLiteral("png"),
            AVERAGE_GOOGLE_TERRAIN_MAP,
            QGeoMapType::TerrainMap) {}
};

class GoogleHybridMapProvider : public GoogleMapProvider
{
public:
    GoogleHybridMapProvider()
        : GoogleMapProvider(
            QStringLiteral("Google Hybrid"),
            QStringLiteral("lyrs"),
            QStringLiteral("y"),
            QStringLiteral("png"),
            AVERAGE_GOOGLE_SAT_MAP,
            QGeoMapType::HybridMap) {}
};
