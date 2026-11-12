#pragma once

#include "MapProvider.h"

static constexpr const quint32 AVERAGE_BING_STREET_MAP = 1297;
static constexpr const quint32 AVERAGE_BING_SAT_MAP = 19597;

class BingMapProvider : public MapProvider
{
protected:
    BingMapProvider(const QString& mapName, const QString& mapTypeCode, const QString& imageFormat, quint32 averageSize,
                    MapProvider::MapStyle mapType)
        : MapProvider(mapName, QStringLiteral("https://www.bing.com/maps/"), imageFormat, averageSize, mapType)
        , _mapTypeId(mapTypeCode)
    {}

public:
    /// Bing answers with this picture, not with an HTTP error, where it has no imagery
    bool isNoTileImage(QByteArrayView image) const final;

    QString attribution() const final { return QStringLiteral("© Microsoft"); }

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapTypeId;
    const QString _mapUrl = QStringLiteral("https://ecn.t%1.tiles.virtualearth.net/tiles/%2%3.%4?g=%5&mkt=%6");
    const QString _versionBingMaps = QStringLiteral("2981");
};

class BingRoadMapProvider : public BingMapProvider
{
public:
    BingRoadMapProvider()
        : BingMapProvider(QStringLiteral("Bing Road"), QStringLiteral("r"), QStringLiteral("png"),
                          AVERAGE_BING_STREET_MAP, MapProvider::StreetMap)
    {}
};

class BingSatelliteMapProvider : public BingMapProvider
{
public:
    BingSatelliteMapProvider()
        : BingMapProvider(QStringLiteral("Bing Satellite"), QStringLiteral("a"), QStringLiteral("jpg"),
                          AVERAGE_BING_SAT_MAP, MapProvider::SatelliteMapDay)
    {}
};

class BingHybridMapProvider : public BingMapProvider
{
public:
    BingHybridMapProvider()
        : BingMapProvider(QStringLiteral("Bing Hybrid"), QStringLiteral("h"), QStringLiteral("jpg"),
                          AVERAGE_BING_SAT_MAP, MapProvider::HybridMap)
    {}
};
