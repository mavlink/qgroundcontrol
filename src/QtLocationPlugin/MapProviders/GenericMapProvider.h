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

class CustomURLMapProvider : public MapProvider
{
public:
    CustomURLMapProvider()
        : MapProvider("CustomURL Custom", QStringLiteral(""), QStringLiteral(""),
                      AVERAGE_TILE_SIZE, QGeoMapType::CustomMap) {}

    QString _getURL(int x, int y, int zoom) const final;
};

class JapanStdMapProvider : public MapProvider
{
public:
    JapanStdMapProvider()
        : MapProvider("Japan-GSI Contour", QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/std"), QStringLiteral("png"),
                      AVERAGE_TILE_SIZE, QGeoMapType::StreetMap) {}

    QString _getURL(int x, int y, int zoom) const final;
};

class JapanSeamlessMapProvider : public MapProvider
{
public:
    JapanSeamlessMapProvider()
        : MapProvider("Japan-GSI Seamless", QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/seamlessphoto"), QStringLiteral("jpg"),
                      AVERAGE_TILE_SIZE, QGeoMapType::StreetMap) {}

    QString _getURL(int x, int y, int zoom) const final;
};

class JapanAnaglyphMapProvider : public MapProvider
{
public:
    JapanAnaglyphMapProvider()
        : MapProvider("Japan-GSI Anaglyph", QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/anaglyphmap_color"), QStringLiteral("png"),
                      AVERAGE_TILE_SIZE, QGeoMapType::StreetMap) {}

    QString _getURL(int x, int y, int zoom) const final;
};

class JapanSlopeMapProvider : public MapProvider
{
public:
    JapanSlopeMapProvider()
        : MapProvider("Japan-GSI Slope", QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/slopemap"), QStringLiteral("png"),
                      AVERAGE_TILE_SIZE, QGeoMapType::StreetMap) {}

    QString _getURL(int x, int y, int zoom) const final;
};

class JapanReliefMapProvider : public MapProvider
{
public:
    JapanReliefMapProvider()
        : MapProvider("Japan-GSI Relief", QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/relief"), QStringLiteral("png"),
                      AVERAGE_TILE_SIZE, QGeoMapType::StreetMap) {}

    QString _getURL(int x, int y, int zoom) const final;
};

class LINZBasemapMapProvider : public MapProvider
{
public:
    LINZBasemapMapProvider()
        : MapProvider("LINZ Basemap", QStringLiteral("https://basemaps.linz.govt.nz/v1/tiles/aerial"), QStringLiteral("png"),
                      AVERAGE_TILE_SIZE, QGeoMapType::SatelliteMapDay) {}

    QString _getURL(int x, int y, int zoom) const final;
};

class StatkartMapProvider : public MapProvider
{
public:
    StatkartMapProvider()
        : MapProvider("Statkart Topo", QStringLiteral("https://norgeskart.no/"), QStringLiteral("png"),
                      AVERAGE_TILE_SIZE, QGeoMapType::StreetMap) {}

    QString _getURL(int x, int y, int zoom) const final;
};

class StatkartBaseMapProvider : public MapProvider
{
public:
    StatkartBaseMapProvider()
        : MapProvider("Statkart Basemap", QStringLiteral("https://norgeskart.no/"), QStringLiteral("png"),
                      AVERAGE_TILE_SIZE, QGeoMapType::StreetMap) {}

    QString _getURL(int x, int y, int zoom) const final;
};

class EniroMapProvider : public MapProvider
{
public:
    EniroMapProvider()
        : MapProvider("Eniro Topo", QStringLiteral("https://www.eniro.se/"), QStringLiteral("png"),
                      AVERAGE_TILE_SIZE, QGeoMapType::StreetMap) {}

    QString _getURL(int x, int y, int zoom) const final;
};

class MapQuestMapMapProvider : public MapProvider
{
public:
    MapQuestMapMapProvider()
        : MapProvider("MapQuest Map", QStringLiteral("https://mapquest.com"), QStringLiteral("jpg"),
                      AVERAGE_TILE_SIZE, QGeoMapType::StreetMap) {}

    QString _getURL(int x, int y, int zoom) const final;
};

class MapQuestSatMapProvider : public MapProvider
{
public:
    MapQuestSatMapProvider()
        : MapProvider("MapQuest Sat", QStringLiteral("https://mapquest.com"), QStringLiteral("jpg"),
                      AVERAGE_TILE_SIZE, QGeoMapType::SatelliteMapDay) {}

    QString _getURL(int x, int y, int zoom) const final;
};

class VWorldMapProvider : public MapProvider
{
public:
    VWorldMapProvider(const QString& mapName, const QString& imageFormat, QGeoMapType::MapStyle mapStyle)
        : MapProvider(mapName, QStringLiteral("www.vworld.kr"), imageFormat,
                      AVERAGE_TILE_SIZE, mapStyle) {}

    QString _getURL(int x, int y, int zoom) const final;

private:
    const QString m_versionBingMaps = QStringLiteral("563");
};

class VWorldStreetMapProvider : public VWorldMapProvider
{
public:
    VWorldStreetMapProvider()
        : VWorldMapProvider("VWorld Street Map", QStringLiteral("png"), QGeoMapType::StreetMap) {}
};

class VWorldSatMapProvider : public VWorldMapProvider
{
public:
    VWorldSatMapProvider()
        : VWorldMapProvider("VWorld Satellite Map", QStringLiteral("jpeg"), QGeoMapType::SatelliteMapDay) {}
};
