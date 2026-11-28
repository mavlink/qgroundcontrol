#pragma once

#include <cstdint>

#include <QtCore/QByteArray>
#include <QtCore/QJsonValue>
#include <QtCore/QtCompilerDetection>

/// The elevation tiles of https://terrain-ce.suite.auterion.com/api/v1/: the answer of the server and the form
/// the tile cache holds a tile in
namespace CopernicusTileFormat {

/// Each tile is a square of this many degrees of latitude and of longitude
inline constexpr double kTileSizeDegrees = 0.01;

/// The elevations of a tile are one arc second apart, which is about this many meters
inline constexpr double kValueSpacingDegrees = 1.0 / 3600;
inline constexpr double kValueSpacingMeters = 30.0;

/// Start of a cached tile. The elevations follow it as int16_t, row by row from the south west corner.
/// Packed where the compiler has Q_PACKED. MSVC has not: the tiles that its builds cached have the padded header, and
/// Q_PACKED after the struct would declare a variable of that name there.
#ifdef Q_PACKED
#define QGC_COPERNICUS_TILE_PACKED Q_PACKED
#else
#define QGC_COPERNICUS_TILE_PACKED
#endif
struct Header
{
    double swLat, swLon, neLat, neLon;
    int16_t minElevation, maxElevation;
    double avgElevation;
    int16_t gridSizeLat, gridSizeLon;
} QGC_COPERNICUS_TILE_PACKED;

#undef QGC_COPERNICUS_TILE_PACKED

/// The "data" of an answer of the server, null for an answer that is not a valid one
QJsonValue responseData(const QByteArray& response);

/// The cached form of an answer of the server, empty for an answer that is not a valid tile
QByteArray serialize(const QByteArray& response);

}  // namespace CopernicusTileFormat
