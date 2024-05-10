/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

// #include <QtCore/QUrl>
// #include <QtCore/QUrlQuery>

#include "MapProvider.h"

class BingMapProvider : public MapProvider
{
protected:
    BingMapProvider(const QString &mapName, const QString &imageFormat, quint32 averageSize,
                    QGeoMapType::MapStyle mapStyle, const QString &mapUrl)
    : MapProvider(mapName, QStringLiteral("https://www.bing.com/maps/"), imageFormat, averageSize, mapStyle)
    , m_mapUrl(mapUrl)
    {
        m_cameraCapabilities.setMaximumZoomLevel(20.0);
    }

    bool isBingProvider() const final { return true; }

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString m_mapUrl;
    const QString m_versionBingMaps = QStringLiteral("2981");

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
            "Bing Road",
            QStringLiteral("png"),
            1297,
            QGeoMapType::StreetMap,
            QStringLiteral("http://ecn.t%1.tiles.virtualearth.net/tiles/r%2.png?g=%3&mkt=%4")) {}
};

class BingSatelliteMapProvider : public BingMapProvider
{
public:
    BingSatelliteMapProvider()
        : BingMapProvider(
            "Bing Satellite",
            QStringLiteral("jpg"),
            19597,
            QGeoMapType::SatelliteMapDay,
            QStringLiteral("http://ecn.t%1.tiles.virtualearth.net/tiles/a%2.jpeg?g=%3&mkt=%4")) {}
};

class BingHybridMapProvider : public BingMapProvider
{
public:
    BingHybridMapProvider()
        : BingMapProvider(
            "Bing Hybrid",
            QStringLiteral("jpg"),
            19597,
            QGeoMapType::HybridMap,
            QStringLiteral("http://ecn.t%1.tiles.virtualearth.net/tiles/h%2.jpeg?g=%3&mkt=%4")) {}
};
