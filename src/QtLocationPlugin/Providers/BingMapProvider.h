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

static constexpr const quint32 AVERAGE_BING_STREET_MAP = 1297;
static constexpr const quint32 AVERAGE_BING_SAT_MAP    = 19597;

class BingMapProvider : public MapProvider
{
protected:
    BingMapProvider(const QString &mapName, const QString &mapTypeCode, const QString &imageFormat, quint32 averageSize,
                    QGeoMapType::MapStyle mapType)
        : MapProvider(mapName, QStringLiteral("https://www.bing.com/maps/"), imageFormat, averageSize, mapType)
        , _mapTypeId(mapTypeCode) {}

public:
    bool isBingProvider() const final { return true; }

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapTypeId;
    const QString _mapUrl = QStringLiteral("http://ecn.t%1.tiles.virtualearth.net/tiles/%2%3.%4?g=%5&mkt=%6");
    const QString _versionBingMaps = QStringLiteral("2981");

    /*QUrl m_url;
    const QString m_scheme = QStringLiteral("http");
    const QString m_host = QStringLiteral("ecn.t%1.tiles.virtualearth.net");
    const QString m_path = QStringLiteral("tiles/%1%2.%3");
    const QUrlQuery m_query = QStringLiteral("g=%1&mkt=%2");*/
};

class BingRoadMapProvider : public BingMapProvider
{
public:
    BingRoadMapProvider()
        : BingMapProvider(
            QStringLiteral("Bing Road"),
            QStringLiteral("r"),
            QStringLiteral("png"),
            AVERAGE_BING_STREET_MAP,
            QGeoMapType::StreetMap) {}
};

class BingSatelliteMapProvider : public BingMapProvider
{
public:
    BingSatelliteMapProvider()
        : BingMapProvider(
            QStringLiteral("Bing Satellite"),
            QStringLiteral("a"),
            QStringLiteral("jpg"),
            AVERAGE_BING_SAT_MAP,
            QGeoMapType::SatelliteMapDay) {}
};

class BingHybridMapProvider : public BingMapProvider
{
public:
    BingHybridMapProvider()
        : BingMapProvider(
            QStringLiteral("Bing Hybrid"),
            QStringLiteral("h"),
            QStringLiteral("jpg"),
            AVERAGE_BING_SAT_MAP,
            QGeoMapType::HybridMap) {}
};
