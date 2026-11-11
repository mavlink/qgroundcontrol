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
    return TerrainTileCopernicus::serializeFromData(image);
}

QGCTileSet TerrariumElevationProvider::getTileCount(int zoom, double topleftLon, double topleftLat,
                                                    double bottomRightLon, double bottomRightLat) const
{
    QGCTileSet set = MapProvider::getTileCount(zoom, topleftLon, topleftLat, bottomRightLon, bottomRightLat);
    if (zoom != kAnchorZoom) {
        return set;
    }

    // A tile of the next zoom is half an anchor wide: its neighbor outside the region can lie in the next anchor
    constexpr int kLastFinerTile = (1 << (kAnchorZoom + 1)) - 1;
    const QGCTileSet finer =
        MapProvider::getTileCount(kAnchorZoom + 1, topleftLon, topleftLat, bottomRightLon, bottomRightLat);
    set.tileX0 = qMax(finer.tileX0 - 1, 0) / 2;
    set.tileY0 = qMax(finer.tileY0 - 1, 0) / 2;
    set.tileX1 = qMin(finer.tileX1 + 1, kLastFinerTile) / 2;
    set.tileY1 = qMin(finer.tileY1 + 1, kLastFinerTile) / 2;
    set.tileCount = (static_cast<quint64>(set.tileX1) - static_cast<quint64>(set.tileX0) + 1) *
                    (static_cast<quint64>(set.tileY1) - static_cast<quint64>(set.tileY0) + 1);
    set.tileSize = getAverageSize() * set.tileCount;
    return set;
}

QString TerrariumElevationProvider::_getURL(int x, int y, int zoom) const
{
    return QStringLiteral("%1/terrarium/%2/%3/%4.png").arg(kProviderURL).arg(zoom).arg(x).arg(y);
}
