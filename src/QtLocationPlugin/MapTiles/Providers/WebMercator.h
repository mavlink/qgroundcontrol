#pragma once

#include <cmath>

#include <QtCore/QtMath>

/// The projection of the maps and their tiles: where a longitude and a latitude lie on the square map of the world,
/// from 0 at its west and north edges to 1 at its east and south edges. At the zoom level z the map is 2^z tiles
/// wide and high.
///
/// The expressions are those that the indices of the tiles have always been computed with. Another form of them
/// gives another tile for a place on the edge of one, and the tile cache and the offline sets are keyed by tiles.
namespace WebMercator {

inline double xOfLongitude(double longitude)
{
    return (longitude + 180.0) / 360.0;
}

/// Beyond 0 and 1 for a latitude that the map does not have: it ends near 85.05 degrees north and south
inline double yOfLatitude(double latitude)
{
    const double radians = latitude * M_PI / 180.0;
    return (1.0 - (std::log(std::tan(radians) + (1.0 / std::cos(radians))) / M_PI)) / 2.0;
}

inline double longitudeOfX(double x)
{
    return (x * 360.0) - 180.0;
}

inline double latitudeOfY(double y)
{
    return qRadiansToDegrees(std::atan(std::sinh(M_PI - (2.0 * M_PI * y))));
}

}  // namespace WebMercator
