#include "ElevationMapProvider.h"
#if defined(DEBUG_GOOGLE_MAPS)
#include <QFile>
#include <QStandardPaths>
#endif
#include "QGCMapEngine.h"

ElevationProvider::ElevationProvider(QString imageFormat, quint32 averageSize,
                                     QGeoMapType::MapStyle mapType,
                                     QObject*              parent)
    : MapProvider(QString("https://api.airmap.com/"), imageFormat, averageSize,
                  mapType, parent) {}
ElevationProvider::~ElevationProvider() {}

//-----------------------------------------------------------------------------
int AirmapElevationProvider::long2tileX(double lon, int z) {
    Q_UNUSED(z);
    return static_cast<int>(floor((lon + 180.0) / srtm1TileSize));
}

//-----------------------------------------------------------------------------
int AirmapElevationProvider::lat2tileY(double lat, int z) {
    Q_UNUSED(z);
    return static_cast<int>(floor((lat + 90.0) / srtm1TileSize));
}

QString
AirmapElevationProvider::_getURL(int x, int y, int zoom,
                                 QNetworkAccessManager* networkManager) {
    Q_UNUSED(networkManager);
    Q_UNUSED(zoom);
    return QString("https://api.airmap.com/elevation/v1/ele/"
                   "carpet?points=%1,%2,%3,%4")
        .arg(static_cast<double>(y) * srtm1TileSize - 90.0)
        .arg(static_cast<double>(x) * srtm1TileSize - 180.0)
        .arg(static_cast<double>(y + 1) * srtm1TileSize - 90.0)
        .arg(static_cast<double>(x + 1) * srtm1TileSize - 180.0);
}

QGCTileSet AirmapElevationProvider::getTileCount(int zoom, double topleftLon,
                                                 double topleftLat,
                                                 double bottomRightLon,
                                                 double bottomRightLat) {
    QGCTileSet set;
    set.tileX0 = long2tileX(topleftLon, zoom);
    set.tileY0 = lat2tileY(bottomRightLat, zoom);
    set.tileX1 = long2tileX(bottomRightLon, zoom);
    set.tileY1 = lat2tileY(topleftLat, zoom);

    set.tileCount = (static_cast<quint64>(set.tileX1) -
                     static_cast<quint64>(set.tileX0) + 1) *
                    (static_cast<quint64>(set.tileY1) -
                     static_cast<quint64>(set.tileY0) + 1);

    set.tileSize = getAverageSize() * set.tileCount;

    return set;
}
