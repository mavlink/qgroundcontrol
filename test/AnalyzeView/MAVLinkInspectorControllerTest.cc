#include "MAVLinkInspectorControllerTest.h"

#include "MAVLinkInspectorController.h"
#include "QmlObjectListModel.h"

void MAVLinkInspectorControllerTest::_constructionTest()
{
    MAVLinkInspectorController controller;

    QVERIFY(controller.systems() != nullptr);
    QVERIFY(controller.activeSystem() == nullptr);
}

void MAVLinkInspectorControllerTest::_timeScalesNonEmptyTest()
{
    MAVLinkInspectorController controller;

    QVERIFY(!controller.timeScales().isEmpty());
}

void MAVLinkInspectorControllerTest::_rangeListNonEmptyTest()
{
    MAVLinkInspectorController controller;

    QVERIFY(!controller.rangeList().isEmpty());
}

void MAVLinkInspectorControllerTest::_systemNamesInitiallyEmptyTest()
{
    MAVLinkInspectorController controller;

    // No vehicles connected in unit test context
    QVERIFY(controller.systemNames().isEmpty());
}

void MAVLinkInspectorControllerTest::_activeSystemInitiallyNullTest()
{
    MAVLinkInspectorController controller;

    QVERIFY(controller.activeSystem() == nullptr);
}

void MAVLinkInspectorControllerTest::_timeScalesCountTest()
{
    MAVLinkInspectorController controller;

    // 6 time scales: 5s, 10s, 30s, 60s, 2min, 5min
    QCOMPARE(controller.timeScales().count(), 6);
    QCOMPARE(controller.timeScaleSt().count(), 6);
}

void MAVLinkInspectorControllerTest::_rangeListCountTest()
{
    MAVLinkInspectorController controller;

    // 10 range entries: Auto + 9 fixed ranges
    QCOMPARE(controller.rangeList().count(), 10);
    QCOMPARE(controller.rangeSt().count(), 10);
}

UT_REGISTER_TEST(MAVLinkInspectorControllerTest, TestLabel::Unit, TestLabel::AnalyzeView)
