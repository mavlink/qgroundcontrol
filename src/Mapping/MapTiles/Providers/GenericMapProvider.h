#pragma once

#include "MapProvider.h"

class CustomURLMapProvider : public MapProvider
{
public:
    CustomURLMapProvider()
        : MapProvider(QStringLiteral("CustomURL Custom"), QStringLiteral(""), QStringLiteral(""), QGC_AVERAGE_TILE_SIZE,
                      MapProvider::CustomMap)
    {}

    /// The URL template the user set
    QList<Fact*> sourceSettings() const final;

private:
    QString _getURL(int x, int y, int zoom) const final;
};

class CyberJapanMapProvider : public MapProvider
{
protected:
    CyberJapanMapProvider(const QString& mapName, const QString& mapTypeId, const QString& imageFormat)
        : MapProvider(mapName, QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/std"), imageFormat,
                      QGC_AVERAGE_TILE_SIZE, MapProvider::StreetMap)
        , _mapTypeId(mapTypeId)
    {}

public:
    QString attribution() const final { return QStringLiteral("Source: Geospatial Information Authority of Japan"); }

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapTypeId;
    const QString _mapUrl = QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/%1/%2/%3/%4.%5");
};

class JapanStdMapProvider : public CyberJapanMapProvider
{
public:
    JapanStdMapProvider()
        : CyberJapanMapProvider(QStringLiteral("Japan-GSI Contour"), QStringLiteral("std"), QStringLiteral("png"))
    {}
};

class JapanSeamlessMapProvider : public CyberJapanMapProvider
{
public:
    JapanSeamlessMapProvider()
        : CyberJapanMapProvider(QStringLiteral("Japan-GSI Seamless"), QStringLiteral("seamlessphoto"),
                                QStringLiteral("jpg"))
    {}
};

class JapanAnaglyphMapProvider : public CyberJapanMapProvider
{
public:
    JapanAnaglyphMapProvider()
        : CyberJapanMapProvider(QStringLiteral("Japan-GSI Anaglyph"), QStringLiteral("anaglyphmap_color"),
                                QStringLiteral("png"))
    {}
};

class JapanSlopeMapProvider : public CyberJapanMapProvider
{
public:
    JapanSlopeMapProvider()
        : CyberJapanMapProvider(QStringLiteral("Japan-GSI Slope"), QStringLiteral("slopemap"), QStringLiteral("png"))
    {}
};

class JapanReliefMapProvider : public CyberJapanMapProvider
{
public:
    JapanReliefMapProvider()
        : CyberJapanMapProvider(QStringLiteral("Japan-GSI Relief"), QStringLiteral("relief"), QStringLiteral("png"))
    {}
};

class LINZBasemapMapProvider : public MapProvider
{
public:
    LINZBasemapMapProvider()
        : MapProvider(QStringLiteral("LINZ Basemap"), QStringLiteral("https://basemaps.linz.govt.nz/v1/tiles/aerial"),
                      QStringLiteral("png"), QGC_AVERAGE_TILE_SIZE, MapProvider::SatelliteMapDay)
    {}

    QString attribution() const final { return QStringLiteral("© LINZ CC BY 4.0 © Imagery Basemap contributors"); }

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapUrl = QStringLiteral(
        "https://basemaps.linz.govt.nz/v1/tiles/aerial/EPSG:3857/%1/%2/%3.%4?api=d01ev80nqcjxddfvc6amyvkk1ka");
};

class OpenAIPMapProvider : public MapProvider
{
public:
    OpenAIPMapProvider()
        : MapProvider(QStringLiteral("OpenAIP"), QStringLiteral("https://www.openaip.net"), QStringLiteral("png"),
                      QGC_AVERAGE_TILE_SIZE, MapProvider::CustomMap)
    {}

    QString attribution() const final { return QStringLiteral("© openAIP (CC BY-NC 4.0)"); }

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapUrl = QStringLiteral("https://api.tiles.openaip.net/api/data/openaip/%1/%2/%3.png");
};

/// The tile usage policy of the OpenStreetMap servers (https://operations.osmfoundation.org/policies/tiles/) asks for
/// HTTPS, for the name of the application as the user agent and, of an application, for no Referer. The servers
/// answer other requests with a picture of a refusal, with the HTTP status of a tile.
class OpenStreetMapProvider : public MapProvider
{
public:
    OpenStreetMapProvider()
        : MapProvider(QStringLiteral("Street Map"), QString(), QStringLiteral("png"), QGC_AVERAGE_TILE_SIZE,
                      MapProvider::StreetMap)
    {}

    bool requiresApplicationUserAgent() const final { return true; }

    bool isRefusalImage(QByteArrayView image) const final;

    QString attribution() const final { return QStringLiteral("© OpenStreetMap contributors"); }

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapUrl = QStringLiteral("https://tile.openstreetmap.org/%1/%2/%3.png");
};

class StatkartMapProvider : public MapProvider
{
protected:
    StatkartMapProvider(const QString& mapName, const QString& mapTypeId)
        : MapProvider(mapName, QStringLiteral("https://norgeskart.no/"), QStringLiteral("png"), QGC_AVERAGE_TILE_SIZE,
                      MapProvider::StreetMap)
        , _mapTypeId(mapTypeId)
    {}

public:
    QString attribution() const final { return QStringLiteral("© Kartverket"); }

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapTypeId;
    const QString _mapUrl =
        QStringLiteral("https://cache.kartverket.no/v1/wmts/1.0.0/%1/default/webmercator/%2/%3/%4.png");
};

class StatkartTopoMapProvider : public StatkartMapProvider
{
public:
    StatkartTopoMapProvider()
        : StatkartMapProvider(QStringLiteral("Statkart Topo"), QStringLiteral("topo"))
    {}
};

class StatkartBaseMapProvider : public StatkartMapProvider
{
public:
    StatkartBaseMapProvider()
        : StatkartMapProvider(QStringLiteral("Statkart Basemap"), QStringLiteral("topo"))
    {}
};

class SvalbardMapProvider : public MapProvider
{
public:
    SvalbardMapProvider()
        : MapProvider(QStringLiteral("Svalbard Topo"), QStringLiteral("https://www.npolar.no/"), QStringLiteral("png"),
                      QGC_AVERAGE_TILE_SIZE, MapProvider::StreetMap)
    {}

    QString attribution() const final { return QStringLiteral("© Norwegian Polar Institute"); }

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapUrl = QStringLiteral(
        "https://geodata.npolar.no/arcgis/rest/services/Basisdata/NP_Basiskart_Svalbard_WMTS_3857/MapServer/WMTS/tile/"
        "1.0.0/Basisdata_NP_Basiskart_Svalbard_WMTS_3857/default/default028mm/%1/%2/%3");
};

class EniroMapProvider : public MapProvider
{
public:
    EniroMapProvider()
        : MapProvider(QStringLiteral("Eniro Topo"), QStringLiteral("https://www.eniro.se/"), QStringLiteral("png"),
                      QGC_AVERAGE_TILE_SIZE, MapProvider::StreetMap)
    {}

    QString attribution() const final { return QStringLiteral("© Eniro"); }

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapUrl = QStringLiteral("http://map.eniro.com/geowebcache/service/tms1.0.0/map/%1/%2/%3.%4");
};

class MapQuestMapProvider : public MapProvider
{
protected:
    MapQuestMapProvider(const QString& mapName, const QString& mapTypeId, MapProvider::MapStyle mapType)
        : MapProvider(mapName, QStringLiteral("https://mapquest.com"), QStringLiteral("jpg"), QGC_AVERAGE_TILE_SIZE,
                      mapType)
        , _mapTypeId(mapTypeId)
    {}

public:
    QString attribution() const final { return QStringLiteral("© MapQuest"); }

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapTypeId;
    const QString _mapUrl = QStringLiteral("http://otile%1.mqcdn.com/tiles/1.0.0/%2/%3/%4/%5.%6");
};

class MapQuestMapMapProvider : public MapQuestMapProvider
{
public:
    MapQuestMapMapProvider()
        : MapQuestMapProvider(QStringLiteral("MapQuest Map"), QStringLiteral("map"), MapProvider::StreetMap)
    {}
};

class MapQuestSatMapProvider : public MapQuestMapProvider
{
public:
    MapQuestSatMapProvider()
        : MapQuestMapProvider(QStringLiteral("MapQuest Sat"), QStringLiteral("sat"), MapProvider::SatelliteMapDay)
    {}
};

class VWorldMapProvider : public MapProvider
{
protected:
    VWorldMapProvider(const QString& mapName, const QString& mapTypeId, const QString& imageFormat, quint32 averageSize,
                      MapProvider::MapStyle mapStyle)
        : MapProvider(mapName, QStringLiteral("www.vworld.kr"), imageFormat, averageSize, mapStyle)
        , _mapTypeId(mapTypeId)
    {}

public:
    QString attribution() const final { return QStringLiteral("© VWorld"); }

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapTypeId;
    const QString _mapUrl = QStringLiteral("http://api.vworld.kr/req/wmts/1.0.0/%1/%2/%3/%4/%5.%6");
};

class VWorldStreetMapProvider : public VWorldMapProvider
{
public:
    VWorldStreetMapProvider()
        : VWorldMapProvider(QStringLiteral("VWorld Street Map"), QStringLiteral("Base"), QStringLiteral("png"),
                            QGC_AVERAGE_TILE_SIZE, MapProvider::StreetMap)
    {}
};

class VWorldSatMapProvider : public VWorldMapProvider
{
public:
    VWorldSatMapProvider()
        : VWorldMapProvider(QStringLiteral("VWorld Satellite Map"), QStringLiteral("Satellite"), QStringLiteral("jpeg"),
                            QGC_AVERAGE_TILE_SIZE, MapProvider::SatelliteMapDay)
    {}
};
