#include "TileMath.h"

#include <QtCore/QDebug>
#include <QtCore/QtMath>

#include <algorithm>
#include <cmath>

namespace TileMath {

QDebug operator<<(QDebug debug, const TileKey& key)
{
    const QDebugStateSaver saver(debug);
    debug.nospace() << key.zoom << '/' << key.x << '/' << key.y;
    return debug;
}

double worldSize()
{
    return 2.0 * M_PI * kEarthRadius;
}

QPointF geoToWorld(const QGeoCoordinate& coord)
{
    const double lat = std::clamp(coord.latitude(), -kMaxLatitude, kMaxLatitude);
    const double x = qDegreesToRadians(coord.longitude()) * kEarthRadius;
    const double y = std::log(std::tan(M_PI_4 + qDegreesToRadians(lat) / 2.0)) * kEarthRadius;
    return QPointF(x, y);
}

QGeoCoordinate worldToGeo(const QPointF& world)
{
    const double lon = qRadiansToDegrees(world.x() / kEarthRadius);
    const double lat = qRadiansToDegrees(2.0 * std::atan(std::exp(world.y() / kEarthRadius)) - M_PI_2);
    return QGeoCoordinate(lat, lon, 0);
}

double mercatorScale(double latitude)
{
    const double lat = std::clamp(latitude, -kMaxLatitude, kMaxLatitude);
    return 1.0 / std::cos(qDegreesToRadians(lat));
}

bool isValidKey(const TileKey& key)
{
    if ((key.zoom < kMinZoom) || (key.zoom > kMaxZoom)) {
        return false;
    }
    const int tilesAtZoom = 1 << key.zoom;
    return (key.x >= 0) && (key.x < tilesAtZoom) && (key.y >= 0) && (key.y < tilesAtZoom);
}

double tileSpanAtZoom(int zoom)
{
    return worldSize() / static_cast<double>(1 << std::clamp(zoom, kMinZoom, kMaxZoom));
}

QPointF tileMinCorner(const TileKey& key)
{
    const double span = tileSpanAtZoom(key.zoom);
    const double half = worldSize() / 2.0;
    const double minX = -half + (key.x * span);
    const double minY = half - ((key.y + 1) * span);
    return QPointF(minX, minY);
}

TileKey tileForWorld(const QPointF& world, int zoom)
{
    const int clampedZoom = std::clamp(zoom, kMinZoom, kMaxZoom);
    const double span = tileSpanAtZoom(clampedZoom);
    const double half = worldSize() / 2.0;
    const int tileCount = 1 << clampedZoom;

    const double x = std::clamp(world.x(), -half, half);
    const double y = std::clamp(world.y(), -half, half);

    TileKey key;
    key.zoom = clampedZoom;
    key.x = std::clamp(static_cast<int>(std::floor((x + half) / span)), 0, tileCount - 1);
    key.y = std::clamp(static_cast<int>(std::floor((half - y) / span)), 0, tileCount - 1);
    return key;
}

double metersPerPixelAtZoom(int zoom)
{
    return tileSpanAtZoom(zoom) / static_cast<double>(kTilePixels);
}

int zoomForMetersPerPixel(double metersPerPixel)
{
    if (metersPerPixel <= 0.0) {
        return kMaxZoom;
    }
    const double zoomExact = std::log2(metersPerPixelAtZoom(kMinZoom) / metersPerPixel);
    return std::clamp(static_cast<int>(std::ceil(zoomExact)), kMinZoom, kMaxZoom);
}

}  // namespace TileMath
