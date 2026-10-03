#include "MapMarkerGroupingTest.h"

#include <QtCore/QList>
#include <QtCore/QPointF>
#include <QtCore/QtNumeric>

#include "MapMarkerGrouping.h"

void MapMarkerGroupingTest::_representatives_data()
{
    QTest::addColumn<QList<QPointF>>("points");
    QTest::addColumn<double>("distance");
    QTest::addColumn<QList<int>>("expected");

    QTest::addRow("empty") << QList<QPointF>() << 10.0 << QList<int>();
    QTest::addRow("far apart") << QList<QPointF>{{0, 0}, {100, 0}} << 10.0 << QList<int>{0, 1};
    QTest::addRow("within distance") << QList<QPointF>{{0, 0}, {5, 0}} << 10.0 << QList<int>{0, 0};
    QTest::addRow("distance is inclusive") << QList<QPointF>{{0, 0}, {0, 10}} << 10.0 << QList<int>{0, 0};
    QTest::addRow("joins the nearest group")
        << QList<QPointF>{{0, 0}, {18, 0}, {9.5, 0}} << 10.0 << QList<int>{0, 1, 1};
    QTest::addRow("tie goes to the earlier group")
        << QList<QPointF>{{0, 0}, {16, 0}, {8, 0}} << 10.0 << QList<int>{0, 1, 0};
    QTest::addRow("measured from the representative")
        << QList<QPointF>{{0, 0}, {8, 0}, {16, 0}} << 10.0 << QList<int>{0, 0, 2};
    QTest::addRow("across grid cells") << QList<QPointF>{{-0.1, 9.9}, {0.1, 10.1}} << 10.0 << QList<int>{0, 0};
    QTest::addRow("non-finite point stands alone")
        << QList<QPointF>{{qQNaN(), 0}, {0, 0}, {0, 0}} << 10.0 << QList<int>{0, 1, 1};
    QTest::addRow("zero distance groups nothing") << QList<QPointF>{{0, 0}, {0, 0}} << 0.0 << QList<int>{0, 1};
}

void MapMarkerGroupingTest::_representatives()
{
    QFETCH(QList<QPointF>, points);
    QFETCH(double, distance);
    QFETCH(QList<int>, expected);

    QCOMPARE(MapMarkerGrouping::representatives(points, distance), expected);
}

UT_REGISTER_TEST_LIGHTWEIGHT(MapMarkerGroupingTest, TestLabel::Unit)
