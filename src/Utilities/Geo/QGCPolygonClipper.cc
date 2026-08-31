#include "QGCPolygonClipper.h"

#include <QtPositioning/private/qclipperutils_p.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace {

struct CoordinateNormalizer
{
    QPointF center;
    double extent = 0.0;

    [[nodiscard]] bool isValid() const { return std::isfinite(extent) && (extent > 0.0); }

    [[nodiscard]] QDoubleVector2D normalize(const QPointF& point) const
    {
        return QDoubleVector2D((point.x() - center.x()) / extent, (point.y() - center.y()) / extent);
    }
};

bool updateBounds(const QPointF& point, double& minimumX, double& minimumY, double& maximumX, double& maximumY)
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

bool createNormalizer(const QList<QLineF>& lines, const QPolygonF& polygon, CoordinateNormalizer& normalizer)
{
    double minimumX = std::numeric_limits<double>::infinity();
    double minimumY = std::numeric_limits<double>::infinity();
    double maximumX = -std::numeric_limits<double>::infinity();
    double maximumY = -std::numeric_limits<double>::infinity();

    for (const QLineF& line : lines) {
        if (!updateBounds(line.p1(), minimumX, minimumY, maximumX, maximumY) ||
            !updateBounds(line.p2(), minimumX, minimumY, maximumX, maximumY)) {
            return false;
        }
    }
    for (const QPointF& point : polygon) {
        if (!updateBounds(point, minimumX, minimumY, maximumX, maximumY)) {
            return false;
        }
    }

    if (!std::isfinite(minimumX)) {
        return false;
    }

    normalizer.center = QPointF(std::midpoint(minimumX, maximumX), std::midpoint(minimumY, maximumY));
    normalizer.extent = (std::max) (maximumX - minimumX, maximumY - minimumY);
    return normalizer.isValid();
}

struct LineInterval
{
    double start = 0.0;
    double end = 0.0;
};

QList<QDoubleVector2D> normalizedPath(const QPolygonF& polygon, const CoordinateNormalizer& normalizer)
{
    qsizetype pointCount = polygon.size();
    if ((pointCount > 1) && (polygon.first() == polygon.last())) {
        --pointCount;
    }

    QList<QDoubleVector2D> result;
    result.reserve(pointCount);
    for (qsizetype index = 0; index < pointCount; ++index) {
        result.append(normalizer.normalize(polygon[index]));
    }
    return result;
}

QList<QLineF> intersectNormalizedLine(const QLineF& line, const QList<QDoubleVector2D>& clipPath,
                                      const CoordinateNormalizer& normalizer)
{
    if (line.isNull()) {
        return {};
    }

    QClipperUtils clipper;
    const QDoubleVector2D normalizedLineStart = normalizer.normalize(line.p1());
    const QDoubleVector2D normalizedLineEnd = normalizer.normalize(line.p2());
    clipper.addSubjectPath({normalizedLineStart, normalizedLineEnd}, false);
    clipper.addClipPolygon(clipPath);

    const double directionX = normalizedLineEnd.x() - normalizedLineStart.x();
    const double directionY = normalizedLineEnd.y() - normalizedLineStart.y();
    const double directionLengthSquared = (directionX * directionX) + (directionY * directionY);
    if (!std::isfinite(directionLengthSquared) || (directionLengthSquared <= 0.0)) {
        return {};
    }

    const auto projection = [directionX, directionY, directionLengthSquared,
                             &normalizedLineStart](const QDoubleVector2D& point) {
        return (((point.x() - normalizedLineStart.x()) * directionX) +
                ((point.y() - normalizedLineStart.y()) * directionY)) /
               directionLengthSquared;
    };

    constexpr double intervalTolerance = 1e-12;
    const double collinearityTolerance =
        64.0 * std::numeric_limits<double>::epsilon() * std::sqrt(directionLengthSquared);
    QList<LineInterval> intervals;
    const auto appendInterval = [&intervals](double start, double end) {
        start = std::clamp(start, 0.0, 1.0);
        end = std::clamp(end, 0.0, 1.0);
        if ((end - start) > intervalTolerance) {
            intervals.append({start, end});
        }
    };

    const QList<QList<QDoubleVector2D>> clippedPaths =
        clipper.execute(QClipperUtils::Intersection, QClipperUtils::pftEvenOdd, QClipperUtils::pftEvenOdd);
    for (const QList<QDoubleVector2D>& path : clippedPaths) {
        if (path.size() < 2) {
            continue;
        }

        double firstProjection = std::numeric_limits<double>::infinity();
        double lastProjection = -std::numeric_limits<double>::infinity();
        for (const QDoubleVector2D& point : path) {
            const double pointProjection = projection(point);
            firstProjection = (std::min) (firstProjection, pointProjection);
            lastProjection = (std::max) (lastProjection, pointProjection);
        }
        appendInterval(firstProjection, lastProjection);
    }

    // Clipper excludes open paths coincident with a clip boundary. Preserve those valid line
    // segments explicitly, then merge them with the interior intervals below.
    for (qsizetype index = 0; index < clipPath.size(); ++index) {
        const QDoubleVector2D& edgeStart = clipPath[index];
        const QDoubleVector2D& edgeEnd = clipPath[(index + 1) % clipPath.size()];
        const double edgeStartCross = (directionX * (edgeStart.y() - normalizedLineStart.y())) -
                                      (directionY * (edgeStart.x() - normalizedLineStart.x()));
        const double edgeEndCross = (directionX * (edgeEnd.y() - normalizedLineStart.y())) -
                                    (directionY * (edgeEnd.x() - normalizedLineStart.x()));
        if ((std::abs(edgeStartCross) <= collinearityTolerance) && (std::abs(edgeEndCross) <= collinearityTolerance)) {
            const double edgeStartProjection = projection(edgeStart);
            const double edgeEndProjection = projection(edgeEnd);
            appendInterval((std::min) (edgeStartProjection, edgeEndProjection),
                           (std::max) (edgeStartProjection, edgeEndProjection));
        }
    }

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
    if (!createNormalizer(lines, polygon, normalizer)) {
        return results;
    }

    const QList<QDoubleVector2D> clipPath = normalizedPath(polygon, normalizer);
    if (clipPath.size() < 3) {
        return results;
    }

    for (qsizetype index = 0; index < lines.size(); ++index) {
        results[index] = intersectNormalizedLine(lines[index], clipPath, normalizer);
    }
    return results;
}
