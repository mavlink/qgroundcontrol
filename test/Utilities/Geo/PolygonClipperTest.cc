#include "PolygonClipperTest.h"

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
    QVERIFY(QGCPolygonClipper::intersectLine(QLineF(1.0, 2e9, 1.000001, 2e9), largePolygon).isEmpty());
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

UT_REGISTER_TEST(PolygonClipperTest, TestLabel::Unit, TestLabel::Utilities)
