/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QDebug>
#include <QtCore/QHashFunctions>
#include <QtCore/QMetaType>
#include <QtCore/QPointF>
#include <QtPositioning/QGeoCoordinate>

/// Web Mercator (EPSG:3857) projection and slippy-tile math for the GeoMap engine.
///
/// World space: mercator meters, origin at (lon 0, equator), x east, y north, z up.
/// Note mercator meters are true meters only at the equator (scale grows with 1/cos(lat));
/// the rendered scene is self-consistent but distances are not geodesic.
///
/// Tile addressing: standard slippy scheme — tile (0,0) is the north-west corner,
/// x grows east, y grows south.
namespace TileMath {

constexpr double kEarthRadius = 6378137.0;    ///< WGS84 equatorial radius (m)
constexpr double kMaxLatitude = 85.05112878;  ///< Mercator latitude clamp (deg)
constexpr int kMinZoom = 0;
constexpr int kMaxZoom = 23;
constexpr int kTilePixels = 256;  ///< Nominal tile edge used for meters-per-pixel

struct TileKey
{
    int x = 0;
    int y = 0;
    int zoom = 0;

    bool operator==(const TileKey& other) const { return (x == other.x) && (y == other.y) && (zoom == other.zoom); }
};

inline size_t qHash(const TileKey& key, size_t seed = 0)
{
    return ::qHashMulti(seed, key.x, key.y, key.zoom);
}

/// Streams as slippy "zoom/x/y" notation for logging
QDebug operator<<(QDebug debug, const TileKey& key);

}  // namespace TileMath

Q_DECLARE_METATYPE(TileMath::TileKey)

namespace TileMath {

/// Full mercator world extent (2*pi*R) in world meters
double worldSize();

/// True if zoom is within [kMinZoom, kMaxZoom] and x/y address a tile at that zoom
bool isValidKey(const TileKey& key);

/// Geo -> world meters. Latitude is clamped to +/-kMaxLatitude.
QPointF geoToWorld(const QGeoCoordinate& coord);

/// World meters -> geo (altitude 0)
QGeoCoordinate worldToGeo(const QPointF& world);

/// Mercator scale factor 1/cos(lat): how many mercator meters one true ground
/// meter spans at the given latitude (clamped to +/-kMaxLatitude). Used to
/// convert true-meter heights/altitudes into mercator scene units so vertical
/// and horizontal geometry stay consistent away from the equator.
double mercatorScale(double latitude);

/// Edge length of one tile at zoom, in world meters
double tileSpanAtZoom(int zoom);

/// South-west (minimum x/y) corner of a tile in world meters
QPointF tileMinCorner(const TileKey& key);

/// Tile containing a world point. World coordinates are clamped to the world extent.
TileKey tileForWorld(const QPointF& world, int zoom);

/// World meters spanned by one pixel of a kTilePixels tile at zoom (equator)
double metersPerPixelAtZoom(int zoom);

/// Smallest zoom whose resolution is at least as fine as metersPerPixel (clamped to valid range)
int zoomForMetersPerPixel(double metersPerPixel);

}  // namespace TileMath
