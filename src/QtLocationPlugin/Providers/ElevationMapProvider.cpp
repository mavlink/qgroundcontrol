#include "ElevationMapProvider.h"
#include "TerrainTileCopernicus.h"

#include <QtCore/QDir>
#include <QtCore/QTemporaryFile>

int CopernicusElevationProvider::long2tileX(double lon, int z) const
{
    Q_UNUSED(z)
    (void) isValidLongitude(lon);
    const int tileX = static_cast<int>(floor((lon + 180.0) / TerrainTileCopernicus::kTileSizeDegrees));
    return tileX;
}

int CopernicusElevationProvider::lat2tileY(double lat, int z) const
{
    Q_UNUSED(z)
    (void) isValidLatitude(lat);
    const int tileY = static_cast<int>(floor((lat + 90.0) / TerrainTileCopernicus::kTileSizeDegrees));
    return tileY;
}

QString CopernicusElevationProvider::_getURL(int x, int y, int zoom) const
{
    Q_UNUSED(zoom)
    const double lat1 = (static_cast<double>(y) * TerrainTileCopernicus::kTileSizeDegrees) - 90.0;
    const double lon1 = (static_cast<double>(x) * TerrainTileCopernicus::kTileSizeDegrees) - 180.0;
    const double lat2 = (static_cast<double>(y + 1) * TerrainTileCopernicus::kTileSizeDegrees) - 90.0;
    const double lon2 = (static_cast<double>(x + 1) * TerrainTileCopernicus::kTileSizeDegrees) - 180.0;
    const QString url = _mapUrl.arg(lat1).arg(lon1).arg(lat2).arg(lon2);
    return url;
}

QGCTileSet CopernicusElevationProvider::getTileCount(int zoom, double topleftLon,
                                                     double topleftLat, double bottomRightLon,
                                                     double bottomRightLat) const
{
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

QByteArray CopernicusElevationProvider::serialize(const QByteArray &image) const
{
    const QByteArray serialized = TerrainTileCopernicus::serializeFromData(image);
    return serialized;
}
