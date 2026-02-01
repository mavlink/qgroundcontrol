#include "SectionTest.h"

#include "MultiSignalSpy.h"
#include "Section.h"
#include "SimpleMissionItem.h"
#include "SurveyComplexItem.h"

void SectionTest::init()
{
    VisualMissionItemTest::init();

    MissionItem missionItem(1,           // sequence number
                            MAV_CMD_NAV_WAYPOINT, MAV_FRAME_GLOBAL_RELATIVE_ALT,
                            10.1234567,  // param 1-7
                            20.1234567, 30.1234567, 40.1234567, 50.1234567, 60.1234567, 70.1234567,
                            true,        // autoContinue
                            false);      // isCurrentItem
    _simpleItem = new SimpleMissionItem(planController(), false /* flyView */, missionItem);
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
    QCOMPARE(spy->init(section), true);
    *sectionSpy = spy;
}

void SectionTest::_commonScanTest(Section* section)
{
    QCOMPARE(section->available(), true);

    QmlObjectListModel emptyVisualItems;

    QmlObjectListModel waypointVisualItems;
    MissionItem waypointItem(0, MAV_CMD_NAV_WAYPOINT, MAV_FRAME_GLOBAL_RELATIVE_ALT, 0, 0, 0, 0, 0, 0, 0, true, false);
    SimpleMissionItem simpleItem(planController(), false /* flyView */, waypointItem);
    waypointVisualItems.append(&simpleItem);
    waypointVisualItems.append(&simpleItem);
    waypointVisualItems.append(&simpleItem);

    QmlObjectListModel complexVisualItems;
    SurveyComplexItem surveyItem(planController(), false /* fly View */, QString() /* kmlFile */);
    complexVisualItems.append(&surveyItem);

    // This tests the common cases which should not lead to scan succeess

    int scanIndex = 0;
    QCOMPARE(section->scanForSection(&emptyVisualItems, scanIndex), false);
    QCOMPARE(scanIndex, 0);

    scanIndex = 0;
    QCOMPARE(section->scanForSection(&waypointVisualItems, scanIndex), false);
    QCOMPARE(scanIndex, 0);

    scanIndex = 0;
    QCOMPARE(section->scanForSection(&complexVisualItems, scanIndex), false);
    QCOMPARE(scanIndex, 0);
}
