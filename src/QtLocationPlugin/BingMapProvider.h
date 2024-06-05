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

static constexpr const quint32 AVERAGE_BING_STREET_MAP = 1297;
static constexpr const quint32 AVERAGE_BING_SAT_MAP    = 19597;

class BingMapProvider : public MapProvider
{
    Q_OBJECT

protected:
    BingMapProvider(const QString &mapName, const QString &imageFormat, quint32 averageSize,
                    QGeoMapType::MapStyle mapType, QObject* parent = nullptr)
        : MapProvider(QStringLiteral("https://www.bing.com/maps/"), imageFormat, averageSize, mapType, parent)
        , _mapName(mapName) {}

public:
    bool isBingProvider() const final { return true; }

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapName;
    const QString _mapUrl = QStringLiteral("http://ecn.t%1.tiles.virtualearth.net/tiles/%2%3.%4?g=%5&mkt=%6");
    const QString _versionBingMaps = QStringLiteral("2981");
};

class BingRoadMapProvider : public BingMapProvider
{
    Q_OBJECT

public:
    BingRoadMapProvider(QObject* parent = nullptr)
        : BingMapProvider(QStringLiteral("r"), QStringLiteral("png"), AVERAGE_BING_STREET_MAP, QGeoMapType::StreetMap, parent) {}
};

class BingSatelliteMapProvider : public BingMapProvider
{
    Q_OBJECT

public:
    BingSatelliteMapProvider(QObject* parent = nullptr)
        : BingMapProvider(QStringLiteral("a"), QStringLiteral("jpg"), AVERAGE_BING_SAT_MAP, QGeoMapType::SatelliteMapDay, parent) {}
};

class BingHybridMapProvider : public BingMapProvider
{
    Q_OBJECT

public:
    BingHybridMapProvider(QObject* parent = nullptr)
        : BingMapProvider(QStringLiteral("h"), QStringLiteral("jpg"),AVERAGE_BING_SAT_MAP, QGeoMapType::HybridMap, parent) {}
};
