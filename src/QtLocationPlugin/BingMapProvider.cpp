/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "BingMapProvider.h"

BingMapProvider::BingMapProvider(const QString &imageFormat, const quint32 averageSize,
                                 const QGeoMapType::MapStyle mapType, QObject* parent)
    : MapProvider(QStringLiteral("https://www.bing.com/maps/"), imageFormat, averageSize, mapType, parent) {}

static const QString RoadMapUrl = QStringLiteral("http://ecn.t%1.tiles.virtualearth.net/tiles/r%2.png?g=%3&mkt=%4");

QString BingRoadMapProvider::_getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) {
    Q_UNUSED(networkManager)
    const QString key = _tileXYToQuadKey(x, y, zoom);
    return RoadMapUrl.arg(QString::number(_getServerNum(x, y, 4)), key, _versionBingMaps, _language);
}

static const QString SatteliteMapUrl = QStringLiteral("http://ecn.t%1.tiles.virtualearth.net/tiles/a%2.jpeg?g=%3&mkt=%4");

QString BingSatelliteMapProvider::_getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) {
    Q_UNUSED(networkManager)
    const QString key = _tileXYToQuadKey(x, y, zoom);
    return SatteliteMapUrl.arg(QString::number(_getServerNum(x, y, 4)) ,key ,_versionBingMaps ,_language);
}

static const QString HybridMapUrl = QStringLiteral("http://ecn.t%1.tiles.virtualearth.net/tiles/h%2.jpeg?g=%3&mkt=%4");

QString BingHybridMapProvider::_getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) {
    Q_UNUSED(networkManager)
    const QString key = _tileXYToQuadKey(x, y, zoom);
    return HybridMapUrl.arg(QString::number(_getServerNum(x, y, 4)), key, _versionBingMaps, _language);
}

