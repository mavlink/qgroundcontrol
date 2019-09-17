#include "BingMapProvider.h"
#if defined(DEBUG_GOOGLE_MAPS)
#include <QFile>
#include <QStandardPaths>
#endif
#include "QGCMapEngine.h"

BingMapProvider::BingMapProvider(QString imageFormat, quint32 averageSize,
                                 QGeoMapType::MapStyle mapType, QObject* parent)
    : MapProvider(QString("https://www.bing.com/maps/"), imageFormat,
                  averageSize, mapType, parent) {}

BingMapProvider::~BingMapProvider() {}

QString BingRoadMapProvider::_getURL(int x, int y, int zoom,
                                     QNetworkAccessManager* networkManager) {
    Q_UNUSED(networkManager);
    QString key = _tileXYToQuadKey(x, y, zoom);
    return QString("http://ecn.t%1.tiles.virtualearth.net/tiles/"
                   "r%2.png?g=%3&mkt=%4")
        .arg(_getServerNum(x, y, 4))
        .arg(key)
        .arg(_versionBingMaps)
        .arg(_language);
}

QString
BingSatelliteMapProvider::_getURL(int x, int y, int zoom,
                                  QNetworkAccessManager* networkManager) {
    Q_UNUSED(networkManager);
    QString key = _tileXYToQuadKey(x, y, zoom);
    return QString("http://ecn.t%1.tiles.virtualearth.net/tiles/"
                   "a%2.jpeg?g=%3&mkt=%4")
        .arg(_getServerNum(x, y, 4))
        .arg(key)
        .arg(_versionBingMaps)
        .arg(_language);
}

QString BingHybridMapProvider::_getURL(int x, int y, int zoom,
                                       QNetworkAccessManager* networkManager) {
    Q_UNUSED(networkManager);
    QString key = _tileXYToQuadKey(x, y, zoom);
    return QString("http://ecn.t%1.tiles.virtualearth.net/tiles/"
                   "h%2.jpeg?g=%3&mkt=%4")
        .arg(_getServerNum(x, y, 4))
        .arg(key)
        .arg(_versionBingMaps)
        .arg(_language);
}

