#include "PlanarGeometryTest.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

#include "Benchmarking.h"
#include "QGCPlanarGeometry.h"

namespace {

bool fuzzyPointCompare(const QPointF& actual, const QPointF& expected, double tolerance = 1e-9)
{
    return (QLineF(actual, expected).length() <= tolerance);
}

QPolygonF square(double left, double top, double right, double bottom)
{
    return {{left, top}, {right, top}, {right, bottom}, {left, bottom}, {left, top}};
}

double signedArea(const QPolygonF& polygon)
{
    double area = 0.0;
    for (qsizetype index = 0; index < polygon.size(); ++index) {
        const QPointF& current = polygon[index];
        const QPointF& next = polygon[(index + 1) % polygon.size()];
        area += (current.x() * next.y()) - (next.x() * current.y());
    }
    return area / 2.0;
}

}  // namespace

void PlanarGeometryTest::_lineIntersection_test()
{
    const QPolygonF polygon = square(0.0, 0.0, 10.0, 10.0);

    const QList<QLineF> clipped = QGCPlanarGeometry::intersectLine(QLineF(-5.0, 5.0, 15.0, 5.0), polygon);
    QCOMPARE(clipped.size(), 1);
    QVERIFY(fuzzyPointCompare(clipped.first().p1(), QPointF(0.0, 5.0)));
    QVERIFY(fuzzyPointCompare(clipped.first().p2(), QPointF(10.0, 5.0)));

    const QList<QLineF> reversed = QGCPlanarGeometry::intersectLine(QLineF(15.0, 5.0, -5.0, 5.0), polygon);
    QCOMPARE(reversed.size(), 1);
    QVERIFY(fuzzyPointCompare(reversed.first().p1(), QPointF(10.0, 5.0)));
    QVERIFY(fuzzyPointCompare(reversed.first().p2(), QPointF(0.0, 5.0)));

    const QList<QLineF> contained = QGCPlanarGeometry::intersectLine(QLineF(2.0, 5.0, 8.0, 5.0), polygon);
    QCOMPARE(contained.size(), 1);
    QVERIFY(fuzzyPointCompare(contained.first().p1(), QPointF(2.0, 5.0)));
    QVERIFY(fuzzyPointCompare(contained.first().p2(), QPointF(8.0, 5.0)));

    const QList<QLineF> boundary = QGCPlanarGeometry::intersectLine(QLineF(-5.0, 0.0, 15.0, 0.0), polygon);
    QCOMPARE(boundary.size(), 1);
    QVERIFY(fuzzyPointCompare(boundary.first().p1(), QPointF(0.0, 0.0)));
    QVERIFY(fuzzyPointCompare(boundary.first().p2(), QPointF(10.0, 0.0)));

    QVERIFY(QGCPlanarGeometry::intersectLine(QLineF(-5.0, 15.0, 15.0, 15.0), polygon).isEmpty());

    const QPolygonF largePolygon = square(0.0, 0.0, 1e9, 1e9);
    const QLineF shortContainedLine(1.0, 5e8, 1.000001, 5e8);
    const QList<QLineF> shortContained = QGCPlanarGeometry::intersectLine(shortContainedLine, largePolygon);
    QCOMPARE(shortContained, QList<QLineF>{shortContainedLine});
    QVERIFY(QGCPlanarGeometry::intersectLine(QLineF(1.0, 2e9, 1.000001, 2e9), largePolygon).isEmpty());

    const QPolygonF largeConcavePolygon = {
        {0.0, 0.0}, {1e9, 0.0}, {1e9, 1e9}, {4e8 + 1.0, 1e9}, {4e8 + 1.0, 4e8},
        {4e8, 4e8}, {4e8, 1e9}, {0.0, 1e9}, {0.0, 0.0},
    };
    const QList<QLineF> shortBoundaryCrossing =
        QGCPlanarGeometry::intersectLine(QLineF(4e8 - 1e-6, 5e8, 4e8 + 1e-6, 5e8), largeConcavePolygon);
    QCOMPARE(shortBoundaryCrossing.size(), 1);
    QVERIFY(fuzzyPointCompare(shortBoundaryCrossing.first().p1(), QPointF(4e8 - 1e-6, 5e8), 1e-9));
    QVERIFY(fuzzyPointCompare(shortBoundaryCrossing.first().p2(), QPointF(4e8, 5e8), 1e-9));
    QVERIFY(QGCPlanarGeometry::intersectLine(QLineF(4e8 + 1e-6, 5e8, 4e8 + 2e-6, 5e8), largeConcavePolygon).isEmpty());
}

void PlanarGeometryTest::_batchLineIntersection_test()
{
    const QPolygonF polygon = square(0.0, 0.0, 10.0, 10.0);
    const QList<QLineF> lines = {
        QLineF(-5.0, 2.0, 15.0, 2.0),
        QLineF(-5.0, 15.0, 15.0, 15.0),
        QLineF(15.0, 8.0, -5.0, 8.0),
    };

    const QList<QList<QLineF>> clippedLines = QGCPlanarGeometry::intersectLines(lines, polygon);
    QCOMPARE(clippedLines.size(), lines.size());
    QCOMPARE(clippedLines[0].size(), 1);
    QVERIFY(fuzzyPointCompare(clippedLines[0].first().p1(), QPointF(0.0, 2.0)));
    QVERIFY(fuzzyPointCompare(clippedLines[0].first().p2(), QPointF(10.0, 2.0)));
    QVERIFY(clippedLines[1].isEmpty());
    QCOMPARE(clippedLines[2].size(), 1);
    QVERIFY(fuzzyPointCompare(clippedLines[2].first().p1(), QPointF(10.0, 8.0)));
    QVERIFY(fuzzyPointCompare(clippedLines[2].first().p2(), QPointF(0.0, 8.0)));

    const QList<QList<QLineF>> clippedWithDistantLine =
        QGCPlanarGeometry::intersectLines({QLineF(2.0, 5.0, 8.0, 5.0), QLineF(1e16, 1e16, 1e16 + 10.0, 1e16)}, polygon);
    QCOMPARE(clippedWithDistantLine.size(), 2);
    QCOMPARE(clippedWithDistantLine.first(), QList<QLineF>{QLineF(2.0, 5.0, 8.0, 5.0)});
    QVERIFY(clippedWithDistantLine.last().isEmpty());
}

void PlanarGeometryTest::_concaveLineIntersection_test()
{
    const QPolygonF polygon = {
        {0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}, {7.0, 10.0}, {7.0, 3.0},
        {3.0, 3.0}, {3.0, 10.0}, {0.0, 10.0},  {0.0, 0.0},
    };

    const QList<QLineF> clipped = QGCPlanarGeometry::intersectLine(QLineF(-5.0, 5.0, 15.0, 5.0), polygon);
    QCOMPARE(clipped.size(), 2);
    QVERIFY(fuzzyPointCompare(clipped[0].p1(), QPointF(0.0, 5.0)));
    QVERIFY(fuzzyPointCompare(clipped[0].p2(), QPointF(3.0, 5.0)));
    QVERIFY(fuzzyPointCompare(clipped[1].p1(), QPointF(7.0, 5.0)));
    QVERIFY(fuzzyPointCompare(clipped[1].p2(), QPointF(10.0, 5.0)));
}

void PlanarGeometryTest::_vertexLineIntersection_test()
{
    const QPolygonF diamond = {{5.0, 0.0}, {10.0, 5.0}, {5.0, 10.0}, {0.0, 5.0}};

    const QList<QLineF> throughVertices = QGCPlanarGeometry::intersectLine(QLineF(-5.0, 5.0, 15.0, 5.0), diamond);
    QCOMPARE(throughVertices.size(), 1);
    QVERIFY(fuzzyPointCompare(throughVertices.first().p1(), QPointF(0.0, 5.0)));
    QVERIFY(fuzzyPointCompare(throughVertices.first().p2(), QPointF(10.0, 5.0)));

    QVERIFY(QGCPlanarGeometry::intersectLine(QLineF(-5.0, 0.0, 15.0, 0.0), diamond).isEmpty());

    // The sweep passes exactly through the reflex vertex at (5, 5)
    const QPolygonF arrow = {{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}, {5.0, 5.0}, {0.0, 10.0}};
    const QList<QLineF> throughReflexVertex = QGCPlanarGeometry::intersectLine(QLineF(-5.0, 5.0, 15.0, 5.0), arrow);
    QCOMPARE(throughReflexVertex.size(), 1);
    QVERIFY(fuzzyPointCompare(throughReflexVertex.first().p1(), QPointF(0.0, 5.0)));
    QVERIFY(fuzzyPointCompare(throughReflexVertex.first().p2(), QPointF(10.0, 5.0)));

    const QList<QLineF> aboveReflexVertex = QGCPlanarGeometry::intersectLine(QLineF(-5.0, 7.5, 15.0, 7.5), arrow);
    QCOMPARE(aboveReflexVertex.size(), 2);
    QVERIFY(fuzzyPointCompare(aboveReflexVertex[0].p2(), QPointF(2.5, 7.5)));
    QVERIFY(fuzzyPointCompare(aboveReflexVertex[1].p1(), QPointF(7.5, 7.5)));

    QVERIFY(QGCPlanarGeometry::intersectLine(QLineF(0.0, 0.0, 0.0, 0.0), diamond).isEmpty());
    QVERIFY(QGCPlanarGeometry::intersectLine(QLineF(0.0, std::numeric_limits<double>::quiet_NaN(), 10.0, 5.0), diamond)
                .isEmpty());
}

void PlanarGeometryTest::_largeCoordinateLineIntersection_test()
{
    constexpr double offset = 1e9;
    const QPolygonF polygon = square(offset, offset, offset + 10.0, offset + 10.0);
    const QList<QLineF> clipped =
        QGCPlanarGeometry::intersectLine(QLineF(offset - 5.0, offset + 5.0, offset + 15.0, offset + 5.0), polygon);

    QCOMPARE(clipped.size(), 1);
    QVERIFY(fuzzyPointCompare(clipped.first().p1(), QPointF(offset, offset + 5.0), 1e-6));
    QVERIFY(fuzzyPointCompare(clipped.first().p2(), QPointF(offset + 10.0, offset + 5.0), 1e-6));
}

void PlanarGeometryTest::_polygonOffset_test()
{
    const QList<QPolygonF> expanded = QGCPlanarGeometry::offsetPolygon(square(0.0, 0.0, 10.0, 10.0), 2.0);
    QCOMPARE(expanded.size(), 1);
    const QRectF expandedBounds = expanded.first().boundingRect();
    QVERIFY(fuzzyPointCompare(expandedBounds.topLeft(), QPointF(-2.0, -2.0), 1e-9));
    QVERIFY(fuzzyPointCompare(expandedBounds.bottomRight(), QPointF(12.0, 12.0), 1e-9));

    QPolygonF reversedPolygon = square(0.0, 0.0, 10.0, 10.0);
    std::reverse(reversedPolygon.begin(), reversedPolygon.end());
    const QList<QPolygonF> reversedExpanded = QGCPlanarGeometry::offsetPolygon(reversedPolygon, 2.0);
    QCOMPARE(reversedExpanded.size(), 1);
    QCOMPARE(reversedExpanded.first().boundingRect(), expandedBounds);

    const QList<QPolygonF> contracted = QGCPlanarGeometry::offsetPolygon(square(0.0, 0.0, 10.0, 10.0), -2.0);
    QCOMPARE(contracted.size(), 1);
    const QRectF contractedBounds = contracted.first().boundingRect();
    QVERIFY(fuzzyPointCompare(contractedBounds.topLeft(), QPointF(2.0, 2.0), 1e-9));
    QVERIFY(fuzzyPointCompare(contractedBounds.bottomRight(), QPointF(8.0, 8.0), 1e-9));

    QVERIFY(QGCPlanarGeometry::offsetPolygon(square(0.0, 0.0, 10.0, 10.0), -6.0).isEmpty());
}

void PlanarGeometryTest::_polylineOffset_test()
{
    const QList<QPointF> polyline = {QPointF(0.0, 0.0), QPointF(10.0, 0.0), QPointF(10.0, 10.0)};

    const QList<QPointF> leftOffset = QGCPlanarGeometry::offsetPolyline(polyline, 2.0);
    QCOMPARE(leftOffset.size(), 3);
    QVERIFY(fuzzyPointCompare(leftOffset[0], QPointF(0.0, 2.0)));
    QVERIFY(fuzzyPointCompare(leftOffset[1], QPointF(8.0, 2.0)));
    QVERIFY(fuzzyPointCompare(leftOffset[2], QPointF(8.0, 10.0)));

    const QList<QPointF> rightOffset = QGCPlanarGeometry::offsetPolyline(polyline, -2.0);
    QCOMPARE(rightOffset.size(), 3);
    QVERIFY(fuzzyPointCompare(rightOffset[0], QPointF(0.0, -2.0)));
    QVERIFY(fuzzyPointCompare(rightOffset[1], QPointF(12.0, -2.0)));
    QVERIFY(fuzzyPointCompare(rightOffset[2], QPointF(12.0, 10.0)));

    QCOMPARE(QGCPlanarGeometry::offsetPolyline(polyline, 0.0), polyline);
    QVERIFY(QGCPlanarGeometry::offsetPolyline({QPointF(0.0, 0.0)}, 2.0).isEmpty());
    QVERIFY(QGCPlanarGeometry::offsetPolyline(polyline, std::numeric_limits<double>::infinity()).isEmpty());

    const QList<QPointF> duplicatePointOffset =
        QGCPlanarGeometry::offsetPolyline({QPointF(0.0, 0.0), QPointF(0.0, 0.0), QPointF(10.0, 0.0)}, 2.0);
    QCOMPARE(duplicatePointOffset.size(), 2);
    QVERIFY(fuzzyPointCompare(duplicatePointOffset[0], QPointF(0.0, 2.0)));
    QVERIFY(fuzzyPointCompare(duplicatePointOffset[1], QPointF(10.0, 2.0)));

    constexpr double largeCoordinate = 1e9;
    const QList<QPointF> largeCoordinateOffset = QGCPlanarGeometry::offsetPolyline(
        {QPointF(largeCoordinate, largeCoordinate), QPointF(largeCoordinate + 10.0, largeCoordinate)}, 2.0);
    QCOMPARE(largeCoordinateOffset.size(), 2);
    QVERIFY(fuzzyPointCompare(largeCoordinateOffset[0], QPointF(largeCoordinate, largeCoordinate + 2.0), 1e-6));
    QVERIFY(fuzzyPointCompare(largeCoordinateOffset[1], QPointF(largeCoordinate + 10.0, largeCoordinate + 2.0), 1e-6));

    const QList<QPointF> reversal = QGCPlanarGeometry::offsetPolyline(
        {QPointF(0.0, 0.0), QPointF(10.0, 0.0), QPointF(0.0, 0.0), QPointF(-10.0, 0.0)}, 2.0);
    QCOMPARE(reversal.size(), 5);
    for (const QPointF& point : reversal) {
        QVERIFY(std::isfinite(point.x()));
        QVERIFY(std::isfinite(point.y()));
        QVERIFY(std::abs(point.x()) <= 10.0);
        QVERIFY(std::abs(point.y()) <= 2.0);
    }

    const QList<QPointF> closedOffset = QGCPlanarGeometry::offsetPolyline(square(0.0, 0.0, 10.0, 10.0), 2.0);
    QCOMPARE(closedOffset.size(), 5);
    QCOMPARE(closedOffset.first(), closedOffset.last());
    const QRectF closedOffsetBounds = QPolygonF(closedOffset).boundingRect();
    QVERIFY(fuzzyPointCompare(closedOffsetBounds.topLeft(), QPointF(2.0, 2.0)));
    QVERIFY(fuzzyPointCompare(closedOffsetBounds.bottomRight(), QPointF(8.0, 8.0)));

    // Inward offsets at or beyond the center collapse the closed path and must not produce inverted loops
    for (const double distance : {5.0, 6.0, 15.0}) {
        QVERIFY(QGCPlanarGeometry::offsetPolyline(square(0.0, 0.0, 10.0, 10.0), distance).isEmpty());
    }

    // Left of travel is outside a clockwise path, and the offset keeps the direction of travel
    QList<QPointF> clockwiseSquare = square(0.0, 0.0, 10.0, 10.0);
    std::reverse(clockwiseSquare.begin(), clockwiseSquare.end());
    const QList<QPointF> clockwiseOffset = QGCPlanarGeometry::offsetPolyline(clockwiseSquare, 2.0);
    QCOMPARE(clockwiseOffset.size(), 5);
    QVERIFY(fuzzyPointCompare(clockwiseOffset.first(), QPointF(-2.0, -2.0)));
    QVERIFY(fuzzyPointCompare(clockwiseOffset[1], QPointF(-2.0, 12.0)));

    // A path out and back along the same line encloses no area, so it is offset as an open path
    const QList<QPointF> outAndBack =
        QGCPlanarGeometry::offsetPolyline({QPointF(0.0, 0.0), QPointF(10.0, 0.0), QPointF(0.0, 0.0)}, 1.0);
    QCOMPARE(outAndBack.size(), 4);
    QVERIFY(fuzzyPointCompare(outAndBack.first(), QPointF(0.0, 1.0)));
    QVERIFY(fuzzyPointCompare(outAndBack.last(), QPointF(0.0, -1.0)));
    const QList<QPointF> backAndForth = QGCPlanarGeometry::offsetPolyline(
        {QPointF(0.0, 0.0), QPointF(10.0, 0.0), QPointF(5.0, 0.0), QPointF(8.0, 0.0), QPointF(0.0, 0.0)}, 1.0);
    QVERIFY(!backAndForth.isEmpty());
    QVERIFY(fuzzyPointCompare(backAndForth.first(), QPointF(0.0, 1.0)));
    QVERIFY(fuzzyPointCompare(backAndForth.last(), QPointF(0.0, -1.0)));

    // A closed path crossing itself has no consistent inside, whichever side the offset is on
    const QList<QPointF> figureEight = {QPointF(0.0, 0.0), QPointF(10.0, 10.0), QPointF(10.0, 0.0), QPointF(0.0, 10.0),
                                        QPointF(0.0, 0.0)};
    QVERIFY(QGCPlanarGeometry::offsetPolyline(figureEight, 1.0).isEmpty());
    QVERIFY(QGCPlanarGeometry::offsetPolyline(figureEight, -1.0).isEmpty());
}

void PlanarGeometryTest::_polylineBuffer_test()
{
    const QList<QPolygonF> contours =
        QGCPlanarGeometry::bufferPolyline({QPointF(0.0, 0.0), QPointF(10.0, 0.0), QPointF(10.0, 10.0)}, 2.0);
    QCOMPARE(contours.size(), 1);
    const QPolygonF& buffered = contours.first();
    QVERIFY(buffered.containsPoint(QPointF(5.0, 1.0), Qt::OddEvenFill));
    QVERIFY(buffered.containsPoint(QPointF(9.0, 5.0), Qt::OddEvenFill));
    QVERIFY(!buffered.containsPoint(QPointF(5.0, 3.0), Qt::OddEvenFill));
    QVERIFY(!buffered.containsPoint(QPointF(-1.0, 0.0), Qt::OddEvenFill));
    QVERIFY(QGCPlanarGeometry::bufferPolyline({QPointF(0.0, 0.0)}, 2.0).isEmpty());
    QVERIFY(QGCPlanarGeometry::bufferPolyline({QPointF(0.0, 0.0), QPointF(10.0, 0.0)}, 0.0).isEmpty());
}

void PlanarGeometryTest::_closedPolylineBuffer_test()
{
    const QList<QPolygonF> contours = QGCPlanarGeometry::bufferPolyline(square(0.0, 0.0, 10.0, 10.0), 2.0);

    // Outer boundary first, then the hole inside the closed path
    QCOMPARE(contours.size(), 2);
    QVERIFY(contours[0].boundingRect().contains(contours[1].boundingRect()));
    QVERIFY(fuzzyPointCompare(contours[0].boundingRect().topLeft(), QPointF(-2.0, -2.0)));
    QVERIFY(fuzzyPointCompare(contours[1].boundingRect().topLeft(), QPointF(2.0, 2.0)));

    const QPolygonF joined = QGCPlanarGeometry::joinContours(contours);
    QVERIFY(joined.containsPoint(QPointF(1.0, 5.0), Qt::OddEvenFill));
    QVERIFY(!joined.containsPoint(QPointF(5.0, 5.0), Qt::OddEvenFill));
    QVERIFY(!joined.containsPoint(QPointF(15.0, 5.0), Qt::OddEvenFill));

    QCOMPARE(QGCPlanarGeometry::joinContours({contours.first()}), contours.first());

    // A path out and back along the same line is buffered as an open path, without a hole
    QCOMPARE(QGCPlanarGeometry::bufferPolyline({QPointF(0.0, 0.0), QPointF(10.0, 0.0), QPointF(0.0, 0.0)}, 1.0).size(),
             1);
    QVERIFY(QGCPlanarGeometry::joinContours({}).isEmpty());
}

void PlanarGeometryTest::_acuteCornerOffset_test()
{
    // Mitering the ~11 degree corner at the origin would place a vertex about 10x the offset distance away
    // (mavlink/qgroundcontrol#5917). It is squared off instead, adding one vertex.
    const QPolygonF sliver = {{0.0, 0.0}, {100.0, 0.0}, {100.0, 20.0}};
    constexpr double distance = 10.0;
    const QList<QPolygonF> expanded = QGCPlanarGeometry::offsetPolygon(sliver, distance);
    QCOMPARE(expanded.size(), 1);
    QCOMPARE(expanded.first().size(), sliver.size() + 1);

    // Corners of 45 degrees or more are still mitered, which reaches at most distance / sin(22.5 degrees)
    const double maximumReach = distance * 2.62;
    for (const QPointF& vertex : expanded.first()) {
        double nearestDistance = std::numeric_limits<double>::infinity();
        for (qsizetype index = 0; index < sliver.size(); ++index) {
            const QLineF edge(sliver[index], sliver[(index + 1) % sliver.size()]);
            const double edgeParameter = std::clamp(
                QPointF::dotProduct(vertex - edge.p1(), edge.p2() - edge.p1()) / (edge.length() * edge.length()), 0.0,
                1.0);
            nearestDistance = (std::min) (nearestDistance, QLineF(vertex, edge.pointAt(edgeParameter)).length());
        }
        QVERIFY2(nearestDistance <= maximumReach,
                 qPrintable(QStringLiteral("Offset vertex is %1 from the polygon").arg(nearestDistance)));
    }
}

void PlanarGeometryTest::_largeDistanceOffset_test()
{
    // Offsets far larger than the polygon extent are typical for structure scans of small structures
    const QPolygonF clockwiseSquare = {{0.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}, {1.0, 0.0}};
    const QPolygonF counterClockwiseSquare = {{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}};
    for (const QPolygonF& polygon : {clockwiseSquare, counterClockwiseSquare}) {
        for (const double distance : {40.0, 100.0, 5000.0}) {
            const QList<QPolygonF> expanded = QGCPlanarGeometry::offsetPolygon(polygon, distance);
            QCOMPARE(expanded.size(), 1);
            QCOMPARE(expanded.first().size(), polygon.size());
            QVERIFY(fuzzyPointCompare(expanded.first().first(), QPointF(-distance, -distance), 1e-6 * distance));
            QVERIFY((signedArea(expanded.first()) > 0.0) == (signedArea(polygon) > 0.0));
            const QRectF bounds = expanded.first().boundingRect();
            QVERIFY(fuzzyPointCompare(bounds.topLeft(), QPointF(-distance, -distance), 1e-6 * distance));
            QVERIFY(fuzzyPointCompare(bounds.bottomRight(), QPointF(1.0 + distance, 1.0 + distance), 1e-6 * distance));
        }
    }

    const QList<QPolygonF> bufferedContours =
        QGCPlanarGeometry::bufferPolyline({QPointF(0.0, 0.0), QPointF(1.0, 0.0)}, 100.0);
    QCOMPARE(bufferedContours.size(), 1);
    const QPolygonF& buffered = bufferedContours.first();
    QVERIFY(buffered.containsPoint(QPointF(0.5, 99.0), Qt::OddEvenFill));
    QVERIFY(!buffered.containsPoint(QPointF(0.5, 101.0), Qt::OddEvenFill));
}

void PlanarGeometryTest::_benchmarkIntersectLines()
{
    constexpr int vertexCount = 1000;
    constexpr int lineCount = 1000;
    QPolygonF polygon;
    for (int index = 0; index < vertexCount; ++index) {
        const double angle = (2.0 * std::numbers::pi * index) / vertexCount;
        const double radius = (index % 2) ? 400.0 : 500.0;
        polygon.append(QPointF(500.0 + (radius * std::cos(angle)), 500.0 + (radius * std::sin(angle))));
    }
    QList<QLineF> lines;
    for (int index = 0; index < lineCount; ++index) {
        lines.append(QLineF(-1000.0, index, 2000.0, index));
    }

    auto bench = qgc::bench::ciConfig().warmup(1).epochs(5).minEpochIterations(1);
    bench.run("intersectLines 1000 lines x 1000 vertices", [&] {
        const QList<QList<QLineF>> clipped = QGCPlanarGeometry::intersectLines(lines, polygon);
        ankerl::nanobench::doNotOptimizeAway(clipped);
    });
}

UT_REGISTER_TEST(PlanarGeometryTest, TestLabel::Unit, TestLabel::Utilities)
