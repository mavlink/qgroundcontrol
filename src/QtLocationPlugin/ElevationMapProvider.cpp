#include "ElevationMapProvider.h"
#if defined(DEBUG_GOOGLE_MAPS)
#include <QFile>
#include <QStandardPaths>
#endif
#include "QGCMapEngine.h"
#include "TerrainTile.h"

ElevationProvider::ElevationProvider(const QString& imageFormat, quint32 averageSize, QGeoMapType::MapStyle mapType, QObject* parent)
    : MapProvider(QStringLiteral("https://api.airmap.com/"), imageFormat, averageSize, mapType, parent) {}

//-----------------------------------------------------------------------------
int AirmapElevationProvider::long2tileX(const double lon, const int z) const {
    Q_UNUSED(z)
    return static_cast<int>(floor((lon + 180.0) / TerrainTile::tileSizeDegrees));
}

//-----------------------------------------------------------------------------
int AirmapElevationProvider::lat2tileY(const double lat, const int z) const {
    Q_UNUSED(z)
    return static_cast<int>(floor((lat + 90.0) / TerrainTile::tileSizeDegrees));
}

QString AirmapElevationProvider::_getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) {
    Q_UNUSED(networkManager)
    Q_UNUSED(zoom)
    return QString("https://api.airmap.com/elevation/v1/ele/carpet?points=%1,%2,%3,%4")
        .arg(static_cast<double>(y) * TerrainTile::tileSizeDegrees - 90.0)
        .arg(static_cast<double>(x) * TerrainTile::tileSizeDegrees - 180.0)
        .arg(static_cast<double>(y + 1) * TerrainTile::tileSizeDegrees - 90.0)
        .arg(static_cast<double>(x + 1) * TerrainTile::tileSizeDegrees - 180.0);
}

QGCTileSet AirmapElevationProvider::getTileCount(const int zoom, const double topleftLon,
                                                 const double topleftLat, const double bottomRightLon,
                                                 const double bottomRightLat) const {
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
