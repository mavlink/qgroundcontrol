#include "ElevationMapProvider.h"
#include "QGCTileSet.h"
#include "TerrainTileCopernicus.h"

#include <QtCore/QDir>
#include <QtCore/QTemporaryFile>

int CopernicusElevationProvider::long2tileX(double lon, int z) const
{
    Q_UNUSED(z)
    return static_cast<int>(floor((lon + 180.0) / TerrainTileCopernicus::kTileSizeDegrees));
}

int CopernicusElevationProvider::lat2tileY(double lat, int z) const
{
    Q_UNUSED(z)
    return static_cast<int>(floor((lat + 90.0) / TerrainTileCopernicus::kTileSizeDegrees));
}

bool CopernicusElevationProvider::isValidTileCoordinate(int x, int y, int zoom) const
{
    // Copernicus tiles a global 0.01° grid, not a 2^zoom web-mercator pyramid, so the
    // base bound rejects every real tile. Bound by the provider's own grid extents.
    const int maxX = long2tileX(180.0, zoom);
    const int maxY = lat2tileY(90.0, zoom);
    return (x >= 0) && (x <= maxX) && (y >= 0) && (y <= maxY);
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

    // Guard against an inverted bbox: an unsigned subtraction with tileX1 < tileX0
    // (or tileY1 < tileY0) would underflow to ~2^64 and report a bogus tile count.
    if ((set.tileX1 < set.tileX0) || (set.tileY1 < set.tileY0)) {
        set.tileCount = 0;
        set.tileSize = 0;
        return set;
    }

    set.tileCount = (static_cast<quint64>(set.tileX1) -
                     static_cast<quint64>(set.tileX0) + 1) *
                    (static_cast<quint64>(set.tileY1) -
                     static_cast<quint64>(set.tileY0) + 1);

    set.tileSize = getAverageSize() * set.tileCount;

    return set;
}

QByteArray CopernicusElevationProvider::serialize(const QByteArray &image) const
{
    return TerrainTileCopernicus::serializeFromData(image);
}
