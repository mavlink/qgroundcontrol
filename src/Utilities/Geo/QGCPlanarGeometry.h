#pragma once

#include <QtCore/QLineF>
#include <QtCore/QList>
#include <QtCore/QPointF>
#include <QtGui/QPolygonF>

/// Geometry operations on planar (local tangent plane) coordinates.
namespace QGCPlanarGeometry {

/// Clips a directed line segment to a planar polygon using the odd-even fill rule.
/// Polygon edges lying on the line are kept. Results are ordered from line.p1() to line.p2().
[[nodiscard]] QList<QLineF> intersectLine(const QLineF& line, const QPolygonF& polygon);

/// Clips directed line segments to a planar polygon.
/// The result at each index contains the ordered fragments for the corresponding input line.
[[nodiscard]] QList<QList<QLineF>> intersectLines(const QList<QLineF>& lines, const QPolygonF& polygon);

/// Offsets a planar polygon by distance. Positive distances expand the polygon.
/// Corners sharper than 45 degrees are squared off rather than mitered, so the result stays near the
/// requested distance. Output winding matches the input. Multiple polygons can be returned when an
/// inward offset separates a concave polygon.
[[nodiscard]] QList<QPolygonF> offsetPolygon(const QPolygonF& polygon, double distance);

/// Builds a directed parallel path. Positive distances offset to the left of travel.
/// A path ending where it starts is closed, unless all its points lie on one line, and produces an explicitly closed
/// output. The output is empty when an inward offset collapses the path or when a closed path crosses itself.
[[nodiscard]] QList<QPointF> offsetPolyline(const QList<QPointF>& polyline, double distance);

/// Builds the planar area within distance of a polyline and returns its boundary contours, outer
/// boundary first. Any further contours are holes, such as the interior of a closed path. Paths are closed
/// following the same rule as offsetPolyline.
[[nodiscard]] QList<QPolygonF> bufferPolyline(const QList<QPointF>& polyline, double distance);

/// Joins contours into a single ring using zero-width bridges, for use with odd-even polygon filling.
[[nodiscard]] QPolygonF joinContours(const QList<QPolygonF>& contours);

}  // namespace QGCPlanarGeometry
