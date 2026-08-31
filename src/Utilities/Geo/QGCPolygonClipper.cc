#include "QGCPolygonClipper.h"

#include <algorithm>
#include <clipper2/clipper.h>
#include <cmath>
#include <limits>
#include <numeric>

namespace {

constexpr double coordinateScale = 1e14;
constexpr double miterLimit = 1000.0;
constexpr double maximumScaledOffset = 4e18 / (miterLimit + 1.0);
constexpr double normalizedPointTolerance = 1.0 / coordinateScale;

struct CoordinateNormalizer
{
    QPointF center;
    double extent = 0.0;

    [[nodiscard]] bool isValid() const { return std::isfinite(extent) && (extent > 0.0); }

    [[nodiscard]] QPointF normalize(const QPointF& point) const
    {
        return QPointF((point.x() - center.x()) / extent, (point.y() - center.y()) / extent);
    }

    [[nodiscard]] QPointF denormalize(const QPointF& point) const
    {
        return QPointF((point.x() * extent) + center.x(), (point.y() * extent) + center.y());
    }
};

struct Bounds
{
    double minimumX = std::numeric_limits<double>::infinity();
    double minimumY = std::numeric_limits<double>::infinity();
    double maximumX = -std::numeric_limits<double>::infinity();
    double maximumY = -std::numeric_limits<double>::infinity();

    bool add(const QPointF& point)
    {
        if (!std::isfinite(point.x()) || !std::isfinite(point.y())) {
            return false;
        }

        minimumX = (std::min) (minimumX, point.x());
        minimumY = (std::min) (minimumY, point.y());
        maximumX = (std::max) (maximumX, point.x());
        maximumY = (std::max) (maximumY, point.y());
        return true;
    }

    bool configure(CoordinateNormalizer& normalizer) const
    {
        if (!std::isfinite(minimumX)) {
            return false;
        }

        normalizer.center = QPointF(std::midpoint(minimumX, maximumX), std::midpoint(minimumY, maximumY));
        normalizer.extent = (std::max) (maximumX - minimumX, maximumY - minimumY);
        return normalizer.isValid();
    }
};

bool createNormalizer(const QPolygonF& polygon, CoordinateNormalizer& normalizer)
{
    Bounds bounds;
    for (const QPointF& point : polygon) {
        if (!bounds.add(point)) {
            return false;
        }
    }
    return bounds.configure(normalizer);
}

bool createNormalizer(const QList<QPointF>& points, CoordinateNormalizer& normalizer)
{
    Bounds bounds;
    for (const QPointF& point : points) {
        if (!bounds.add(point)) {
            return false;
        }
    }
    return bounds.configure(normalizer);
}

QPolygonF normalizedPath(const QPolygonF& polygon, const CoordinateNormalizer& normalizer)
{
    qsizetype pointCount = polygon.size();
    if ((pointCount > 1) && (polygon.first() == polygon.last())) {
        --pointCount;
    }

    QPolygonF result;
    result.reserve(pointCount);
    for (qsizetype index = 0; index < pointCount; ++index) {
        result.append(normalizer.normalize(polygon[index]));
    }
    return result;
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
    if (std::isfinite(intersection.x()) && std::isfinite(intersection.y()) &&
        (distanceSquared(vertex, intersection) <= (maximumMiterDistance * maximumMiterDistance))) {
        appendUnique(result, intersection);
    } else {
        appendUnique(result, previousOffset);
        appendUnique(result, nextOffset);
    }
}

Clipper2Lib::Point64 toClipperPoint(const QPointF& point)
{
    return {std::llround(point.x() * coordinateScale), std::llround(point.y() * coordinateScale)};
}

QPointF fromClipperPoint(const Clipper2Lib::Point64& point)
{
    return QPointF(static_cast<double>(point.x) / coordinateScale, static_cast<double>(point.y) / coordinateScale);
}

Clipper2Lib::Path64 toClipperPath(const QPolygonF& path)
{
    Clipper2Lib::Path64 result;
    result.reserve(path.size());
    for (const QPointF& point : path) {
        result.push_back(toClipperPoint(point));
    }
    return result;
}

Clipper2Lib::Path64 toClipperPath(const QList<QPointF>& path, const CoordinateNormalizer& normalizer)
{
    Clipper2Lib::Path64 result;
    result.reserve(path.size());
    for (const QPointF& point : path) {
        result.push_back(toClipperPoint(normalizer.normalize(point)));
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
            polygon.append(normalizer.denormalize(fromClipperPoint(point)));
        }
        result.append(polygon);
    }
    return result;
}

QPolygonF fromClipperCompoundPath(const Clipper2Lib::Paths64& paths, const CoordinateNormalizer& normalizer)
{
    const QList<QPolygonF> polygons = fromClipperPaths(paths, normalizer);
    if (polygons.isEmpty()) {
        return {};
    }
    if (polygons.size() == 1) {
        return polygons.first();
    }

    qsizetype pointCount = polygons.first().size() + 1;
    for (qsizetype index = 1; index < polygons.size(); ++index) {
        pointCount += polygons[index].size() + 2;
    }

    QPolygonF result;
    result.reserve(pointCount);
    result.append(polygons.first());

    const QPointF anchor = result.first();
    result.append(anchor);
    for (qsizetype index = 1; index < polygons.size(); ++index) {
        const QPolygonF& polygon = polygons[index];
        result.append(polygon.first());
        for (qsizetype pointIndex = 1; pointIndex < polygon.size(); ++pointIndex) {
            result.append(polygon[pointIndex]);
        }
        result.append(polygon.first());
        result.append(anchor);
    }
    return result;
}

void alignPolygonStart(QPolygonF& polygon, const QPointF& sourceStart)
{
    if (polygon.isEmpty()) {
        return;
    }

    qsizetype nearestIndex = 0;
    double nearestDistanceSquared = std::numeric_limits<double>::infinity();
    for (qsizetype index = 0; index < polygon.size(); ++index) {
        const double deltaX = polygon[index].x() - sourceStart.x();
        const double deltaY = polygon[index].y() - sourceStart.y();
        const double distanceSquared = (deltaX * deltaX) + (deltaY * deltaY);
        if (distanceSquared < nearestDistanceSquared) {
            nearestIndex = index;
            nearestDistanceSquared = distanceSquared;
        }
    }
    std::rotate(polygon.begin(), polygon.begin() + nearestIndex, polygon.end());
}

struct LineInterval
{
    double start = 0.0;
    double end = 0.0;
};

constexpr double intervalTolerance = 1e-12;

bool isDegenerateLine(const QLineF& line)
{
    return (line.dx() == 0.0) && (line.dy() == 0.0);
}

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

QList<QLineF> intersectCollapsedLine(const QLineF& line, const QPolygonF& polygon)
{
    const long double directionX = static_cast<long double>(line.dx());
    const long double directionY = static_cast<long double>(line.dy());
    const long double directionLengthSquared = (directionX * directionX) + (directionY * directionY);
    if (directionLengthSquared <= 0.0L) {
        return {};
    }

    qsizetype pointCount = polygon.size();
    if ((pointCount > 1) && (polygon.first() == polygon.last())) {
        --pointCount;
    }

    constexpr long double parameterTolerance = 1e-12L;
    constexpr long double crossProductTolerance = 64.0L * std::numeric_limits<long double>::epsilon();
    QList<double> intersectionParameters = {0.0, 1.0};
    QList<LineInterval> intervals;
    for (qsizetype index = 0; index < pointCount; ++index) {
        const QPointF& edgeStart = polygon[index];
        const QPointF& edgeEnd = polygon[(index + 1) % pointCount];
        const long double edgeDirectionX = static_cast<long double>(edgeEnd.x()) - edgeStart.x();
        const long double edgeDirectionY = static_cast<long double>(edgeEnd.y()) - edgeStart.y();
        const long double offsetX = static_cast<long double>(edgeStart.x()) - line.x1();
        const long double offsetY = static_cast<long double>(edgeStart.y()) - line.y1();
        const long double denominator = (directionX * edgeDirectionY) - (directionY * edgeDirectionX);
        const long double denominatorError = crossProductTolerance * ((std::abs) (directionX * edgeDirectionY) +
                                                                      (std::abs) (directionY * edgeDirectionX));

        if ((std::abs) (denominator) > denominatorError) {
            const long double lineParameter = ((offsetX * edgeDirectionY) - (offsetY * edgeDirectionX)) / denominator;
            const long double edgeParameter = ((offsetX * directionY) - (offsetY * directionX)) / denominator;
            if ((lineParameter >= -parameterTolerance) && (lineParameter <= (1.0L + parameterTolerance)) &&
                (edgeParameter >= -parameterTolerance) && (edgeParameter <= (1.0L + parameterTolerance))) {
                intersectionParameters.append(std::clamp(static_cast<double>(lineParameter), 0.0, 1.0));
            }
            continue;
        }

        const long double offsetCrossProduct = (offsetX * directionY) - (offsetY * directionX);
        const long double offsetError =
            crossProductTolerance * ((std::abs) (offsetX * directionY) + (std::abs) (offsetY * directionX));
        if ((std::abs) (offsetCrossProduct) > offsetError) {
            continue;
        }

        const long double edgeEndOffsetX = static_cast<long double>(edgeEnd.x()) - line.x1();
        const long double edgeEndOffsetY = static_cast<long double>(edgeEnd.y()) - line.y1();
        const double edgeStartParameter =
            static_cast<double>(((offsetX * directionX) + (offsetY * directionY)) / directionLengthSquared);
        const double edgeEndParameter = static_cast<double>(
            ((edgeEndOffsetX * directionX) + (edgeEndOffsetY * directionY)) / directionLengthSquared);
        appendInterval(intervals, (std::min) (edgeStartParameter, edgeEndParameter),
                       (std::max) (edgeStartParameter, edgeEndParameter));
    }

    std::sort(intersectionParameters.begin(), intersectionParameters.end());
    const auto uniqueEnd =
        std::unique(intersectionParameters.begin(), intersectionParameters.end(),
                    [](double first, double second) { return std::abs(first - second) <= intervalTolerance; });
    intersectionParameters.erase(uniqueEnd, intersectionParameters.end());

    for (qsizetype index = 1; index < intersectionParameters.size(); ++index) {
        const double start = intersectionParameters[index - 1];
        const double end = intersectionParameters[index];
        if (((end - start) > intervalTolerance) &&
            polygon.containsPoint(line.pointAt(std::midpoint(start, end)), Qt::OddEvenFill)) {
            appendInterval(intervals, start, end);
        }
    }

    return linesFromIntervals(line, intervals);
}

bool clipLineToBounds(const QLineF& line, const QRectF& bounds, QLineF& clippedLine)
{
    if (isDegenerateLine(line) || !std::isfinite(line.x1()) || !std::isfinite(line.y1()) || !std::isfinite(line.x2()) ||
        !std::isfinite(line.y2())) {
        return false;
    }

    double start = 0.0;
    double end = 1.0;
    const auto clipBoundary = [&start, &end](double direction, double distance) {
        if (direction == 0.0) {
            return distance >= 0.0;
        }

        const double parameter = distance / direction;
        if (direction < 0.0) {
            start = (std::max) (start, parameter);
        } else {
            end = (std::min) (end, parameter);
        }
        return start <= end;
    };

    if (!clipBoundary(-line.dx(), line.x1() - bounds.left()) || !clipBoundary(line.dx(), bounds.right() - line.x1()) ||
        !clipBoundary(-line.dy(), line.y1() - bounds.top()) || !clipBoundary(line.dy(), bounds.bottom() - line.y1())) {
        return false;
    }

    clippedLine = QLineF(line.pointAt(start), line.pointAt(end));
    return !isDegenerateLine(clippedLine);
}

QList<QLineF> intersectNormalizedLine(const QLineF& line, const Clipper2Lib::Path64& clipPath,
                                      const CoordinateNormalizer& normalizer, const QPolygonF& polygon)
{
    if (isDegenerateLine(line)) {
        return {};
    }

    const QPointF normalizedLineStart = normalizer.normalize(line.p1());
    const QPointF normalizedLineEnd = normalizer.normalize(line.p2());
    const double directionX = normalizedLineEnd.x() - normalizedLineStart.x();
    const double directionY = normalizedLineEnd.y() - normalizedLineStart.y();

    const Clipper2Lib::Point64 clipperLineStart = toClipperPoint(normalizedLineStart);
    const Clipper2Lib::Point64 clipperLineEnd = toClipperPoint(normalizedLineEnd);
    if (clipperLineStart == clipperLineEnd) {
        return intersectCollapsedLine(line, polygon);
    }

    const double directionLengthSquared = (directionX * directionX) + (directionY * directionY);
    if (!std::isfinite(directionLengthSquared) || (directionLengthSquared <= 0.0)) {
        return {};
    }

    Clipper2Lib::Clipper64 clipper;
    clipper.AddOpenSubject({{clipperLineStart, clipperLineEnd}});
    clipper.AddClip({clipPath});

    Clipper2Lib::Paths64 closedPaths;
    Clipper2Lib::Paths64 clippedPaths;
    if (!clipper.Execute(Clipper2Lib::ClipType::Intersection, Clipper2Lib::FillRule::EvenOdd, closedPaths,
                         clippedPaths)) {
        return {};
    }

    const auto projection = [directionX, directionY, directionLengthSquared,
                             &normalizedLineStart](const QPointF& point) {
        return (((point.x() - normalizedLineStart.x()) * directionX) +
                ((point.y() - normalizedLineStart.y()) * directionY)) /
               directionLengthSquared;
    };

    const double collinearityTolerance =
        64.0 * std::numeric_limits<double>::epsilon() * std::sqrt(directionLengthSquared);
    QList<LineInterval> intervals;

    for (const Clipper2Lib::Path64& path : clippedPaths) {
        if (path.size() < 2) {
            continue;
        }

        double firstProjection = std::numeric_limits<double>::infinity();
        double lastProjection = -std::numeric_limits<double>::infinity();
        for (const Clipper2Lib::Point64& point : path) {
            const double pointProjection = projection(fromClipperPoint(point));
            firstProjection = (std::min) (firstProjection, pointProjection);
            lastProjection = (std::max) (lastProjection, pointProjection);
        }
        appendInterval(intervals, firstProjection, lastProjection);
    }

    // Open-path clipping intentionally omits some paths that coincide with a clip boundary.
    // Preserve these valid line segments explicitly, then merge them with the interior intervals.
    for (size_t index = 0; index < clipPath.size(); ++index) {
        const QPointF edgeStart = fromClipperPoint(clipPath[index]);
        const QPointF edgeEnd = fromClipperPoint(clipPath[(index + 1) % clipPath.size()]);
        const double edgeStartCross = (directionX * (edgeStart.y() - normalizedLineStart.y())) -
                                      (directionY * (edgeStart.x() - normalizedLineStart.x()));
        const double edgeEndCross = (directionX * (edgeEnd.y() - normalizedLineStart.y())) -
                                    (directionY * (edgeEnd.x() - normalizedLineStart.x()));
        if ((std::abs(edgeStartCross) <= collinearityTolerance) && (std::abs(edgeEndCross) <= collinearityTolerance)) {
            const double edgeStartProjection = projection(edgeStart);
            const double edgeEndProjection = projection(edgeEnd);
            appendInterval(intervals, (std::min) (edgeStartProjection, edgeEndProjection),
                           (std::max) (edgeStartProjection, edgeEndProjection));
        }
    }

    return linesFromIntervals(line, intervals);
}

}  // namespace

QList<QLineF> QGCPolygonClipper::intersectLine(const QLineF& line, const QPolygonF& polygon)
{
    const QList<QList<QLineF>> results = intersectLines({line}, polygon);
    return results.isEmpty() ? QList<QLineF>{} : results.first();
}

QList<QList<QLineF>> QGCPolygonClipper::intersectLines(const QList<QLineF>& lines, const QPolygonF& polygon)
{
    QList<QList<QLineF>> results(lines.size());
    if (lines.isEmpty()) {
        return results;
    }

    CoordinateNormalizer normalizer;
    if (!createNormalizer(polygon, normalizer)) {
        return results;
    }

    const QPolygonF clipPolygon = normalizedPath(polygon, normalizer);
    if (clipPolygon.size() < 3) {
        return results;
    }
    const Clipper2Lib::Path64 clipPath = toClipperPath(clipPolygon);
    const QRectF polygonBounds = polygon.boundingRect();

    for (qsizetype index = 0; index < lines.size(); ++index) {
        QLineF boundedLine;
        if (clipLineToBounds(lines[index], polygonBounds, boundedLine)) {
            results[index] = intersectNormalizedLine(boundedLine, clipPath, normalizer, polygon);
        }
    }
    return results;
}

QList<QPolygonF> QGCPolygonClipper::offsetPolygon(const QPolygonF& polygon, double distance)
{
    if ((polygon.size() < 3) || !std::isfinite(distance)) {
        return {};
    }

    CoordinateNormalizer normalizer;
    if (!createNormalizer(polygon, normalizer)) {
        return {};
    }

    const QPolygonF normalizedPolygon = normalizedPath(polygon, normalizer);
    if (normalizedPolygon.size() < 3) {
        return {};
    }
    if (qFuzzyIsNull(distance)) {
        return {polygon};
    }

    const double scaledDistance = (distance / normalizer.extent) * coordinateScale;
    if (!std::isfinite(scaledDistance) || (std::abs(scaledDistance) > maximumScaledOffset)) {
        return {};
    }

    const Clipper2Lib::Paths64 resultPaths =
        Clipper2Lib::InflatePaths({toClipperPath(normalizedPolygon)}, scaledDistance, Clipper2Lib::JoinType::Miter,
                                  Clipper2Lib::EndType::Polygon, miterLimit);
    QList<QPolygonF> result = fromClipperPaths(resultPaths, normalizer);
    for (QPolygonF& resultPolygon : result) {
        alignPolygonStart(resultPolygon, polygon.first());
    }
    return result;
}

QList<QPointF> QGCPolygonClipper::offsetPolyline(const QList<QPointF>& polyline, double distance)
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

    bool explicitlyClosed = false;
    if ((normalizedPolyline.size() > 2) && (distanceSquared(normalizedPolyline.first(), normalizedPolyline.last()) <=
                                            (normalizedPointTolerance * normalizedPointTolerance))) {
        explicitlyClosed = true;
        normalizedPolyline.removeLast();
    }
    if ((explicitlyClosed && (normalizedPolyline.size() < 3)) ||
        (!explicitlyClosed && (normalizedPolyline.size() < 2))) {
        return {};
    }

    const qsizetype segmentCount = explicitlyClosed ? normalizedPolyline.size() : normalizedPolyline.size() - 1;
    QList<OffsetSegment> segments;
    segments.reserve(segmentCount);
    for (qsizetype index = 0; index < segmentCount; ++index) {
        const QPointF delta = normalizedPolyline[(index + 1) % normalizedPolyline.size()] - normalizedPolyline[index];
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
    if (explicitlyClosed) {
        for (qsizetype index = 0; index < normalizedPolyline.size(); ++index) {
            const qsizetype previousSegmentIndex = (index + segmentCount - 1) % segmentCount;
            appendOffsetJoin(normalizedResult, normalizedPolyline[index], segments[previousSegmentIndex],
                             segments[index], normalizedDistance);
        }
        if (!normalizedResult.isEmpty()) {
            normalizedResult.append(normalizedResult.first());
        }
    } else {
        appendUnique(normalizedResult, normalizedPolyline.first() + (segments.first().leftNormal * normalizedDistance));
        for (qsizetype index = 1; index < normalizedPolyline.size() - 1; ++index) {
            appendOffsetJoin(normalizedResult, normalizedPolyline[index], segments[index - 1], segments[index],
                             normalizedDistance);
        }
        appendUnique(normalizedResult, normalizedPolyline.last() + (segments.last().leftNormal * normalizedDistance));
    }

    QList<QPointF> result;
    result.reserve(normalizedResult.size());
    for (const QPointF& point : normalizedResult) {
        result.append(normalizer.denormalize(point));
    }
    return result;
}

QPolygonF QGCPolygonClipper::bufferPolyline(const QList<QPointF>& polyline, double distance)
{
    if ((polyline.size() < 2) || !std::isfinite(distance) || (distance <= 0.0)) {
        return {};
    }

    CoordinateNormalizer normalizer;
    if (!createNormalizer(polyline, normalizer)) {
        return {};
    }

    const double scaledDistance = (distance / normalizer.extent) * coordinateScale;
    if (!std::isfinite(scaledDistance) || (scaledDistance > maximumScaledOffset)) {
        return {};
    }

    Clipper2Lib::Path64 clipperPath = toClipperPath(polyline, normalizer);
    Clipper2Lib::EndType endType = Clipper2Lib::EndType::Butt;
    if ((clipperPath.size() > 2) && (clipperPath.front().x == clipperPath.back().x) &&
        (clipperPath.front().y == clipperPath.back().y)) {
        clipperPath.pop_back();
        endType = Clipper2Lib::EndType::Joined;
    }

    const Clipper2Lib::Paths64 resultPaths =
        Clipper2Lib::InflatePaths({clipperPath}, scaledDistance, Clipper2Lib::JoinType::Miter, endType, miterLimit);
    return fromClipperCompoundPath(resultPaths, normalizer);
}
