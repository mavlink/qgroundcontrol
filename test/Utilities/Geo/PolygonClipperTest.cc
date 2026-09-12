#include "PolygonClipperTest.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "QGCPolygonClipper.h"

namespace {

bool fuzzyPointCompare(const QPointF& actual, const QPointF& expected, double tolerance = 1e-9)
{
    return (QLineF(actual, expected).length() <= tolerance);
}

QPolygonF square(double left, double top, double right, double bottom)
{
    return {{left, top}, {right, top}, {right, bottom}, {left, bottom}, {left, top}};
}

}  // namespace

void PolygonClipperTest::_lineIntersection_test()
{
    const QPolygonF polygon = square(0.0, 0.0, 10.0, 10.0);

    const QList<QLineF> clipped = QGCPolygonClipper::intersectLine(QLineF(-5.0, 5.0, 15.0, 5.0), polygon);
    QCOMPARE(clipped.size(), 1);
    QVERIFY(fuzzyPointCompare(clipped.first().p1(), QPointF(0.0, 5.0)));
    QVERIFY(fuzzyPointCompare(clipped.first().p2(), QPointF(10.0, 5.0)));

    const QList<QLineF> reversed = QGCPolygonClipper::intersectLine(QLineF(15.0, 5.0, -5.0, 5.0), polygon);
    QCOMPARE(reversed.size(), 1);
    QVERIFY(fuzzyPointCompare(reversed.first().p1(), QPointF(10.0, 5.0)));
    QVERIFY(fuzzyPointCompare(reversed.first().p2(), QPointF(0.0, 5.0)));

    const QList<QLineF> contained = QGCPolygonClipper::intersectLine(QLineF(2.0, 5.0, 8.0, 5.0), polygon);
    QCOMPARE(contained.size(), 1);
    QVERIFY(fuzzyPointCompare(contained.first().p1(), QPointF(2.0, 5.0)));
    QVERIFY(fuzzyPointCompare(contained.first().p2(), QPointF(8.0, 5.0)));

    const QList<QLineF> boundary = QGCPolygonClipper::intersectLine(QLineF(-5.0, 0.0, 15.0, 0.0), polygon);
    QCOMPARE(boundary.size(), 1);
    QVERIFY(fuzzyPointCompare(boundary.first().p1(), QPointF(0.0, 0.0)));
    QVERIFY(fuzzyPointCompare(boundary.first().p2(), QPointF(10.0, 0.0)));

    QVERIFY(QGCPolygonClipper::intersectLine(QLineF(-5.0, 15.0, 15.0, 15.0), polygon).isEmpty());

    const QPolygonF largePolygon = square(0.0, 0.0, 1e9, 1e9);
    const QLineF shortContainedLine(1.0, 5e8, 1.000001, 5e8);
    const QList<QLineF> shortContained = QGCPolygonClipper::intersectLine(shortContainedLine, largePolygon);
    QCOMPARE(shortContained, QList<QLineF>{shortContainedLine});
    QVERIFY(QGCPolygonClipper::intersectLine(QLineF(1.0, 2e9, 1.000001, 2e9), largePolygon).isEmpty());

    const QPolygonF largeConcavePolygon = {
        {0.0, 0.0}, {1e9, 0.0}, {1e9, 1e9}, {4e8 + 1.0, 1e9}, {4e8 + 1.0, 4e8},
        {4e8, 4e8}, {4e8, 1e9}, {0.0, 1e9}, {0.0, 0.0},
    };
    const QList<QLineF> shortBoundaryCrossing =
        QGCPolygonClipper::intersectLine(QLineF(4e8 - 1e-6, 5e8, 4e8 + 1e-6, 5e8), largeConcavePolygon);
    QCOMPARE(shortBoundaryCrossing.size(), 1);
    QVERIFY(fuzzyPointCompare(shortBoundaryCrossing.first().p1(), QPointF(4e8 - 1e-6, 5e8), 1e-9));
    QVERIFY(fuzzyPointCompare(shortBoundaryCrossing.first().p2(), QPointF(4e8, 5e8), 1e-9));
    QVERIFY(QGCPolygonClipper::intersectLine(QLineF(4e8 + 1e-6, 5e8, 4e8 + 2e-6, 5e8), largeConcavePolygon).isEmpty());
}

void PolygonClipperTest::_batchLineIntersection_test()
{
    const QPolygonF polygon = square(0.0, 0.0, 10.0, 10.0);
    const QList<QLineF> lines = {
        QLineF(-5.0, 2.0, 15.0, 2.0),
        QLineF(-5.0, 15.0, 15.0, 15.0),
        QLineF(15.0, 8.0, -5.0, 8.0),
    };

    const QList<QList<QLineF>> clippedLines = QGCPolygonClipper::intersectLines(lines, polygon);
    QCOMPARE(clippedLines.size(), lines.size());
    QCOMPARE(clippedLines[0].size(), 1);
    QVERIFY(fuzzyPointCompare(clippedLines[0].first().p1(), QPointF(0.0, 2.0)));
    QVERIFY(fuzzyPointCompare(clippedLines[0].first().p2(), QPointF(10.0, 2.0)));
    QVERIFY(clippedLines[1].isEmpty());
    QCOMPARE(clippedLines[2].size(), 1);
    QVERIFY(fuzzyPointCompare(clippedLines[2].first().p1(), QPointF(10.0, 8.0)));
    QVERIFY(fuzzyPointCompare(clippedLines[2].first().p2(), QPointF(0.0, 8.0)));

    const QList<QList<QLineF>> clippedWithDistantLine =
        QGCPolygonClipper::intersectLines({QLineF(2.0, 5.0, 8.0, 5.0), QLineF(1e16, 1e16, 1e16 + 10.0, 1e16)}, polygon);
    QCOMPARE(clippedWithDistantLine.size(), 2);
    QCOMPARE(clippedWithDistantLine.first(), QList<QLineF>{QLineF(2.0, 5.0, 8.0, 5.0)});
    QVERIFY(clippedWithDistantLine.last().isEmpty());
}

void PolygonClipperTest::_concaveLineIntersection_test()
{
    const QPolygonF polygon = {
        {0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}, {7.0, 10.0}, {7.0, 3.0},
        {3.0, 3.0}, {3.0, 10.0}, {0.0, 10.0},  {0.0, 0.0},
    };

    const QList<QLineF> clipped = QGCPolygonClipper::intersectLine(QLineF(-5.0, 5.0, 15.0, 5.0), polygon);
    QCOMPARE(clipped.size(), 2);
    QVERIFY(fuzzyPointCompare(clipped[0].p1(), QPointF(0.0, 5.0)));
    QVERIFY(fuzzyPointCompare(clipped[0].p2(), QPointF(3.0, 5.0)));
    QVERIFY(fuzzyPointCompare(clipped[1].p1(), QPointF(7.0, 5.0)));
    QVERIFY(fuzzyPointCompare(clipped[1].p2(), QPointF(10.0, 5.0)));
}

void PolygonClipperTest::_largeCoordinateLineIntersection_test()
{
    constexpr double offset = 1e9;
    const QPolygonF polygon = square(offset, offset, offset + 10.0, offset + 10.0);
    const QList<QLineF> clipped =
        QGCPolygonClipper::intersectLine(QLineF(offset - 5.0, offset + 5.0, offset + 15.0, offset + 5.0), polygon);

    QCOMPARE(clipped.size(), 1);
    QVERIFY(fuzzyPointCompare(clipped.first().p1(), QPointF(offset, offset + 5.0), 1e-6));
    QVERIFY(fuzzyPointCompare(clipped.first().p2(), QPointF(offset + 10.0, offset + 5.0), 1e-6));
}

void PolygonClipperTest::_polygonOffset_test()
{
    const QList<QPolygonF> expanded = QGCPolygonClipper::offsetPolygon(square(0.0, 0.0, 10.0, 10.0), 2.0);
    QCOMPARE(expanded.size(), 1);
    const QRectF expandedBounds = expanded.first().boundingRect();
    QVERIFY(fuzzyPointCompare(expandedBounds.topLeft(), QPointF(-2.0, -2.0), 1e-9));
    QVERIFY(fuzzyPointCompare(expandedBounds.bottomRight(), QPointF(12.0, 12.0), 1e-9));

    QPolygonF reversedPolygon = square(0.0, 0.0, 10.0, 10.0);
    std::reverse(reversedPolygon.begin(), reversedPolygon.end());
    const QList<QPolygonF> reversedExpanded = QGCPolygonClipper::offsetPolygon(reversedPolygon, 2.0);
    QCOMPARE(reversedExpanded.size(), 1);
    QCOMPARE(reversedExpanded.first().boundingRect(), expandedBounds);

    const QList<QPolygonF> contracted = QGCPolygonClipper::offsetPolygon(square(0.0, 0.0, 10.0, 10.0), -2.0);
    QCOMPARE(contracted.size(), 1);
    const QRectF contractedBounds = contracted.first().boundingRect();
    QVERIFY(fuzzyPointCompare(contractedBounds.topLeft(), QPointF(2.0, 2.0), 1e-9));
    QVERIFY(fuzzyPointCompare(contractedBounds.bottomRight(), QPointF(8.0, 8.0), 1e-9));

    QVERIFY(QGCPolygonClipper::offsetPolygon(square(0.0, 0.0, 10.0, 10.0), -6.0).isEmpty());
}

void PolygonClipperTest::_polylineOffset_test()
{
    const QList<QPointF> polyline = {QPointF(0.0, 0.0), QPointF(10.0, 0.0), QPointF(10.0, 10.0)};

    const QList<QPointF> leftOffset = QGCPolygonClipper::offsetPolyline(polyline, 2.0);
    QCOMPARE(leftOffset.size(), 3);
    QVERIFY(fuzzyPointCompare(leftOffset[0], QPointF(0.0, 2.0)));
    QVERIFY(fuzzyPointCompare(leftOffset[1], QPointF(8.0, 2.0)));
    QVERIFY(fuzzyPointCompare(leftOffset[2], QPointF(8.0, 10.0)));

    const QList<QPointF> rightOffset = QGCPolygonClipper::offsetPolyline(polyline, -2.0);
    QCOMPARE(rightOffset.size(), 3);
    QVERIFY(fuzzyPointCompare(rightOffset[0], QPointF(0.0, -2.0)));
    QVERIFY(fuzzyPointCompare(rightOffset[1], QPointF(12.0, -2.0)));
    QVERIFY(fuzzyPointCompare(rightOffset[2], QPointF(12.0, 10.0)));

    QCOMPARE(QGCPolygonClipper::offsetPolyline(polyline, 0.0), polyline);
    QVERIFY(QGCPolygonClipper::offsetPolyline({QPointF(0.0, 0.0)}, 2.0).isEmpty());
    QVERIFY(QGCPolygonClipper::offsetPolyline(polyline, std::numeric_limits<double>::infinity()).isEmpty());

    const QList<QPointF> duplicatePointOffset =
        QGCPolygonClipper::offsetPolyline({QPointF(0.0, 0.0), QPointF(0.0, 0.0), QPointF(10.0, 0.0)}, 2.0);
    QCOMPARE(duplicatePointOffset.size(), 2);
    QVERIFY(fuzzyPointCompare(duplicatePointOffset[0], QPointF(0.0, 2.0)));
    QVERIFY(fuzzyPointCompare(duplicatePointOffset[1], QPointF(10.0, 2.0)));

    constexpr double largeCoordinate = 1e9;
    const QList<QPointF> largeCoordinateOffset = QGCPolygonClipper::offsetPolyline(
        {QPointF(largeCoordinate, largeCoordinate), QPointF(largeCoordinate + 10.0, largeCoordinate)}, 2.0);
    QCOMPARE(largeCoordinateOffset.size(), 2);
    QVERIFY(fuzzyPointCompare(largeCoordinateOffset[0], QPointF(largeCoordinate, largeCoordinate + 2.0), 1e-6));
    QVERIFY(fuzzyPointCompare(largeCoordinateOffset[1], QPointF(largeCoordinate + 10.0, largeCoordinate + 2.0), 1e-6));

    const QList<QPointF> reversal = QGCPolygonClipper::offsetPolyline(
        {QPointF(0.0, 0.0), QPointF(10.0, 0.0), QPointF(0.0, 0.0), QPointF(-10.0, 0.0)}, 2.0);
    QCOMPARE(reversal.size(), 5);
    for (const QPointF& point : reversal) {
        QVERIFY(std::isfinite(point.x()));
        QVERIFY(std::isfinite(point.y()));
        QVERIFY(std::abs(point.x()) <= 10.0);
        QVERIFY(std::abs(point.y()) <= 2.0);
    }

    const QList<QPointF> closedOffset = QGCPolygonClipper::offsetPolyline(square(0.0, 0.0, 10.0, 10.0), 2.0);
    QCOMPARE(closedOffset.size(), 5);
    QCOMPARE(closedOffset.first(), closedOffset.last());
    const QRectF closedOffsetBounds = QPolygonF(closedOffset).boundingRect();
    QVERIFY(fuzzyPointCompare(closedOffsetBounds.topLeft(), QPointF(2.0, 2.0)));
    QVERIFY(fuzzyPointCompare(closedOffsetBounds.bottomRight(), QPointF(8.0, 8.0)));
}

void PolygonClipperTest::_polylineBuffer_test()
{
    const QPolygonF buffered =
        QGCPolygonClipper::bufferPolyline({QPointF(0.0, 0.0), QPointF(10.0, 0.0), QPointF(10.0, 10.0)}, 2.0);
    QVERIFY(buffered.containsPoint(QPointF(5.0, 1.0), Qt::OddEvenFill));
    QVERIFY(buffered.containsPoint(QPointF(9.0, 5.0), Qt::OddEvenFill));
    QVERIFY(!buffered.containsPoint(QPointF(5.0, 3.0), Qt::OddEvenFill));
    QVERIFY(!buffered.containsPoint(QPointF(-1.0, 0.0), Qt::OddEvenFill));
    QVERIFY(QGCPolygonClipper::bufferPolyline({QPointF(0.0, 0.0)}, 2.0).isEmpty());
    QVERIFY(QGCPolygonClipper::bufferPolyline({QPointF(0.0, 0.0), QPointF(10.0, 0.0)}, 0.0).isEmpty());
}

void PolygonClipperTest::_closedPolylineBuffer_test()
{
    const QPolygonF buffered = QGCPolygonClipper::bufferPolyline(square(0.0, 0.0, 10.0, 10.0), 2.0);

    QVERIFY(buffered.containsPoint(QPointF(1.0, 5.0), Qt::OddEvenFill));
    QVERIFY(!buffered.containsPoint(QPointF(5.0, 5.0), Qt::OddEvenFill));
    QVERIFY(!buffered.containsPoint(QPointF(15.0, 5.0), Qt::OddEvenFill));
}

UT_REGISTER_TEST(PolygonClipperTest, TestLabel::Unit, TestLabel::Utilities)
