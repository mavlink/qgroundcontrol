#pragma once

#include "MapProvider.h"

class CustomURLMapProvider : public MapProvider
{
public:
    CustomURLMapProvider()
        : MapProvider(QStringLiteral("CustomURL Custom"), QStringLiteral(""), QStringLiteral(""), QGC_AVERAGE_TILE_SIZE,
                      MapProvider::CustomMap)
    {}

private:
    QString _getURL(int x, int y, int zoom) const final;
};

// Data-driven provider for the common XYZ/TMS tile schemes; _getURL applies the
// config (server-rotation arg, layer id, axis order, TMS y-flip, image-format token).
struct MapProviderConfig
{
    enum AxisOrder
    {
        ZXY,
        ZYX
    };

    QString name;
    QString referrer;
    QString urlTemplate;
    QString imageFormat;
    quint32 averageSize = QGC_AVERAGE_TILE_SIZE;
    MapProvider::MapStyle mapStyle = MapProvider::CustomMap;
    QString mapTypeId;
    AxisOrder axisOrder = ZXY;
    bool flipY = false;
    bool appendImageFormat = false;
    int serverCount = 0;
    bool isOSM = false;
};

class TemplateMapProvider : public MapProvider
{
public:
    explicit TemplateMapProvider(const MapProviderConfig &config)
        : MapProvider(config.name, config.referrer, config.imageFormat, config.averageSize, config.mapStyle),
          _config(config)
    {}

    bool isOSMProvider() const final { return _config.isOSM; }

private:
    QString _getURL(int x, int y, int zoom) const final;

    const MapProviderConfig _config;
};

class LINZBasemapMapProvider : public MapProvider
{
public:
    LINZBasemapMapProvider()
        : MapProvider(QStringLiteral("LINZ Basemap"), QStringLiteral("https://basemaps.linz.govt.nz/v1/tiles/aerial"),
                      QStringLiteral("png"), QGC_AVERAGE_TILE_SIZE, MapProvider::SatelliteMapDay)
    {}

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapUrl = QStringLiteral("https://basemaps.linz.govt.nz/v1/tiles/aerial/EPSG:3857/%1/%2/%3.%4");
};

class OpenAIPMapProvider : public MapProvider
{
public:
    OpenAIPMapProvider()
        : MapProvider(QStringLiteral("OpenAIP"), QStringLiteral("https://www.openaip.net"), QStringLiteral("png"),
                      QGC_AVERAGE_TILE_SIZE, MapProvider::CustomMap)
    {}

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapUrl = QStringLiteral("https://api.tiles.openaip.net/api/data/openaip/%1/%2/%3.png");
};

class VWorldMapProvider : public MapProvider
{
protected:
    VWorldMapProvider(const QString& mapName, const QString& mapTypeId, const QString& imageFormat, quint32 averageSize,
                      MapProvider::MapStyle mapStyle)
        : MapProvider(mapName, QStringLiteral("www.vworld.kr"), imageFormat, averageSize, mapStyle),
          _mapTypeId(mapTypeId)
    {}

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapTypeId;
    const QString _mapUrl = QStringLiteral("https://api.vworld.kr/req/wmts/1.0.0/%1/%2/%3/%4/%5.%6");
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
