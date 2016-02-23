/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

#include "SimpleMissionItemTest.h"
#include "SimpleMissionItem.h"

const SimpleMissionItemTest::ItemInfo_t SimpleMissionItemTest::_rgItemInfo[] = {
    { MAV_CMD_NAV_WAYPOINT,     MAV_FRAME_GLOBAL_RELATIVE_ALT },
    { MAV_CMD_NAV_LOITER_UNLIM, MAV_FRAME_GLOBAL_RELATIVE_ALT },
    { MAV_CMD_NAV_LOITER_TURNS, MAV_FRAME_GLOBAL_RELATIVE_ALT },
    { MAV_CMD_NAV_LOITER_TIME,  MAV_FRAME_GLOBAL_RELATIVE_ALT },
    { MAV_CMD_NAV_LAND,         MAV_FRAME_GLOBAL_RELATIVE_ALT },
    { MAV_CMD_NAV_TAKEOFF,      MAV_FRAME_GLOBAL_RELATIVE_ALT },
    { MAV_CMD_CONDITION_DELAY,  MAV_FRAME_MISSION },
    { MAV_CMD_DO_JUMP,          MAV_FRAME_MISSION },
};

const SimpleMissionItemTest::FactValue_t SimpleMissionItemTest::_rgFactValuesWaypoint[] = {
    { "Altitude:",      70.1234567 },
    { "Hold:",          10.1234567 },
};

const SimpleMissionItemTest::FactValue_t SimpleMissionItemTest::_rgFactValuesLoiterUnlim[] = {
    { "Altitude:",  70.1234567 },
    { "Radius:",    30.1234567 },
};

const SimpleMissionItemTest::FactValue_t SimpleMissionItemTest::_rgFactValuesLoiterTurns[] = {
    { "Altitude:",  70.1234567 },
    { "Radius:",    30.1234567 },
    { "Turns:",     10.1234567 },
};

const SimpleMissionItemTest::FactValue_t SimpleMissionItemTest::_rgFactValuesLoiterTime[] = {
    { "Altitude:",  70.1234567 },
    { "Radius:",    30.1234567 },
    { "Hold:",      10.1234567 },
};

const SimpleMissionItemTest::FactValue_t SimpleMissionItemTest::_rgFactValuesLand[] = {
    { "Altitude:",  70.1234567 },
    { "Abort Alt:", 10.1234567 },
    { "Heading:",   40.1234567 },
};

const SimpleMissionItemTest::FactValue_t SimpleMissionItemTest::_rgFactValuesTakeoff[] = {
    { "Pitch:",     10.1234567 },
    { "Altitude:",  70.1234567 },
};

const SimpleMissionItemTest::FactValue_t SimpleMissionItemTest::_rgFactValuesConditionDelay[] = {
    { "Hold:", 10.1234567 },
};

const SimpleMissionItemTest::FactValue_t SimpleMissionItemTest::_rgFactValuesDoJump[] = {
    { "Item #:",    10.1234567 },
    { "Repeat:",    20.1234567 },
};

const SimpleMissionItemTest::ItemExpected_t SimpleMissionItemTest::_rgItemExpected[] = {
    { sizeof(SimpleMissionItemTest::_rgFactValuesWaypoint)/sizeof(SimpleMissionItemTest::_rgFactValuesWaypoint[0]),             SimpleMissionItemTest::_rgFactValuesWaypoint,       true },
    { sizeof(SimpleMissionItemTest::_rgFactValuesLoiterUnlim)/sizeof(SimpleMissionItemTest::_rgFactValuesLoiterUnlim[0]),       SimpleMissionItemTest::_rgFactValuesLoiterUnlim,    true },
    { sizeof(SimpleMissionItemTest::_rgFactValuesLoiterTurns)/sizeof(SimpleMissionItemTest::_rgFactValuesLoiterTurns[0]),       SimpleMissionItemTest::_rgFactValuesLoiterTurns,    true },
    { sizeof(SimpleMissionItemTest::_rgFactValuesLoiterTime)/sizeof(SimpleMissionItemTest::_rgFactValuesLoiterTime[0]),         SimpleMissionItemTest::_rgFactValuesLoiterTime,     true },
    { sizeof(SimpleMissionItemTest::_rgFactValuesLand)/sizeof(SimpleMissionItemTest::_rgFactValuesLand[0]),                     SimpleMissionItemTest::_rgFactValuesLand,           true },
    { sizeof(SimpleMissionItemTest::_rgFactValuesTakeoff)/sizeof(SimpleMissionItemTest::_rgFactValuesTakeoff[0]),               SimpleMissionItemTest::_rgFactValuesTakeoff,        false },
    { sizeof(SimpleMissionItemTest::_rgFactValuesConditionDelay)/sizeof(SimpleMissionItemTest::_rgFactValuesConditionDelay[0]), SimpleMissionItemTest::_rgFactValuesConditionDelay, false },
    { sizeof(SimpleMissionItemTest::_rgFactValuesDoJump)/sizeof(SimpleMissionItemTest::_rgFactValuesDoJump[0]),                 SimpleMissionItemTest::_rgFactValuesDoJump,         false },
};

SimpleMissionItemTest::SimpleMissionItemTest(void)
{
    
}

void SimpleMissionItemTest::_testEditorFacts(void)
{
    for (size_t i=0; i<sizeof(_rgItemInfo)/sizeof(_rgItemInfo[0]); i++) {
        const ItemInfo_t* info = &_rgItemInfo[i];
        const ItemExpected_t* expected = &_rgItemExpected[i];
        
        qDebug() << "Command:" << info->command;
        
        MissionItem missionItem(1,              // sequence number
                                info->command,
                                info->frame,
                                10.1234567,     // param 1-7
                                20.1234567,
                                30.1234567,
                                40.1234567,
                                50.1234567,
                                60.1234567,
                                70.1234567,
                                true,           // autoContinue
                                false);         // isCurrentItem
        SimpleMissionItem simpleMissionItem(NULL /* Vehicle */, missionItem);

        // Validate that the fact values are correctly returned

        size_t factCount = 0;
        for (int i=0; i<simpleMissionItem.textFieldFacts()->count(); i++) {
            Fact* fact = qobject_cast<Fact*>(simpleMissionItem.textFieldFacts()->get(i));
            
            bool found = false;
            for (size_t j=0; j<expected->cFactValues; j++) {
                const FactValue_t* factValue = &expected->rgFactValues[j];
                
                if (factValue->name == fact->name()) {
                    QCOMPARE(fact->rawValue().toDouble(), factValue->value);
                    factCount ++;
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                qDebug() << fact->name();
            }
            QVERIFY(found);
        }
        QCOMPARE(factCount, expected->cFactValues);

        int expectedCount = expected->relativeAltCheckbox ? 1 : 0;
        QCOMPARE(simpleMissionItem.checkboxFacts()->count(), expectedCount);
    }
}

void SimpleMissionItemTest::_testDefaultValues(void)
{
    SimpleMissionItem item(NULL /* Vehicle */);

    item.missionItem().setCommand(MAV_CMD_NAV_WAYPOINT);
    item.missionItem().setFrame(MAV_FRAME_GLOBAL_RELATIVE_ALT);
    QCOMPARE(item.missionItem().param7(), SimpleMissionItem::defaultAltitude);
}

void SimpleMissionItemTest::_testSignals(void)
{
    enum {
        commandChangedIndex = 0,
        coordinateChangedIndex,
        exitCoordinateChangedIndex,
        dirtyChangedIndex,
        frameChangedIndex,
        friendlyEditAllowedChangedIndex,
        headingDegreesChangedIndex,
        rawEditChangedIndex,
        uiModelChangedIndex,
        showHomePositionChangedIndex,
        maxSignalIndex
    };

    enum {
        commandChangedMask =                1 << commandChangedIndex,
        coordinateChangedMask =             1 << coordinateChangedIndex,
        exitCoordinateChangedMask =         1 << exitCoordinateChangedIndex,
        dirtyChangedMask =                  1 << dirtyChangedIndex,
        frameChangedMask =                  1 << frameChangedIndex,
        friendlyEditAllowedChangedMask =    1 << friendlyEditAllowedChangedIndex,
        headingDegreesChangedMask =         1 << headingDegreesChangedIndex,
        rawEditChangedMask =                1 << rawEditChangedIndex,
        uiModelChangedMask =                1 << uiModelChangedIndex,
        showHomePositionChangedMask =       1 << showHomePositionChangedIndex,
    };

    static const size_t cSimpleMissionItemSignals = maxSignalIndex;
    const char*         rgSimpleMissionItemSignals[cSimpleMissionItemSignals];

    rgSimpleMissionItemSignals[commandChangedIndex] =               SIGNAL(commandChanged(int));
    rgSimpleMissionItemSignals[coordinateChangedIndex] =            SIGNAL(coordinateChanged(const QGeoCoordinate&));
    rgSimpleMissionItemSignals[exitCoordinateChangedIndex] =        SIGNAL(exitCoordinateChanged(const QGeoCoordinate&));
    rgSimpleMissionItemSignals[dirtyChangedIndex] =                 SIGNAL(dirtyChanged(bool));
    rgSimpleMissionItemSignals[frameChangedIndex] =                 SIGNAL(frameChanged(int));
    rgSimpleMissionItemSignals[friendlyEditAllowedChangedIndex] =   SIGNAL(friendlyEditAllowedChanged(bool));
    rgSimpleMissionItemSignals[headingDegreesChangedIndex] =        SIGNAL(headingDegreesChanged(double));
    rgSimpleMissionItemSignals[rawEditChangedIndex] =               SIGNAL(rawEditChanged(bool));
    rgSimpleMissionItemSignals[uiModelChangedIndex] =               SIGNAL(uiModelChanged());
    rgSimpleMissionItemSignals[showHomePositionChangedIndex] =      SIGNAL(showHomePositionChanged(bool));

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
    SimpleMissionItem simpleMissionItem(NULL /* Vehicle */, missionItem);

    // It's important top check that the right signals are emitted at the right time since that drives ui change.
    // It's also important to check that things are not being over-signalled when they should not be, since that can lead
    // to incorrect ui or perf impact of uneeded signals propogating ui change.

    MultiSignalSpy* multiSpy = new MultiSignalSpy();
    Q_CHECK_PTR(multiSpy);
    QCOMPARE(multiSpy->init(&simpleMissionItem, rgSimpleMissionItemSignals, cSimpleMissionItemSignals), true);

    // Check commandChanged signalling. Call setCommand should trigger:
    //      commandChanged
    //      dirtyChanged
    //      uiModelChanged
    //      coordinateChanged - since altitude will be set back to default

    simpleMissionItem.setCommand(MavlinkQmlSingleton::MAV_CMD_NAV_WAYPOINT);
    QVERIFY(multiSpy->checkNoSignals());
    simpleMissionItem.setCommand(MavlinkQmlSingleton::MAV_CMD_NAV_LOITER_TIME);
    QVERIFY(multiSpy->checkSignalsByMask(commandChangedMask | dirtyChangedMask | uiModelChangedMask | coordinateChangedMask));
    multiSpy->clearAllSignals();

    // Check coordinateChanged signalling. Calling setCoordinate should trigger:
    //      coordinateChanged
    //      dirtyChanged

    // Check that changing to the same coordinate does not signal
    simpleMissionItem.setCoordinate(QGeoCoordinate(50.1234567, 60.1234567, SimpleMissionItem::defaultAltitude));
    QVERIFY(multiSpy->checkNoSignals());

    // Check that actually changing coordinate signals correctly
    simpleMissionItem.setCoordinate(QGeoCoordinate(50.1234567, 60.1234567, 70.1234567));
    QVERIFY(multiSpy->checkOnlySignalByMask(coordinateChangedMask | dirtyChangedMask));
    multiSpy->clearAllSignals();

    // Check dirtyChanged signalling

    // Reset param 1-5 for testing
    simpleMissionItem.missionItem().setParam1(10.1234567);
    simpleMissionItem.missionItem().setParam2(20.1234567);
    simpleMissionItem.missionItem().setParam3(30.1234567);
    simpleMissionItem.missionItem().setParam4(40.1234567);
    multiSpy->clearAllSignals();

    simpleMissionItem.missionItem().setParam1(10.1234567);
    QVERIFY(multiSpy->checkNoSignals());
    simpleMissionItem.missionItem().setParam1(20.1234567);
    QVERIFY(multiSpy->checkOnlySignalByMask(dirtyChangedMask));
    multiSpy->clearAllSignals();

    simpleMissionItem.missionItem().setParam2(20.1234567);
    QVERIFY(multiSpy->checkNoSignals());
    simpleMissionItem.missionItem().setParam2(30.1234567);
    QVERIFY(multiSpy->checkOnlySignalByMask(dirtyChangedMask));
    multiSpy->clearAllSignals();

    simpleMissionItem.missionItem().setParam3(30.1234567);
    QVERIFY(multiSpy->checkNoSignals());
    simpleMissionItem.missionItem().setParam3(40.1234567);
    QVERIFY(multiSpy->checkOnlySignalByMask(dirtyChangedMask));
    multiSpy->clearAllSignals();

    simpleMissionItem.missionItem().setParam4(40.1234567);
    QVERIFY(multiSpy->checkNoSignals());
    simpleMissionItem.missionItem().setParam4(50.1234567);
    QVERIFY(multiSpy->checkOnlySignalByMask(dirtyChangedMask));
    multiSpy->clearAllSignals();

    // Check frameChanged signalling. Calling setFrame should signal:
    //      frameChanged
    //      dirtyChanged
    //      friendlyEditAllowedChanged - this signal is not very smart on when it gets sent

    simpleMissionItem.setCommand(MavlinkQmlSingleton::MAV_CMD_NAV_WAYPOINT);
    simpleMissionItem.missionItem().setFrame(MAV_FRAME_GLOBAL_RELATIVE_ALT);
    multiSpy->clearAllSignals();
    simpleMissionItem.missionItem().setFrame(MAV_FRAME_GLOBAL_RELATIVE_ALT);
    QVERIFY(multiSpy->checkNoSignals());
    simpleMissionItem.missionItem().setFrame(MAV_FRAME_GLOBAL);
    QVERIFY(multiSpy->checkOnlySignalByMask(frameChangedMask | dirtyChangedMask | friendlyEditAllowedChangedMask));
    multiSpy->clearAllSignals();
}
