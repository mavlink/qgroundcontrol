#include "QGCPlanarGeometry.h"

#include <algorithm>
#include <clipper2/clipper.h>
#include <cmath>
#include <limits>
#include <numeric>

namespace {

constexpr double maximumCoordinateScale = 1e14;
constexpr double minimumCoordinateScale = 1e6;
constexpr double miterLimit = 1000.0;
// 1 / sin(22.5 degrees): polygon offset corners sharper than 45 degrees are squared off instead of mitered
constexpr double polygonOffsetMiterLimit = 2.6131259297527531;
constexpr double normalizedPointTolerance = 1.0 / maximumCoordinateScale;
constexpr double intervalTolerance = 1e-12;
constexpr double sideTestErrorFactor = 64.0 * std::numeric_limits<double>::epsilon();
constexpr double normalizedCollinearTolerance = 1e-9;

/// Maps planar coordinates onto a unit extent around the bounding box center and from there onto
/// the Clipper2 integer grid.
struct CoordinateNormalizer
{
    QPointF center;
    double extent = 0.0;
    double scale = maximumCoordinateScale;

    [[nodiscard]] QPointF normalize(const QPointF& point) const
    {
        return QPointF((point.x() - center.x()) / extent, (point.y() - center.y()) / extent);
    }

    [[nodiscard]] QPointF denormalize(const QPointF& point) const
    {
        return QPointF((point.x() * extent) + center.x(), (point.y() * extent) + center.y());
    }

    [[nodiscard]] Clipper2Lib::Point64 toClipper(const QPointF& point) const
    {
        const QPointF normalized = normalize(point);
        return {std::llround(normalized.x() * scale), std::llround(normalized.y() * scale)};
    }

    [[nodiscard]] QPointF fromClipper(const Clipper2Lib::Point64& point) const
    {
        return denormalize(QPointF(static_cast<double>(point.x) / scale, static_cast<double>(point.y) / scale));
    }

    /// Lowers the integer scale so offset output, including mitered spikes, stays within Clipper2's range.
    [[nodiscard]] bool fitOffset(double distance)
    {
        const double normalizedReach = 1.0 + ((miterLimit + 1.0) * std::abs(distance) / extent);
        scale =
            (std::min) (maximumCoordinateScale, (0.5 * static_cast<double>(Clipper2Lib::MAX_COORD)) / normalizedReach);
        return std::isfinite(scale) && (scale >= minimumCoordinateScale);
    }

    [[nodiscard]] double clipperDistance(double distance) const { return (distance / extent) * scale; }
};

bool isFinitePoint(const QPointF& point)
{
    return std::isfinite(point.x()) && std::isfinite(point.y());
}

bool createNormalizer(const QList<QPointF>& points, CoordinateNormalizer& normalizer)
{
    if (points.isEmpty()) {
        return false;
    }

    double minimumX = std::numeric_limits<double>::infinity();
    double minimumY = std::numeric_limits<double>::infinity();
    double maximumX = -std::numeric_limits<double>::infinity();
    double maximumY = -std::numeric_limits<double>::infinity();
    for (const QPointF& point : points) {
        if (!isFinitePoint(point)) {
            return false;
        }
        minimumX = (std::min) (minimumX, point.x());
        minimumY = (std::min) (minimumY, point.y());
        maximumX = (std::max) (maximumX, point.x());
        maximumY = (std::max) (maximumY, point.y());
    }

    normalizer.center = QPointF(std::midpoint(minimumX, maximumX), std::midpoint(minimumY, maximumY));
    normalizer.extent = (std::max) (maximumX - minimumX, maximumY - minimumY);
    return std::isfinite(normalizer.extent) && (normalizer.extent > 0.0);
}

QList<QPointF> withoutClosingVertex(const QList<QPointF>& points)
{
    if ((points.size() > 1) && (points.first() == points.last())) {
        return points.first(points.size() - 1);
    }
    return points;
}

double crossProduct(const QPointF& first, const QPointF& second)
{
    return (first.x() * second.y()) - (first.y() * second.x());
}

double dotProduct(const QPointF& first, const QPointF& second)
{
    return (first.x() * second.x()) + (first.y() * second.y());
}

double distanceSquared(const QPointF& first, const QPointF& second)
{
    const double deltaX = first.x() - second.x();
    const double deltaY = first.y() - second.y();
    return (deltaX * deltaX) + (deltaY * deltaY);
}

/// True when every point lies on one line. Points are normalized to a unit extent.
bool isCollinear(const QList<QPointF>& points)
{
    QPointF farthest = points.first();
    double farthestDistanceSquared = 0.0;
    for (const QPointF& point : points) {
        const double pointDistanceSquared = distanceSquared(points.first(), point);
        if (pointDistanceSquared > farthestDistanceSquared) {
            farthest = point;
            farthestDistanceSquared = pointDistanceSquared;
        }
    }

    const QPointF direction = farthest - points.first();
    const double tolerance = normalizedCollinearTolerance * std::sqrt(farthestDistanceSquared);
    return std::all_of(points.cbegin(), points.cend(), [&points, &direction, tolerance](const QPointF& point) {
        return std::abs(crossProduct(direction, point - points.first())) <= tolerance;
    });
}

bool segmentsIntersect(const QPointF& firstStart, const QPointF& firstEnd, const QPointF& secondStart,
                       const QPointF& secondEnd)
{
    const auto side = [](const QPointF& lineStart, const QPointF& lineEnd, const QPointF& point) {
        const double cross = crossProduct(lineEnd - lineStart, point - lineStart);
        return (std::abs(cross) <= normalizedCollinearTolerance) ? 0 : ((cross > 0.0) ? 1 : -1);
    };
    const auto withinBounds = [](const QPointF& segmentStart, const QPointF& segmentEnd, const QPointF& point) {
        return (point.x() >= ((std::min) (segmentStart.x(), segmentEnd.x()) - normalizedCollinearTolerance)) &&
               (point.x() <= ((std::max) (segmentStart.x(), segmentEnd.x()) + normalizedCollinearTolerance)) &&
               (point.y() >= ((std::min) (segmentStart.y(), segmentEnd.y()) - normalizedCollinearTolerance)) &&
               (point.y() <= ((std::max) (segmentStart.y(), segmentEnd.y()) + normalizedCollinearTolerance));
    };

    const int secondStartSide = side(firstStart, firstEnd, secondStart);
    const int secondEndSide = side(firstStart, firstEnd, secondEnd);
    const int firstStartSide = side(secondStart, secondEnd, firstStart);
    const int firstEndSide = side(secondStart, secondEnd, firstEnd);
    if (((secondStartSide * secondEndSide) < 0) && ((firstStartSide * firstEndSide) < 0)) {
        return true;
    }
    return ((secondStartSide == 0) && withinBounds(firstStart, firstEnd, secondStart)) ||
           ((secondEndSide == 0) && withinBounds(firstStart, firstEnd, secondEnd)) ||
           ((firstStartSide == 0) && withinBounds(secondStart, secondEnd, firstStart)) ||
           ((firstEndSide == 0) && withinBounds(secondStart, secondEnd, firstEnd));
}

/// True when two non-adjacent edges of a closed ring touch or cross
bool ringSelfIntersects(const QList<QPointF>& ring)
{
    const qsizetype edgeCount = ring.size();
    for (qsizetype first = 0; first < edgeCount; ++first) {
        for (qsizetype second = first + 2; second < edgeCount; ++second) {
            if ((first == 0) && (second == (edgeCount - 1))) {
                continue;
            }
            if (segmentsIntersect(ring[first], ring[(first + 1) % edgeCount], ring[second],
                                  ring[(second + 1) % edgeCount])) {
                return true;
            }
        }
    }
    return false;
}

void appendUnique(QList<QPointF>& points, const QPointF& point)
{
    if (points.isEmpty() ||
        (distanceSquared(points.last(), point) > (normalizedPointTolerance * normalizedPointTolerance))) {
        points.append(point);
    }
}

struct OffsetSegment
{
    QPointF direction;
    QPointF leftNormal;
};

void appendOffsetJoin(QList<QPointF>& result, const QPointF& vertex, const OffsetSegment& previousSegment,
                      const OffsetSegment& nextSegment, double distance)
{
    const QPointF previousOffset = vertex + (previousSegment.leftNormal * distance);
    const QPointF nextOffset = vertex + (nextSegment.leftNormal * distance);
    const double denominator = crossProduct(previousSegment.direction, nextSegment.direction);
    const double directionDotProduct = dotProduct(previousSegment.direction, nextSegment.direction);

    if (std::abs(denominator) <= normalizedPointTolerance) {
        if (directionDotProduct > 0.0) {
            appendUnique(result, nextOffset);
        } else {
            appendUnique(result, previousOffset);
            appendUnique(result, nextOffset);
        }
        return;
    }

    const double previousLineDistance = crossProduct(nextOffset - previousOffset, nextSegment.direction) / denominator;
    const QPointF intersection = previousOffset + (previousSegment.direction * previousLineDistance);
    const double maximumMiterDistance = std::abs(distance) * miterLimit;
    if (isFinitePoint(intersection) &&
        (distanceSquared(vertex, intersection) <= (maximumMiterDistance * maximumMiterDistance))) {
        appendUnique(result, intersection);
    } else {
        appendUnique(result, previousOffset);
        appendUnique(result, nextOffset);
    }
}

Clipper2Lib::Path64 toClipperPath(const QList<QPointF>& path, const CoordinateNormalizer& normalizer)
{
    Clipper2Lib::Path64 result;
    result.reserve(path.size());
    for (const QPointF& point : path) {
        result.push_back(normalizer.toClipper(point));
    }
    return result;
}

QList<QPolygonF> fromClipperPaths(const Clipper2Lib::Paths64& paths, const CoordinateNormalizer& normalizer)
{
    QList<QPolygonF> result;
    result.reserve(static_cast<qsizetype>(paths.size()));
    for (const Clipper2Lib::Path64& path : paths) {
        if (path.size() < 3) {
            continue;
        }

        QPolygonF polygon;
        polygon.reserve(static_cast<qsizetype>(path.size()));
        for (const Clipper2Lib::Point64& point : path) {
            polygon.append(normalizer.fromClipper(point));
        }
        result.append(polygon);
    }
    return result;
}

double absoluteArea(const QPolygonF& polygon)
{
    double area = 0.0;
    for (qsizetype index = 0; index < polygon.size(); ++index) {
        area += crossProduct(polygon[index], polygon[(index + 1) % polygon.size()]);
    }
    return std::abs(area) / 2.0;
}

void alignPolygonStart(QPolygonF& polygon, const QPointF& sourceStart)
{
    if (polygon.isEmpty()) {
        return;
    }

    qsizetype nearestIndex = 0;
    double nearestDistanceSquared = std::numeric_limits<double>::infinity();
    for (qsizetype index = 0; index < polygon.size(); ++index) {
        const double vertexDistanceSquared = distanceSquared(polygon[index], sourceStart);
        if (vertexDistanceSquared < nearestDistanceSquared) {
            nearestIndex = index;
            nearestDistanceSquared = vertexDistanceSquared;
        }
    }
    std::rotate(polygon.begin(), polygon.begin() + nearestIndex, polygon.end());
}

struct LineInterval
{
    double start = 0.0;
    double end = 0.0;
};

void appendInterval(QList<LineInterval>& intervals, double start, double end)
{
    start = std::clamp(start, 0.0, 1.0);
    end = std::clamp(end, 0.0, 1.0);
    if ((end - start) > intervalTolerance) {
        intervals.append({start, end});
    }
}

QList<QLineF> linesFromIntervals(const QLineF& line, QList<LineInterval> intervals)
{
    std::sort(intervals.begin(), intervals.end(),
              [](const LineInterval& first, const LineInterval& second) { return first.start < second.start; });

    QList<LineInterval> mergedIntervals;
    for (const LineInterval& interval : intervals) {
        if (!mergedIntervals.isEmpty() && (interval.start <= (mergedIntervals.last().end + intervalTolerance))) {
            mergedIntervals.last().end = (std::max) (mergedIntervals.last().end, interval.end);
        } else {
            mergedIntervals.append(interval);
        }
    }

    QList<QLineF> result;
    result.reserve(mergedIntervals.size());
    for (const LineInterval& interval : mergedIntervals) {
        result.append(QLineF(line.pointAt(interval.start), line.pointAt(interval.end)));
    }
    return result;
}

/// Clips a line to a closed ring (no repeated closing vertex) using the odd-even rule. Each vertex is
/// classified once with a half-open side test, so the crossing count stays even when the line passes
/// exactly through vertices. Edges lying on the line are added separately as boundary coverage.
QList<QLineF> clipLineToRing(const QLineF& line, const QList<QPointF>& ring)
{
    if (!isFinitePoint(line.p1()) || !isFinitePoint(line.p2())) {
        return {};
    }

    const double directionX = line.dx();
    const double directionY = line.dy();
    const double directionLengthSquared = (directionX * directionX) + (directionY * directionY);
    if (!std::isfinite(directionLengthSquared) || (directionLengthSquared <= 0.0)) {
        return {};
    }

    const qsizetype vertexCount = ring.size();
    QList<double> sideDistances(vertexCount);
    QList<bool> onLine(vertexCount);
    for (qsizetype index = 0; index < vertexCount; ++index) {
        const double alongTerm = directionX * (ring[index].y() - line.y1());
        const double acrossTerm = directionY * (ring[index].x() - line.x1());
        sideDistances[index] = alongTerm - acrossTerm;
        onLine[index] =
            std::abs(sideDistances[index]) <= (sideTestErrorFactor * (std::abs(alongTerm) + std::abs(acrossTerm)));
    }

    const auto lineParameter = [&line, directionX, directionY, directionLengthSquared](const QPointF& point) {
        return (((point.x() - line.x1()) * directionX) + ((point.y() - line.y1()) * directionY)) /
               directionLengthSquared;
    };

    QList<double> crossings;
    QList<LineInterval> intervals;
    for (qsizetype index = 0; index < vertexCount; ++index) {
        const qsizetype nextIndex = (index + 1) % vertexCount;
        const double startSide = sideDistances[index];
        const double endSide = sideDistances[nextIndex];
        if ((startSide > 0.0) != (endSide > 0.0)) {
            const double edgeParameter = startSide / (startSide - endSide);
            crossings.append(lineParameter(ring[index] + ((ring[nextIndex] - ring[index]) * edgeParameter)));
        }
        if (onLine[index] && onLine[nextIndex]) {
            const double startParameter = lineParameter(ring[index]);
            const double endParameter = lineParameter(ring[nextIndex]);
            appendInterval(intervals, (std::min) (startParameter, endParameter),
                           (std::max) (startParameter, endParameter));
        }
    }

    std::sort(crossings.begin(), crossings.end());
    for (qsizetype index = 1; index < crossings.size(); index += 2) {
        appendInterval(intervals, crossings[index - 1], crossings[index]);
    }
    return linesFromIntervals(line, intervals);
}

}  // namespace

QList<QLineF> QGCPlanarGeometry::intersectLine(const QLineF& line, const QPolygonF& polygon)
{
    const QList<QList<QLineF>> results = intersectLines({line}, polygon);
    return results.isEmpty() ? QList<QLineF>{} : results.first();
}

QList<QList<QLineF>> QGCPlanarGeometry::intersectLines(const QList<QLineF>& lines, const QPolygonF& polygon)
{
    QList<QList<QLineF>> results(lines.size());

    const QList<QPointF> ring = withoutClosingVertex(polygon);
    if ((ring.size() < 3) || !std::all_of(ring.cbegin(), ring.cend(), isFinitePoint)) {
        return results;
    }

    for (qsizetype index = 0; index < lines.size(); ++index) {
        results[index] = clipLineToRing(lines[index], ring);
    }
    return results;
}

QList<QPolygonF> QGCPlanarGeometry::offsetPolygon(const QPolygonF& polygon, double distance)
{
    if (!std::isfinite(distance)) {
        return {};
    }

    const QList<QPointF> ring = withoutClosingVertex(polygon);
    if (ring.size() < 3) {
        return {};
    }

    CoordinateNormalizer normalizer;
    if (!createNormalizer(ring, normalizer)) {
        return {};
    }
    if (qFuzzyIsNull(distance)) {
        return {polygon};
    }
    if (!normalizer.fitOffset(distance)) {
        return {};
    }

    const Clipper2Lib::Paths64 resultPaths =
        Clipper2Lib::InflatePaths({toClipperPath(ring, normalizer)}, normalizer.clipperDistance(distance),
                                  Clipper2Lib::JoinType::Miter, Clipper2Lib::EndType::Polygon, polygonOffsetMiterLimit);
    QList<QPolygonF> result = fromClipperPaths(resultPaths, normalizer);
    for (QPolygonF& resultPolygon : result) {
        alignPolygonStart(resultPolygon, ring.first());
    }
    return result;
}

QList<QPointF> QGCPlanarGeometry::offsetPolyline(const QList<QPointF>& polyline, double distance)
{
    if ((polyline.size() < 2) || !std::isfinite(distance)) {
        return {};
    }

    CoordinateNormalizer normalizer;
    if (!createNormalizer(polyline, normalizer)) {
        return {};
    }
    if (qFuzzyIsNull(distance)) {
        return polyline;
    }

    QList<QPointF> normalizedPolyline;
    normalizedPolyline.reserve(polyline.size());
    for (const QPointF& point : polyline) {
        appendUnique(normalizedPolyline, normalizer.normalize(point));
    }

    if (normalizedPolyline.size() < 2) {
        return {};
    }

    // A path ending where it starts is closed, unless it only runs out and back along a line
    if ((normalizedPolyline.size() > 3) &&
        (distanceSquared(normalizedPolyline.first(), normalizedPolyline.last()) <=
         (normalizedPointTolerance * normalizedPointTolerance)) &&
        !isCollinear(normalizedPolyline)) {
        normalizedPolyline.removeLast();

        // A closed path that crosses itself has no consistent inside to offset towards
        if (ringSelfIntersects(normalizedPolyline)) {
            return {};
        }

        QPolygonF ring;
        ring.reserve(normalizedPolyline.size());
        double twiceSignedArea = 0.0;
        for (qsizetype index = 0; index < normalizedPolyline.size(); ++index) {
            ring.append(normalizer.denormalize(normalizedPolyline[index]));
            twiceSignedArea +=
                crossProduct(normalizedPolyline[index], normalizedPolyline[(index + 1) % normalizedPolyline.size()]);
        }

        // Left of travel is the interior of a counterclockwise ring, so an offset to the left shrinks it. Polygon
        // offsetting removes the parts of an inward offset that collapse, which a parallel path cannot represent.
        const QList<QPolygonF> offsetRings = offsetPolygon(ring, (twiceSignedArea > 0.0) ? -distance : distance);
        if (offsetRings.size() != 1) {
            return {};
        }
        QList<QPointF> result = offsetRings.first();
        result.append(result.first());
        return result;
    }

    const qsizetype segmentCount = normalizedPolyline.size() - 1;
    QList<OffsetSegment> segments;
    segments.reserve(segmentCount);
    for (qsizetype index = 0; index < segmentCount; ++index) {
        const QPointF delta = normalizedPolyline[index + 1] - normalizedPolyline[index];
        const double length = std::hypot(delta.x(), delta.y());
        if (!std::isfinite(length) || (length <= normalizedPointTolerance)) {
            return {};
        }
        const QPointF direction(delta.x() / length, delta.y() / length);
        segments.append({direction, QPointF(-direction.y(), direction.x())});
    }

    const double normalizedDistance = distance / normalizer.extent;
    if (!std::isfinite(normalizedDistance)) {
        return {};
    }

    QList<QPointF> normalizedResult;
    normalizedResult.reserve(normalizedPolyline.size() + 2);
    appendUnique(normalizedResult, normalizedPolyline.first() + (segments.first().leftNormal * normalizedDistance));
    for (qsizetype index = 1; index < normalizedPolyline.size() - 1; ++index) {
        appendOffsetJoin(normalizedResult, normalizedPolyline[index], segments[index - 1], segments[index],
                         normalizedDistance);
    }
    appendUnique(normalizedResult, normalizedPolyline.last() + (segments.last().leftNormal * normalizedDistance));

    QList<QPointF> result;
    result.reserve(normalizedResult.size());
    for (const QPointF& point : normalizedResult) {
        result.append(normalizer.denormalize(point));
    }
    return result;
}

QList<QPolygonF> QGCPlanarGeometry::bufferPolyline(const QList<QPointF>& polyline, double distance)
{
    if ((polyline.size() < 2) || !std::isfinite(distance) || (distance <= 0.0)) {
        return {};
    }

    CoordinateNormalizer normalizer;
    if (!createNormalizer(polyline, normalizer) || !normalizer.fitOffset(distance)) {
        return {};
    }

    Clipper2Lib::Path64 clipperPath = toClipperPath(polyline, normalizer);
    clipperPath.erase(std::unique(clipperPath.begin(), clipperPath.end()), clipperPath.end());
    QList<QPointF> normalizedPolyline;
    normalizedPolyline.reserve(polyline.size());
    for (const QPointF& point : polyline) {
        normalizedPolyline.append(normalizer.normalize(point));
    }
    Clipper2Lib::EndType endType = Clipper2Lib::EndType::Butt;
    // Out-and-back paths are buffered as open paths, matching offsetPolyline
    if ((clipperPath.size() > 3) && (clipperPath.front() == clipperPath.back()) && !isCollinear(normalizedPolyline)) {
        clipperPath.pop_back();
        endType = Clipper2Lib::EndType::Joined;
    }

    const Clipper2Lib::Paths64 resultPaths = Clipper2Lib::InflatePaths(
        {clipperPath}, normalizer.clipperDistance(distance), Clipper2Lib::JoinType::Miter, endType, miterLimit);
    QList<QPolygonF> contours = fromClipperPaths(resultPaths, normalizer);
    std::stable_sort(contours.begin(), contours.end(), [](const QPolygonF& first, const QPolygonF& second) {
        return absoluteArea(first) > absoluteArea(second);
    });
    return contours;
}

QPolygonF QGCPlanarGeometry::joinContours(const QList<QPolygonF>& contours)
{
    if (contours.isEmpty()) {
        return {};
    }
    if (contours.size() == 1) {
        return contours.first();
    }

    qsizetype pointCount = contours.first().size() + 1;
    for (qsizetype index = 1; index < contours.size(); ++index) {
        pointCount += contours[index].size() + 2;
    }

    QPolygonF result;
    result.reserve(pointCount);
    result.append(contours.first());

    const QPointF anchor = result.first();
    result.append(anchor);
    for (qsizetype index = 1; index < contours.size(); ++index) {
        const QPolygonF& contour = contours[index];
        result.append(contour);
        result.append(contour.first());
        result.append(anchor);
    }
    return result;
}
