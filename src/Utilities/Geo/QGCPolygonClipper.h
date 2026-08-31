#pragma once

#include <QtCore/QLineF>
#include <QtCore/QList>
#include <QtGui/QPolygonF>

namespace QGCPolygonClipper {

/// Clips a directed line segment to a planar polygon.
/// Results are ordered from line.p1() to line.p2() and preserve that direction.
[[nodiscard]] QList<QLineF> intersectLine(const QLineF& line, const QPolygonF& polygon);

/// Clips directed line segments to a planar polygon.
/// The result at each index contains the ordered fragments for the corresponding input line.
[[nodiscard]] QList<QList<QLineF>> intersectLines(const QList<QLineF>& lines, const QPolygonF& polygon);

/// Offsets a planar polygon by distance. Positive distances expand the polygon.
/// Multiple polygons can be returned when an inward offset separates a concave polygon.
[[nodiscard]] QList<QPolygonF> offsetPolygon(const QPolygonF& polygon, double distance);

/// Builds a directed parallel path. Positive distances offset to the left of travel.
/// Explicitly closed input produces explicitly closed output.
[[nodiscard]] QList<QPointF> offsetPolyline(const QList<QPointF>& polyline, double distance);

/// Builds a planar buffer around a polyline. Explicitly closed paths preserve their interior hole.
/// Multiple contours are joined by zero-width bridges for use with odd-even polygon filling.
[[nodiscard]] QPolygonF bufferPolyline(const QList<QPointF>& polyline, double distance);

}  // namespace QGCPolygonClipper
