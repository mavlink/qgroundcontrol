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

class CustomURLMapProvider : public MapProvider
{
public:
    CustomURLMapProvider()
        : MapProvider(
            QStringLiteral("CustomURL Custom"),
            QStringLiteral(""),
            QStringLiteral(""),
            AVERAGE_TILE_SIZE,
            QGeoMapType::CustomMap) {}

private:
    QString _getURL(int x, int y, int zoom) const final;
};

class CyberJapanMapProvider : public MapProvider
{
protected:
    CyberJapanMapProvider(const QString &mapName, const QString &mapTypeId, const QString &imageFormat)
        : MapProvider(
            mapName,
            QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/std"),
            imageFormat,
            AVERAGE_TILE_SIZE,
            QGeoMapType::StreetMap)
        , _mapTypeId(mapName) {}

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapTypeId;
    const QString _mapUrl = QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/%1/%2/%3/%4.%5");
};

class JapanStdMapProvider : public CyberJapanMapProvider
{
public:
    JapanStdMapProvider()
        : CyberJapanMapProvider(
            QStringLiteral("Japan-GSI Contour"),
            QStringLiteral("std"),
            QStringLiteral("png")) {}
};

class JapanSeamlessMapProvider : public CyberJapanMapProvider
{
public:
    JapanSeamlessMapProvider()
        : CyberJapanMapProvider(
            QStringLiteral("Japan-GSI Seamless"),
            QStringLiteral("seamlessphoto"),
            QStringLiteral("jpg")) {}
};

class JapanAnaglyphMapProvider : public CyberJapanMapProvider
{
public:
    JapanAnaglyphMapProvider()
        : CyberJapanMapProvider(
            QStringLiteral("Japan-GSI Anaglyph"),
            QStringLiteral("anaglyphmap_color"),
            QStringLiteral("png")) {}
};

class JapanSlopeMapProvider : public CyberJapanMapProvider
{
public:
    JapanSlopeMapProvider()
        : CyberJapanMapProvider(
            QStringLiteral("Japan-GSI Slope"),
            QStringLiteral("slopemap"),
            QStringLiteral("png")) {}
};

class JapanReliefMapProvider : public CyberJapanMapProvider
{
public:
    JapanReliefMapProvider()
        : CyberJapanMapProvider(
            QStringLiteral("Japan-GSI Relief"),
            QStringLiteral("relief"),
            QStringLiteral("png")) {}
};

class LINZBasemapMapProvider : public MapProvider
{
public:
    LINZBasemapMapProvider()
        : MapProvider(
            QStringLiteral("LINZ Basemap"),
            QStringLiteral("https://basemaps.linz.govt.nz/v1/tiles/aerial"),
            QStringLiteral("png"),
            AVERAGE_TILE_SIZE,
            QGeoMapType::SatelliteMapDay) {}

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapUrl = QStringLiteral("https://basemaps.linz.govt.nz/v1/tiles/aerial/EPSG:3857/%1/%2/%3.%4?api=d01ev80nqcjxddfvc6amyvkk1ka");
};

class StatkartMapProvider : public MapProvider
{
protected:
    StatkartMapProvider(const QString &mapName, const QString &mapTypeId)
        : MapProvider(
            mapName,
            QStringLiteral("https://norgeskart.no/"),
            QStringLiteral("png"),
            AVERAGE_TILE_SIZE,
            QGeoMapType::StreetMap)
        , _mapTypeId(mapName) {}

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapTypeId;
    const QString _mapUrl = QStringLiteral("https://cache.kartverket.no/v1/wmts/1.0.0/topo/default/webmercator/%1/%2/%3");
};

class StatkartTopoMapProvider : public StatkartMapProvider
{
public:
    StatkartTopoMapProvider()
        : StatkartMapProvider(
            QStringLiteral("Statkart Topo"),
            QStringLiteral("topo4")) {}
};

class StatkartBaseMapProvider : public StatkartMapProvider
{
public:
    StatkartBaseMapProvider()
        : StatkartMapProvider(
            QStringLiteral("Statkart Basemap"),
            QStringLiteral("norgeskart_bakgrunn")) {}
};

class SvalbardMapProvider : public MapProvider
{
public:
    SvalbardMapProvider()
        : MapProvider(
            QStringLiteral("Svalbard Topo"),
            QStringLiteral("https://www.npolar.no/"),
            QStringLiteral("png"),
            AVERAGE_TILE_SIZE,
            QGeoMapType::StreetMap) {}

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapUrl = QStringLiteral("https://geodata.npolar.no/arcgis/rest/services/Basisdata/NP_Basiskart_Svalbard_WMTS_3857/MapServer/WMTS/tile/1.0.0/Basisdata_NP_Basiskart_Svalbard_WMTS_3857/default/default028mm/%1/%2/%3");
};


class EniroMapProvider : public MapProvider
{
public:
    EniroMapProvider()
        : MapProvider(
            QStringLiteral("Eniro Topo"),
            QStringLiteral("https://www.eniro.se/"),
            QStringLiteral("png"),
            AVERAGE_TILE_SIZE,
            QGeoMapType::StreetMap) {}

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapUrl = QStringLiteral("http://map.eniro.com/geowebcache/service/tms1.0.0/map/%1/%2/%3.%4");
};

class MapQuestMapProvider : public MapProvider
{
protected:
    MapQuestMapProvider(const QString &mapName, const QString &mapTypeId, QGeoMapType::MapStyle mapType)
        : MapProvider(
            mapName,
            QStringLiteral("https://mapquest.com"),
            QStringLiteral("jpg"),
            AVERAGE_TILE_SIZE,
            mapType)
        , _mapTypeId(mapName) {}

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapTypeId;
    const QString _mapUrl = QStringLiteral("http://otile%1.mqcdn.com/tiles/1.0.0/%2/%3/%4/%5.%6");
};

class MapQuestMapMapProvider : public MapQuestMapProvider
{
public:
    MapQuestMapMapProvider()
        : MapQuestMapProvider(
            QStringLiteral("MapQuest Map"),
            QStringLiteral("map"),
            QGeoMapType::StreetMap) {}
};

class MapQuestSatMapProvider : public MapQuestMapProvider
{
public:
    MapQuestSatMapProvider()
        : MapQuestMapProvider(
            QStringLiteral("MapQuest Sat"),
            QStringLiteral("sat"),
            QGeoMapType::SatelliteMapDay) {}
};

class VWorldMapProvider : public MapProvider
{
protected:
    VWorldMapProvider(const QString &mapName, const QString &mapTypeId, const QString &imageFormat, quint32 averageSize, QGeoMapType::MapStyle mapStyle)
        : MapProvider(
            mapName,
            QStringLiteral("www.vworld.kr"),
            imageFormat,
            averageSize,
            mapStyle)
        , _mapTypeId(mapName) {}

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapTypeId;
    const QString _mapUrl = QStringLiteral("http://api.vworld.kr/req/wmts/1.0.0/%1/%2/%3/%4/%5.%6");
};

class VWorldStreetMapProvider : public VWorldMapProvider
{
public:
    VWorldStreetMapProvider()
        : VWorldMapProvider(
            QStringLiteral("VWorld Street Map"),
            QStringLiteral("Base"),
            QStringLiteral("png"),
            AVERAGE_TILE_SIZE,
            QGeoMapType::StreetMap) {}
};

class VWorldSatMapProvider : public VWorldMapProvider
{
public:
    VWorldSatMapProvider()
        : VWorldMapProvider(
            QStringLiteral("VWorld Satellite Map"),
            QStringLiteral("Satellite"),
            QStringLiteral("jpeg"),
            AVERAGE_TILE_SIZE,
            QGeoMapType::SatelliteMapDay) {}
};
