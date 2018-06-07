/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SectionTest.h"
#include "SurveyComplexItem.h"

SectionTest::SectionTest(void)
    : _simpleItem(NULL)
{
    
}

void SectionTest::init(void)
{
    VisualMissionItemTest::init();

    rgSectionSignals[availableChangedIndex] =           SIGNAL(availableChanged(bool));
    rgSectionSignals[settingsSpecifiedChangedIndex] =   SIGNAL(settingsSpecifiedChanged(bool));
    rgSectionSignals[dirtyChangedIndex] =               SIGNAL(dirtyChanged(bool));
    rgSectionSignals[itemCountChangedIndex] =           SIGNAL(itemCountChanged(int));

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
    _simpleItem = new SimpleMissionItem(_offlineVehicle, false /* flyView */, missionItem, this);
}

void SectionTest::cleanup(void)
{
    delete _simpleItem;
    VisualMissionItemTest::cleanup();
}

void SectionTest::_createSpy(Section* section, MultiSignalSpy** sectionSpy)
{
    *sectionSpy = NULL;
    MultiSignalSpy* spy = new MultiSignalSpy();
    QCOMPARE(spy->init(section, rgSectionSignals, cSectionSignals), true);
    *sectionSpy = spy;
}

void SectionTest::_missionItemsEqual(MissionItem& actual, MissionItem& expected)
{
    QCOMPARE(actual.command(),      expected.command());
    QCOMPARE(actual.frame(),        expected.frame());
    QCOMPARE(actual.autoContinue(), expected.autoContinue());

    QVERIFY(UnitTest::doubleNaNCompare(actual.param1(), expected.param1()));
    QVERIFY(UnitTest::doubleNaNCompare(actual.param2(), expected.param2()));
    QVERIFY(UnitTest::doubleNaNCompare(actual.param3(), expected.param3()));
    QVERIFY(UnitTest::doubleNaNCompare(actual.param4(), expected.param4()));
    QVERIFY(UnitTest::doubleNaNCompare(actual.param5(), expected.param5()));
    QVERIFY(UnitTest::doubleNaNCompare(actual.param6(), expected.param6()));
    QVERIFY(UnitTest::doubleNaNCompare(actual.param7(), expected.param7()));
}

void SectionTest::_commonScanTest(Section* section)
{
    QCOMPARE(section->available(), true);

    QmlObjectListModel emptyVisualItems;

    QmlObjectListModel waypointVisualItems;
    MissionItem waypointItem(0, MAV_CMD_NAV_WAYPOINT, MAV_FRAME_GLOBAL_RELATIVE_ALT, 0, 0, 0, 0, 0, 0, 0, true, false);
    SimpleMissionItem simpleItem(_offlineVehicle, false /* flyView */, waypointItem, this);
    waypointVisualItems.append(&simpleItem);
    waypointVisualItems.append(&simpleItem);
    waypointVisualItems.append(&simpleItem);

    QmlObjectListModel complexVisualItems;
    SurveyComplexItem surveyItem(_offlineVehicle, false /* fly View */, QString() /* kmlFile */, this /* parent */);
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
