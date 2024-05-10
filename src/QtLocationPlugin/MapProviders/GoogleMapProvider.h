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

class GoogleMapProvider : public MapProvider
{
protected:
    GoogleMapProvider(const QString& mapName, const QString& imageFormat, quint32 averageSize,
                      QGeoMapType::MapStyle mapStyle, const QString &mapUrl,
                      const QString &version)
    : MapProvider(mapName, QStringLiteral("https://www.google.com/maps/preview"), imageFormat, averageSize, mapStyle)
    , m_mapUrl(mapUrl)
    , m_version(version) {}

    ~GoogleMapProvider() = default;

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString m_mapUrl;
    const QString m_version;
};

class GoogleStreetMapProvider : public GoogleMapProvider
{
public:
    GoogleStreetMapProvider()
        : GoogleMapProvider(
            "Google Street Map",
            QStringLiteral("png"),
            4913,
            QGeoMapType::StreetMap,
            QStringLiteral("http://mt.google.com/vt/lyrs=%1&hl=%2&x=%3&y=%4&z=%5"),
            QStringLiteral("m")) {}
};

class GoogleSatelliteMapProvider : public GoogleMapProvider
{
public:
    GoogleSatelliteMapProvider()
        : GoogleMapProvider(
            "Google Satellite",
            QStringLiteral("jpg"),
            56887,
            QGeoMapType::SatelliteMapDay,
            QStringLiteral("http://mt.google.com/vt/lyrs=%1&hl=%2&x=%3&y=%4&z=%5"),
            QStringLiteral("s")) {}
};

class GoogleLabelsMapProvider : public GoogleMapProvider
{
public:
    GoogleLabelsMapProvider()
        : GoogleMapProvider(
            "Google Labels",
            QStringLiteral("png"),
            13652,
            QGeoMapType::CustomMap,
            QStringLiteral("http://mt.google.com/vt/lyrs=%1&hl=%2&x=%3&y=%4&z=%5"),
            QStringLiteral("h")) {}
};

class GoogleTerrainMapProvider : public GoogleMapProvider
{
public:
    GoogleTerrainMapProvider()
        : GoogleMapProvider(
            "Google Terrain",
            QStringLiteral("png"),
            19391,
            QGeoMapType::TerrainMap,
            QStringLiteral("http://mt.google.com/vt/v=%1&hl=%2&x=%3&y=%4&z=%5"),
            QStringLiteral("p")) {} // ("t,r")
};

class GoogleHybridMapProvider : public GoogleMapProvider
{
public:
    GoogleHybridMapProvider()
        : GoogleMapProvider(
            "Google Hybrid",
            QStringLiteral("png"),
            56887,
            QGeoMapType::HybridMap,
            QStringLiteral("http://mt.google.com/vt/lyrs=%1&hl=%2&x=%3&y=%4&z=%5"),
            QStringLiteral("y")) {} // ("s,h")
};
