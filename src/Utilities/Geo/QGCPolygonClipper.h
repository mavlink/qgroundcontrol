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

}  // namespace QGCPolygonClipper
