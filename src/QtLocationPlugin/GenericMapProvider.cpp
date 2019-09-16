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
