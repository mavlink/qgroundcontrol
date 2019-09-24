#include "GenericMapProvider.h"
#include "QGCMapEngine.h"

QString StatkartMapProvider::_getURL(int x, int y, int zoom,
                                     QNetworkAccessManager* networkManager) {
    Q_UNUSED(networkManager);
    return QString("http://opencache.statkart.no/gatekeeper/gk/"
                   "gk.open_gmaps?layers=topo4&zoom=%1&x=%2&y=%3")
        .arg(zoom)
        .arg(x)
        .arg(y);
}

QString EniroMapProvider::_getURL(int x, int y, int zoom,
                                  QNetworkAccessManager* networkManager) {
    Q_UNUSED(networkManager);
    return QString("http://map.eniro.com/geowebcache/service/tms1.0.0/map/%1/"
                   "%2/%3.png")
        .arg(zoom)
        .arg(x)
        .arg((1 << zoom) - 1 - y);
}

QString MapQuestMapMapProvider::_getURL(int x, int y, int zoom,
                                        QNetworkAccessManager* networkManager) {
    Q_UNUSED(networkManager);
    char letter = "1234"[_getServerNum(x, y, 4)];
    return QString("http://otile%1.mqcdn.com/tiles/1.0.0/map/%2/%3/%4.jpg")
        .arg(letter)
        .arg(zoom)
        .arg(x)
        .arg(y);
}

QString MapQuestSatMapProvider::_getURL(int x, int y, int zoom,
                                        QNetworkAccessManager* networkManager) {
    Q_UNUSED(networkManager);
    char letter = "1234"[_getServerNum(x, y, 4)];
    return QString("http://otile%1.mqcdn.com/tiles/1.0.0/sat/%2/%3/%4.jpg")
        .arg(letter)
        .arg(zoom)
        .arg(x)
        .arg(y);
}

QString
VWorldStreetMapProvider::_getURL(int x, int y, int zoom,
                                 QNetworkAccessManager* networkManager) {
    Q_UNUSED(networkManager);
    int gap   = zoom - 6;
    int x_min = 53 * pow(2, gap);
    int x_max = 55 * pow(2, gap) + (2 * gap - 1);
    int y_min = 22 * pow(2, gap);
    int y_max = 26 * pow(2, gap) + (2 * gap - 1);

    if (zoom > 19) {
        return {};
    } else if (zoom > 5 && x >= x_min && x <= x_max && y >= y_min &&
               y <= y_max) {
        return QString(
                   "http://xdworld.vworld.kr:8080/2d/Base/service/%1/%2/%3.png")
            .arg(zoom)
            .arg(x)
            .arg(y);
    } else {
        QString key = _tileXYToQuadKey(x, y, zoom);
        return QString("http://ecn.t%1.tiles.virtualearth.net/tiles/"
                       "r%2.png?g=%3&mkt=%4")
            .arg(_getServerNum(x, y, 4))
            .arg(key)
            .arg(_versionBingMaps)
            .arg(_language);
    }
}

QString VWorldSatMapProvider::_getURL(int x, int y, int zoom,
                                      QNetworkAccessManager* networkManager) {
    Q_UNUSED(networkManager);
    int gap   = zoom - 6;
    int x_min = 53 * pow(2, gap);
    int x_max = 55 * pow(2, gap) + (2 * gap - 1);
    int y_min = 22 * pow(2, gap);
    int y_max = 26 * pow(2, gap) + (2 * gap - 1);

    if (zoom > 19) {
        return {};
    } else if (zoom > 5 && x >= x_min && x <= x_max && y >= y_min &&
               y <= y_max) {
        return QString("http://xdworld.vworld.kr:8080/2d/Satellite/service/%1/"
                       "%2/%3.jpeg")
            .arg(zoom)
            .arg(x)
            .arg(y);
    } else {
        QString key = _tileXYToQuadKey(x, y, zoom);
        return QString("http://ecn.t%1.tiles.virtualearth.net/tiles/"
                       "a%2.jpeg?g=%3&mkt=%4")
            .arg(_getServerNum(x, y, 4))
            .arg(key)
            .arg(_versionBingMaps)
            .arg(_language);
    }
}

QString TestMapProvider::_getURL(int x, int y, int zoom,
                                        QNetworkAccessManager* networkManager) {
    Q_UNUSED(networkManager);
    qDebug() << "zoom" << zoom << " x " << x << " y " << y;
    return QString("https://tiles.openaerialmap.org/5913eab91acd6100118dd513/0/75ab99f7-5b37-4f59-8392-7276498dc2fc/%1/%2/%3.png")
        .arg(zoom)
        .arg(x)
        .arg(y);
}

