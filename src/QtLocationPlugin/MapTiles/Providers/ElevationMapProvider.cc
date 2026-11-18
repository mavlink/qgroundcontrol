#include "ElevationMapProvider.h"

#include <QtCore/QDir>
#include <QtCore/QTemporaryFile>

#include "QGCTileSet.h"
#include "TerrainTileCopernicus.h"

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

QByteArray CopernicusElevationProvider::serialize(const QByteArray& image) const
{
    return TerrainTileCopernicus::serializeFromData(image);
}

QGCTileSet TerrariumElevationProvider::getTileCount(int zoom, double topleftLon, double topleftLat,
                                                    double bottomRightLon, double bottomRightLat) const
{
    if (zoom != kAnchorZoom) {
        return MapProvider::getTileCount(zoom, topleftLon, topleftLat, bottomRightLon, bottomRightLat);
    }

    // A tile of the next zoom is half an anchor wide: its neighbor outside the region can lie in the next anchor.
    // A tile at an edge of the world has no neighbor beyond it.
    constexpr int kLastFinerTile = (1 << (kAnchorZoom + 1)) - 1;
    const QGCTileSet finer =
        MapProvider::getTileCount(kAnchorZoom + 1, topleftLon, topleftLat, bottomRightLon, bottomRightLat);
    // The columns of a region that crosses the antimeridian run on past the last one
    const bool crossesAntimeridian = finer.tileX1 > kLastFinerTile;
    const int rightX = crossesAntimeridian ? (finer.tileX1 + 1) : qMin(finer.tileX1 + 1, kLastFinerTile);
    return _tileSet(qMax(finer.tileX0 - 1, 0) / 2, qMax(finer.tileY0 - 1, 0) / 2, rightX / 2,
                    qMin(finer.tileY1 + 1, kLastFinerTile) / 2);
}

QString TerrariumElevationProvider::_getURL(int x, int y, int zoom) const
{
    return QStringLiteral("%1/terrarium/%2/%3/%4.png").arg(kProviderURL).arg(zoom).arg(x).arg(y);
}
