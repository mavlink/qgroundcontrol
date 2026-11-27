#include "ElevationMapProvider.h"

#include <algorithm>
#include <cmath>

#include "CopernicusTileFormat.h"
#include "TileRegion.h"

int CopernicusElevationProvider::long2tileX(double lon, int z) const
{
    Q_UNUSED(z)
    // Bounded to the grid before the cast, which is undefined for what int cannot hold
    const double eastEdge = 360.0 / CopernicusTileFormat::kTileSizeDegrees;
    return static_cast<int>(
        std::fmin(std::fmax(std::floor((lon + 180.0) / CopernicusTileFormat::kTileSizeDegrees), 0.0), eastEdge));
}

int CopernicusElevationProvider::lat2tileY(double lat, int z) const
{
    Q_UNUSED(z)
    const double northEdge = 180.0 / CopernicusTileFormat::kTileSizeDegrees;
    return static_cast<int>(
        std::fmin(std::fmax(std::floor((lat + 90.0) / CopernicusTileFormat::kTileSizeDegrees), 0.0), northEdge));
}

QString CopernicusElevationProvider::_getURL(int x, int y, int zoom) const
{
    Q_UNUSED(zoom)
    const double lat1 = (static_cast<double>(y) * CopernicusTileFormat::kTileSizeDegrees) - 90.0;
    const double lon1 = (static_cast<double>(x) * CopernicusTileFormat::kTileSizeDegrees) - 180.0;
    const double lat2 = (static_cast<double>(y + 1) * CopernicusTileFormat::kTileSizeDegrees) - 90.0;
    const double lon2 = (static_cast<double>(x + 1) * CopernicusTileFormat::kTileSizeDegrees) - 180.0;
    const QString url = _mapUrl.arg(lat1).arg(lon1).arg(lat2).arg(lon2);
    return url;
}

QByteArray CopernicusElevationProvider::serialize(const QByteArray& image) const
{
    return CopernicusTileFormat::serialize(image);
}

TileRegion TerrariumElevationProvider::getTileCount(int zoom, double topleftLon, double topleftLat,
                                                    double bottomRightLon, double bottomRightLat) const
{
    if (zoom != kAnchorZoom) {
        return MapProvider::getTileCount(zoom, topleftLon, topleftLat, bottomRightLon, bottomRightLat);
    }

    // A tile of the next zoom is half an anchor wide: its neighbor outside the region can lie in the next anchor.
    // A tile at an edge of the world has no neighbor beyond it.
    constexpr int kLastFinerTile = (1 << (kAnchorZoom + 1)) - 1;
    const TileRegion finer =
        MapProvider::getTileCount(kAnchorZoom + 1, topleftLon, topleftLat, bottomRightLon, bottomRightLat);
    // A region without tiles, as one with a corner that is not a number, needs no anchors
    if (finer.tileCount == 0) {
        return finer;
    }
    // The columns of a region that crosses the antimeridian run on past the last one
    const bool crossesAntimeridian = finer.tileX1 > kLastFinerTile;
    const int rightX = crossesAntimeridian ? (finer.tileX1 + 1) : std::min(finer.tileX1 + 1, kLastFinerTile);
    return _tileRegion(std::max(finer.tileX0 - 1, 0) / 2, std::max(finer.tileY0 - 1, 0) / 2, rightX / 2,
                       std::min(finer.tileY1 + 1, kLastFinerTile) / 2);
}

QString TerrariumElevationProvider::_getURL(int x, int y, int zoom) const
{
    return QStringLiteral("%1/terrarium/%2/%3/%4.png").arg(kProviderURL).arg(zoom).arg(x).arg(y);
}
