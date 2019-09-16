#include "StatkartMapProvider.h"
#if defined(DEBUG_GOOGLE_MAPS)
#include <QFile>
#include <QStandardPaths>
#endif
#include "QGCMapEngine.h"

StatkartMapProvider::StatkartMapProvider(QObject* parent)
    : MapProvider(QString("https://www.norgeskart.no/"), QString("png"),
                  AVERAGE_TILE_SIZE, QGeoMapType::StreetMap, parent) {}

StatkartMapProvider::~StatkartMapProvider() {}

QString StatkartMapProvider::_getURL(int x, int y, int zoom,
                                     QNetworkAccessManager* networkManager) {
    Q_UNUSED(networkManager);
    return QString("http://opencache.statkart.no/gatekeeper/gk/"
                   "gk.open_gmaps?layers=topo4&zoom=%1&x=%2&y=%3")
        .arg(zoom)
        .arg(x)
        .arg(y);
}

