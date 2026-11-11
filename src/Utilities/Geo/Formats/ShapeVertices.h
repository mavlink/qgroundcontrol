#pragma once

#include <QtCore/QList>
#include <QtPositioning/QGeoCoordinate>

/// Clean-up that the shape file loaders apply to the vertices they have read.
namespace ShapeVertices {

/// Removes the last vertex of a ring when it repeats the first one: QGC polygons close implicitly.
/// Only a vertex within 0.01 m of the first is that repeat, so a distinct vertex near the start stays,
/// and a ring of three vertices is left alone.
void removeClosingVertex(QList<QGeoCoordinate>& vertices);

/// Removes each vertex closer than @a filterMeters to the vertex kept before it, without going below
/// @a minVertices. The last vertex is not compared with the first.
/// @param filterMeters 0 or less disables the filter
void filterNearby(QList<QGeoCoordinate>& vertices, double filterMeters, int minVertices);

}  // namespace ShapeVertices
