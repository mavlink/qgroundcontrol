#pragma once

#include "MapProvider.h"

static constexpr const quint32 AVERAGE_BING_STREET_MAP = 1297;
static constexpr const quint32 AVERAGE_BING_SAT_MAP = 19597;

class BingMapProvider : public MapProvider
{
public:
    BingMapProvider(const QString& mapName, const QString& mapTypeCode, const QString& imageFormat, quint32 averageSize,
                    MapProvider::MapStyle mapType)
        : MapProvider(mapName, QStringLiteral("https://www.bing.com/maps/"), imageFormat, averageSize, mapType),
          _mapTypeId(mapTypeCode)
    {}

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapTypeId;
    const QString _mapUrl = QStringLiteral("https://ecn.t%1.tiles.virtualearth.net/tiles/%2%3.%4?g=%5&mkt=%6");
    const QString _versionBingMaps = QStringLiteral("2981");
    static constexpr int kServerCount = 4;

    /*QUrl m_url;
    const QString m_scheme = QStringLiteral("http");
    const QString m_host = QStringLiteral("ecn.t%1.tiles.virtualearth.net");
    const QString m_path = QStringLiteral("tiles/%1%2.%3");
    const QUrlQuery m_query = QStringLiteral("g=%1&mkt=%2");*/
};
