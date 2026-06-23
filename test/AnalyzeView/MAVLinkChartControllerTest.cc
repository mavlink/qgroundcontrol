#include "MAVLinkChartControllerTest.h"
#include <QtTest/QSignalSpy>


#include "MAVLinkChartController.h"
#include "MAVLinkInspectorController.h"

void MAVLinkChartControllerTest::_constructionTest()
{
    MAVLinkChartController chart;

    QVERIFY(chart.inspectorController() == nullptr);
    QCOMPARE(chart.chartIndex(), 0);
}

void MAVLinkChartControllerTest::_defaultRangeValuesTest()
{
    MAVLinkChartController chart;

    QCOMPARE(chart.rangeXIndex(), static_cast<quint32>(0));
    QCOMPARE(chart.rangeYIndex(), static_cast<quint32>(0));
    QCOMPARE_FUZZY(chart.rangeYMin(), 0.0, 1e-9);
    QCOMPARE_FUZZY(chart.rangeYMax(), 1.0, 1e-9);
}

void MAVLinkChartControllerTest::_chartFieldsInitiallyEmptyTest()
{
    MAVLinkChartController chart;

    QVERIFY(chart.chartFields().isEmpty());
}

void MAVLinkChartControllerTest::_setRangeXIndexTest()
{
    MAVLinkInspectorController controller;
    MAVLinkChartController chart;
    chart.setInspectorController(&controller);

    QSignalSpy xIndexSpy(&chart, &MAVLinkChartController::rangeXIndexChanged);

    // Move from default index 0 to index 1
    chart.setRangeXIndex(1);
    QCOMPARE(chart.rangeXIndex(), static_cast<quint32>(1));
    QCOMPARE(xIndexSpy.count(), 1);
}

void MAVLinkChartControllerTest::_setRangeXIndexSameValueNoSignalTest()
{
    MAVLinkInspectorController controller;
    MAVLinkChartController chart;
    chart.setInspectorController(&controller);

    QSignalSpy xIndexSpy(&chart, &MAVLinkChartController::rangeXIndexChanged);

    // Setting the same index (already 0) must not emit
    chart.setRangeXIndex(0);
    QCOMPARE(xIndexSpy.count(), 0);
}

void MAVLinkChartControllerTest::_setRangeYIndexTest()
{
    MAVLinkInspectorController controller;
    MAVLinkChartController chart;
    chart.setInspectorController(&controller);

    QSignalSpy yIndexSpy(&chart, &MAVLinkChartController::rangeYIndexChanged);

    // Index 1 corresponds to the first non-Auto range (10,000)
    chart.setRangeYIndex(1);
    QCOMPARE(chart.rangeYIndex(), static_cast<quint32>(1));
    QCOMPARE(yIndexSpy.count(), 1);

    // Non-Auto range: rangeYMin and rangeYMax must be set symmetrically
    const qreal expectedRange = controller.rangeSt()[1]->range;
    QCOMPARE_FUZZY(chart.rangeYMin(), -expectedRange, 1e-9);
    QCOMPARE_FUZZY(chart.rangeYMax(),  expectedRange, 1e-9);
}

void MAVLinkChartControllerTest::_setRangeYIndexSameValueNoSignalTest()
{
    MAVLinkInspectorController controller;
    MAVLinkChartController chart;
    chart.setInspectorController(&controller);

    QSignalSpy yIndexSpy(&chart, &MAVLinkChartController::rangeYIndexChanged);

    // Default is already 0; setting to 0 again must not emit
    chart.setRangeYIndex(0);
    QCOMPARE(yIndexSpy.count(), 0);
}

void MAVLinkChartControllerTest::_setInspectorControllerTest()
{
    MAVLinkInspectorController controller;
    MAVLinkChartController chart;

    chart.setInspectorController(&controller);
    QCOMPARE(chart.inspectorController(), &controller);

    // Setting the same controller again must be a no-op (no crash, same pointer)
    chart.setInspectorController(&controller);
    QCOMPARE(chart.inspectorController(), &controller);
}

UT_REGISTER_TEST(MAVLinkChartControllerTest, TestLabel::Unit, TestLabel::AnalyzeView)
