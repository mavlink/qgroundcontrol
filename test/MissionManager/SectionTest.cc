#include "SectionTest.h"
#include "SurveyComplexItem.h"
#include "SimpleMissionItem.h"
#include "MultiSignalSpy.h"
#include "Section.h"

#include <QtTest/QTest>

void SectionTest::init()
{
    VisualMissionItemTest::init();

    MissionItem missionItem(1,              // sequence number
                            MAV_CMD_NAV_WAYPOINT,
                            MAV_FRAME_GLOBAL_RELATIVE_ALT,
                            10.1234567,     // param 1-7
                            20.1234567,
                            30.1234567,
                            40.1234567,
                            50.1234567,
                            60.1234567,
                            70.1234567,
                            true,           // autoContinue
                            false);         // isCurrentItem
    _simpleItem = new SimpleMissionItem(_masterController, false /* flyView */, missionItem);
}

void SectionTest::cleanup()
{
    _simpleItem->deleteLater();
    VisualMissionItemTest::cleanup();
}

void SectionTest::_createSpy(Section* section, MultiSignalSpy** sectionSpy)
{
    *sectionSpy = nullptr;
    MultiSignalSpy* spy = new MultiSignalSpy();
    QVERIFY(spy->init(section));
    *sectionSpy = spy;
}

void SectionTest::_commonScanTest(Section* section)
{
    QVERIFY(section->available());

    QmlObjectListModel emptyVisualItems;

    QmlObjectListModel waypointVisualItems;
    MissionItem waypointItem(0, MAV_CMD_NAV_WAYPOINT, MAV_FRAME_GLOBAL_RELATIVE_ALT, 0, 0, 0, 0, 0, 0, 0, true, false);
    SimpleMissionItem simpleItem(_masterController, false /* flyView */, waypointItem);
    waypointVisualItems.append(&simpleItem);
    waypointVisualItems.append(&simpleItem);
    waypointVisualItems.append(&simpleItem);

    QmlObjectListModel complexVisualItems;
    SurveyComplexItem surveyItem(_masterController, false /* fly View */, QString() /* kmlFile */);
    complexVisualItems.append(&surveyItem);

    // This tests the common cases which should not lead to scan succeess

    int scanIndex = 0;
    QVERIFY(!section->scanForSection(&emptyVisualItems, scanIndex));
    QCOMPARE_EQ(scanIndex, 0);

    scanIndex = 0;
    QVERIFY(!section->scanForSection(&waypointVisualItems, scanIndex));
    QCOMPARE_EQ(scanIndex, 0);

    scanIndex = 0;
    QVERIFY(!section->scanForSection(&complexVisualItems, scanIndex));
    QCOMPARE_EQ(scanIndex, 0);
}
