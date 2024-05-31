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
    Q_OBJECT

public:
    CustomURLMapProvider(QObject* parent = nullptr)
        : MapProvider(QStringLiteral(""), QStringLiteral(""),
                      AVERAGE_TILE_SIZE, QGeoMapType::CustomMap, parent) {}

private:
    QString _getURL(int x, int y, int zoom) const final;
};

class CyberJapanMapProvider : public MapProvider
{
    Q_OBJECT

protected:
    CyberJapanMapProvider(const QString &mapName, const QString& imageFormat, QObject* parent = nullptr)
        : MapProvider(QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/std"), imageFormat,
                      AVERAGE_TILE_SIZE, QGeoMapType::StreetMap, parent)
        , _mapName(mapName) {}

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapName;
    const QString _mapUrl = QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/%1/%2/%3/%4.%5");
};

class JapanStdMapProvider : public CyberJapanMapProvider
{
    Q_OBJECT

public:
    JapanStdMapProvider(QObject* parent = nullptr)
        : CyberJapanMapProvider(QStringLiteral("std"), QStringLiteral("png"), parent) {}
};

class JapanSeamlessMapProvider : public CyberJapanMapProvider
{
    Q_OBJECT

public:
    JapanSeamlessMapProvider(QObject* parent = nullptr)
        : CyberJapanMapProvider(QStringLiteral("seamlessphoto"), QStringLiteral("jpg"), parent) {}
};

class JapanAnaglyphMapProvider : public CyberJapanMapProvider
{
    Q_OBJECT

public:
    JapanAnaglyphMapProvider(QObject* parent = nullptr)
        : CyberJapanMapProvider(QStringLiteral("anaglyphmap_color"), QStringLiteral("png"), parent) {}
};

class JapanSlopeMapProvider : public CyberJapanMapProvider
{
    Q_OBJECT

public:
    JapanSlopeMapProvider(QObject* parent = nullptr)
        : CyberJapanMapProvider(QStringLiteral("slopemap"), QStringLiteral("png"), parent) {}
};

class JapanReliefMapProvider : public CyberJapanMapProvider
{
    Q_OBJECT

public:
    JapanReliefMapProvider(QObject* parent = nullptr)
        : CyberJapanMapProvider(QStringLiteral("relief"), QStringLiteral("png"), parent) {}
};

class LINZBasemapMapProvider : public MapProvider
{
    Q_OBJECT

public:
    LINZBasemapMapProvider(QObject* parent = nullptr)
        : MapProvider(QStringLiteral("https://basemaps.linz.govt.nz/v1/tiles/aerial"), QStringLiteral("png"),
                      AVERAGE_TILE_SIZE, QGeoMapType::SatelliteMapDay, parent) {}

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapUrl = QStringLiteral("https://basemaps.linz.govt.nz/v1/tiles/aerial/EPSG:3857/%1/%2/%3.%4?api=d01ev80nqcjxddfvc6amyvkk1ka");
};

class StatkartMapProvider : public MapProvider
{
    Q_OBJECT

protected:
    StatkartMapProvider(const QString &mapName, QObject* parent = nullptr)
        : MapProvider(QStringLiteral("https://norgeskart.no/"), QStringLiteral("png"),
                      AVERAGE_TILE_SIZE, QGeoMapType::StreetMap, parent)
        , _mapName(mapName) {}

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapName;
    const QString _mapUrl = QStringLiteral("http://opencache.statkart.no/gatekeeper/gk/gk.open_gmaps?layers=%1&zoom=%2&x=%3&y=%4");
};

class StatkartTopoMapProvider : public StatkartMapProvider
{
    Q_OBJECT

public:
    StatkartTopoMapProvider(QObject* parent = nullptr)
        : StatkartMapProvider(QStringLiteral("topo4"), parent) {}
};

class StatkartBaseMapProvider : public StatkartMapProvider
{
    Q_OBJECT

public:
    StatkartBaseMapProvider(QObject* parent = nullptr)
        : StatkartMapProvider(QStringLiteral("norgeskart_bakgrunn"), parent) {}
};

class EniroMapProvider : public MapProvider
{
    Q_OBJECT

public:
    EniroMapProvider(QObject* parent = nullptr)
        : MapProvider(QStringLiteral("https://www.eniro.se/"), QStringLiteral("png"),
                      AVERAGE_TILE_SIZE, QGeoMapType::StreetMap, parent) {}

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapUrl = QStringLiteral("http://map.eniro.com/geowebcache/service/tms1.0.0/map/%1/%2/%3.%4");
};

class MapQuestMapProvider : public MapProvider
{
    Q_OBJECT

protected:
    MapQuestMapProvider(const QString &mapName, QGeoMapType::MapStyle mapType, QObject* parent = nullptr)
        : MapProvider(QStringLiteral("https://mapquest.com"), QStringLiteral("jpg"),
                      AVERAGE_TILE_SIZE, mapType, parent)
        , _mapName(mapName) {}

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapName;
    const QString _mapUrl = QStringLiteral("http://otile%1.mqcdn.com/tiles/1.0.0/%2/%3/%4/%5.%6");
};

class MapQuestMapMapProvider : public MapQuestMapProvider
{
    Q_OBJECT

public:
    MapQuestMapMapProvider(QObject* parent = nullptr)
        : MapQuestMapProvider(QStringLiteral("map"), QGeoMapType::StreetMap, parent) {}
};

class MapQuestSatMapProvider : public MapQuestMapProvider
{
    Q_OBJECT

public:
    MapQuestSatMapProvider(QObject* parent = nullptr)
        : MapQuestMapProvider(QStringLiteral("sat"), QGeoMapType::SatelliteMapDay, parent) {}
};

class VWorldMapProvider : public MapProvider
{
    Q_OBJECT

protected:
    VWorldMapProvider(const QString &mapName, const QString& imageFormat, quint32 averageSize, QGeoMapType::MapStyle mapStyle, QObject* parent = nullptr)
        : MapProvider(QStringLiteral("www.vworld.kr"), imageFormat, averageSize, mapStyle, parent)
        , _mapName(mapName) {}

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapName;
    const QString _mapUrl = QStringLiteral("http://api.vworld.kr/req/wmts/1.0.0/%1/%2/%3/%4/%5.%6");
};

class VWorldStreetMapProvider : public VWorldMapProvider
{
    Q_OBJECT

public:
    VWorldStreetMapProvider(QObject* parent = nullptr)
        : VWorldMapProvider(QStringLiteral("Base"), QStringLiteral("png"), AVERAGE_TILE_SIZE, QGeoMapType::StreetMap, parent) {}
};

class VWorldSatMapProvider : public VWorldMapProvider
{
    Q_OBJECT

public:
    VWorldSatMapProvider(QObject* parent = nullptr)
        : VWorldMapProvider(QStringLiteral("Satellite"), QStringLiteral("jpeg"), AVERAGE_TILE_SIZE, QGeoMapType::SatelliteMapDay, parent) {}
};
