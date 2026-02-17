#include "CoordFixtures.h"

namespace TestFixtures {
namespace Coord {

QList<QGeoCoordinate> polygon(const QGeoCoordinate& center, int sides, double radiusMeters)
{
    QList<QGeoCoordinate> coords;
    coords.reserve(sides);

    const double angleStep = 360.0 / sides;
    for (int i = 0; i < sides; ++i) {
        const double azimuth = i * angleStep;
        coords.append(center.atDistanceAndAzimuth(radiusMeters, azimuth));
    }

    return coords;
}

QList<QGeoCoordinate> simpleSquare()
{
    return polygon(zurich(), 4, 100.0);
}

QList<QGeoCoordinate> simpleTriangle()
{
    return polygon(zurich(), 3, 100.0);
}

QList<QGeoCoordinate> lShape()
{
    const QGeoCoordinate base = zurich();
    return {
        base,
        base.atDistanceAndAzimuth(100, 0),   // North
        base.atDistanceAndAzimuth(100, 45),  // Northeast (inner corner)
        base.atDistanceAndAzimuth(141, 45),  // Northeast (outer)
        base.atDistanceAndAzimuth(100, 90),  // East
        base.atDistanceAndAzimuth(0, 90),    // Back to base level, east side
    };
}

QList<QGeoCoordinate> waypointPath(const QGeoCoordinate& start, int count, double spacingMeters, double heading)
{
    QList<QGeoCoordinate> coords;
    coords.reserve(count);

    QGeoCoordinate current = start;
    for (int i = 0; i < count; ++i) {
        coords.append(current);
        current = current.atDistanceAndAzimuth(spacingMeters, heading);
    }

    return coords;
}

}  // namespace Coord
}  // namespace TestFixtures
