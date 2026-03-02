#pragma once

#include <QtCore/QList>
#include <QtPositioning/QGeoCoordinate>

/// @file
/// @brief Standard test coordinates and shapes

namespace TestFixtures {
namespace Coord {

/// Origin coordinate (0, 0, 0)
inline QGeoCoordinate origin()
{
    return QGeoCoordinate(0.0, 0.0, 0.0);
}

/// Zurich, Switzerland - standard test location with reasonable values
inline QGeoCoordinate zurich()
{
    return QGeoCoordinate(47.3977, 8.5456, 408.0);
}

/// San Francisco area - another common test location
inline QGeoCoordinate sanFrancisco()
{
    return QGeoCoordinate(37.7749, -122.4194, 16.0);
}

/// Seattle area - used by many mission manager tests
inline QGeoCoordinate seattle()
{
    return QGeoCoordinate(47.6335, -122.0890, 122.0);
}

/// North pole - edge case for latitude
inline QGeoCoordinate northPole()
{
    return QGeoCoordinate(90.0, 0.0, 0.0);
}

/// South pole - edge case for latitude
inline QGeoCoordinate southPole()
{
    return QGeoCoordinate(-90.0, 0.0, 0.0);
}

/// International date line (positive) - edge case for longitude
inline QGeoCoordinate dateLinePlus()
{
    return QGeoCoordinate(0.0, 180.0, 0.0);
}

/// International date line (negative) - edge case for longitude
inline QGeoCoordinate dateLineMinus()
{
    return QGeoCoordinate(0.0, -180.0, 0.0);
}

/// Common origin used by mission manager transect tests (Survey, Corridor, StructureScan)
/// Seattle area, no altitude (2D)
inline QGeoCoordinate missionTestOrigin()
{
    return QGeoCoordinate(47.633550640000003, -122.08982199);
}

/// Axis-aligned rectangle used by QGCMapPolygon and QGCMapPolyline tests
/// Seattle area, no altitude (2D)
QList<QGeoCoordinate> missionTestRectangle();

/// Generate a regular polygon centered at a coordinate
/// @param center Center point of the polygon
/// @param sides Number of sides (3 = triangle, 4 = square, etc.)
/// @param radiusMeters Distance from center to vertices
/// @return List of coordinates forming the polygon vertices
QList<QGeoCoordinate> polygon(const QGeoCoordinate& center, int sides, double radiusMeters = 100.0);

/// Simple square polygon at Zurich for quick testing
QList<QGeoCoordinate> simpleSquare();

/// Simple triangle polygon at Zurich
QList<QGeoCoordinate> simpleTriangle();

/// L-shaped polygon for testing concave shapes
QList<QGeoCoordinate> lShape();

/// Generate a path of waypoints from start coordinate
/// @param start Starting coordinate
/// @param count Number of waypoints
/// @param spacingMeters Distance between waypoints
/// @param heading Direction of path in degrees
/// @return List of waypoint coordinates
QList<QGeoCoordinate> waypointPath(const QGeoCoordinate& start, int count, double spacingMeters = 100.0,
                                   double heading = 0.0);

}  // namespace Coord
}  // namespace TestFixtures
