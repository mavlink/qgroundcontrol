/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "SimpleMissionItemTest.h"
#include "QGCApplication.h"
#include "QGroundControlQmlGlobal.h"
#include "SettingsManager.h"

const SimpleMissionItemTest::ItemInfo_t SimpleMissionItemTest::_rgItemInfo[] = {
    { MAV_CMD_NAV_WAYPOINT,     MAV_FRAME_GLOBAL_RELATIVE_ALT },
    { MAV_CMD_NAV_LOITER_UNLIM, MAV_FRAME_GLOBAL_RELATIVE_ALT },
    { MAV_CMD_NAV_LOITER_TURNS, MAV_FRAME_GLOBAL_RELATIVE_ALT },
    { MAV_CMD_NAV_LOITER_TIME,  MAV_FRAME_GLOBAL_RELATIVE_ALT },
    { MAV_CMD_NAV_LAND,         MAV_FRAME_GLOBAL_RELATIVE_ALT },
    { MAV_CMD_NAV_TAKEOFF,      MAV_FRAME_GLOBAL_RELATIVE_ALT },
    { MAV_CMD_DO_JUMP,          MAV_FRAME_MISSION },
};

const SimpleMissionItemTest::FactValue_t SimpleMissionItemTest::_rgFactValuesWaypoint[] = {
    { "Altitude",   70.1234567 },
    { "Hold",       10.1234567 },
};

const SimpleMissionItemTest::FactValue_t SimpleMissionItemTest::_rgFactValuesLoiterUnlim[] = {
    { "Altitude",   70.1234567 },
    { "Radius",     30.1234567 },
};

const SimpleMissionItemTest::FactValue_t SimpleMissionItemTest::_rgFactValuesLoiterTurns[] = {
    { "Altitude",   70.1234567 },
    { "Radius",     30.1234567 },
    { "Turns",      10.1234567 },
};

const SimpleMissionItemTest::FactValue_t SimpleMissionItemTest::_rgFactValuesLoiterTime[] = {
    { "Altitude",   70.1234567 },
    { "Radius",     30.1234567 },
    { "Hold",       10.1234567 },
};

const SimpleMissionItemTest::FactValue_t SimpleMissionItemTest::_rgFactValuesLand[] = {
    { "Altitude",   70.1234567 },
};

const SimpleMissionItemTest::FactValue_t SimpleMissionItemTest::_rgFactValuesTakeoff[] = {
    { "Pitch",      10.1234567 },
    { "Altitude",   70.1234567 },
};

const SimpleMissionItemTest::FactValue_t SimpleMissionItemTest::_rgFactValuesDoJump[] = {
    { "Item #",     10.1234567 },
    { "Repeat",     20.1234567 },
};

const SimpleMissionItemTest::ItemExpected_t SimpleMissionItemTest::_rgItemExpected[] = {
    { sizeof(SimpleMissionItemTest::_rgFactValuesWaypoint)/sizeof(SimpleMissionItemTest::_rgFactValuesWaypoint[0]),             SimpleMissionItemTest::_rgFactValuesWaypoint,       true },
    { sizeof(SimpleMissionItemTest::_rgFactValuesLoiterUnlim)/sizeof(SimpleMissionItemTest::_rgFactValuesLoiterUnlim[0]),       SimpleMissionItemTest::_rgFactValuesLoiterUnlim,    true },
    { sizeof(SimpleMissionItemTest::_rgFactValuesLoiterTurns)/sizeof(SimpleMissionItemTest::_rgFactValuesLoiterTurns[0]),       SimpleMissionItemTest::_rgFactValuesLoiterTurns,    true },
    { sizeof(SimpleMissionItemTest::_rgFactValuesLoiterTime)/sizeof(SimpleMissionItemTest::_rgFactValuesLoiterTime[0]),         SimpleMissionItemTest::_rgFactValuesLoiterTime,     true },
    { sizeof(SimpleMissionItemTest::_rgFactValuesLand)/sizeof(SimpleMissionItemTest::_rgFactValuesLand[0]),                     SimpleMissionItemTest::_rgFactValuesLand,           true },
    { sizeof(SimpleMissionItemTest::_rgFactValuesTakeoff)/sizeof(SimpleMissionItemTest::_rgFactValuesTakeoff[0]),               SimpleMissionItemTest::_rgFactValuesTakeoff,        true },
    { sizeof(SimpleMissionItemTest::_rgFactValuesDoJump)/sizeof(SimpleMissionItemTest::_rgFactValuesDoJump[0]),                 SimpleMissionItemTest::_rgFactValuesDoJump,         false },
};

SimpleMissionItemTest::SimpleMissionItemTest(void)
    : _simpleItem(NULL)
{
    
}

void SimpleMissionItemTest::init(void)
{
    VisualMissionItemTest::init();

    rgSimpleItemSignals[commandChangedIndex] =                          SIGNAL(commandChanged(int));
    rgSimpleItemSignals[frameChangedIndex] =                            SIGNAL(frameChanged(int));
    rgSimpleItemSignals[friendlyEditAllowedChangedIndex] =              SIGNAL(friendlyEditAllowedChanged(bool));
    rgSimpleItemSignals[headingDegreesChangedIndex] =                   SIGNAL(headingDegreesChanged(double));
    rgSimpleItemSignals[rawEditChangedIndex] =                          SIGNAL(rawEditChanged(bool));
    rgSimpleItemSignals[cameraSectionChangedIndex] =                    SIGNAL(rawEditChanged(bool));
    rgSimpleItemSignals[speedSectionChangedIndex] =                     SIGNAL(rawEditChanged(bool));
    rgSimpleItemSignals[coordinateHasRelativeAltitudeChangedIndex] =    SIGNAL(coordinateHasRelativeAltitudeChanged(bool));

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
    _simpleItem = new SimpleMissionItem(_offlineVehicle, missionItem);

    // It's important top check that the right signals are emitted at the right time since that drives ui change.
    // It's also important to check that things are not being over-signalled when they should not be, since that can lead
    // to incorrect ui or perf impact of uneeded signals propogating ui change.

    _spySimpleItem = new MultiSignalSpy();
    QCOMPARE(_spySimpleItem->init(_simpleItem, rgSimpleItemSignals, cSimpleItemSignals), true);
    VisualMissionItemTest::_createSpy(_simpleItem, &_spyVisualItem);
}

void SimpleMissionItemTest::cleanup(void)
{
    delete _simpleItem;
    VisualMissionItemTest::cleanup();
}

void SimpleMissionItemTest::_testEditorFacts(void)
{
    Vehicle* vehicle = new Vehicle(MAV_AUTOPILOT_PX4, MAV_TYPE_FIXED_WING, qgcApp()->toolbox()->firmwarePluginManager());

    for (size_t i=0; i<sizeof(_rgItemInfo)/sizeof(_rgItemInfo[0]); i++) {
        const ItemInfo_t* info = &_rgItemInfo[i];
        const ItemExpected_t* expected = &_rgItemExpected[i];
        
        qDebug() << "Command" << info->command;
        
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
        SimpleMissionItem simpleMissionItem(vehicle, missionItem);

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
            
            qDebug() << "textFieldFact" << fact->name();
            QVERIFY(found);
        }
        QCOMPARE(factCount, expected->cFactValues);

        int expectedCount = expected->relativeAltCheckbox ? 1 : 0;
        QCOMPARE(simpleMissionItem.checkboxFacts()->count(), expectedCount);
    }

    delete vehicle;
}

void SimpleMissionItemTest::_testDefaultValues(void)
{
    SimpleMissionItem item(_offlineVehicle);

    item.missionItem().setCommand(MAV_CMD_NAV_WAYPOINT);
    item.missionItem().setFrame(MAV_FRAME_GLOBAL_RELATIVE_ALT);
    QCOMPARE(item.missionItem().param7(), qgcApp()->toolbox()->settingsManager()->appSettings()->defaultMissionItemAltitude()->rawValue().toDouble());
}

void SimpleMissionItemTest::_testSignals(void)
{
    MissionItem& missionItem = _simpleItem->missionItem();

    // Check that changing to the same coordinate does not signal
    _simpleItem->setCoordinate(QGeoCoordinate(missionItem.param5(), missionItem.param6(), missionItem.param7()));
    QVERIFY(_spyVisualItem->checkNoSignals());
    QVERIFY(_spySimpleItem->checkNoSignals());

    // Check coordinateChanged signalling. Calling setCoordinate should trigger:
    //      coordinateChanged
    //      dirtyChanged

    // Check that actually changing coordinate signals correctly
    _simpleItem->setCoordinate(QGeoCoordinate(missionItem.param5() + 1, missionItem.param6(), missionItem.param7()));
    QVERIFY(_spyVisualItem->checkOnlySignalByMask(coordinateChangedMask | dirtyChangedMask));
    _spyVisualItem->clearAllSignals();
    _simpleItem->setCoordinate(QGeoCoordinate(missionItem.param5(), missionItem.param6() + 1, missionItem.param7()));
    QVERIFY(_spyVisualItem->checkOnlySignalByMask(coordinateChangedMask | dirtyChangedMask));
    _spyVisualItem->clearAllSignals();
    _simpleItem->setCoordinate(QGeoCoordinate(missionItem.param5(), missionItem.param6(), missionItem.param7() + 1));
    QVERIFY(_spyVisualItem->checkOnlySignalByMask(coordinateChangedMask | dirtyChangedMask));
    _spyVisualItem->clearAllSignals();

    // Check dirtyChanged signalling

    missionItem.setParam1(missionItem.param1());
    QVERIFY(_spyVisualItem->checkNoSignals());
    missionItem.setParam2(missionItem.param2());
    QVERIFY(_spyVisualItem->checkNoSignals());
    missionItem.setParam3(missionItem.param3());
    QVERIFY(_spyVisualItem->checkNoSignals());
    missionItem.setParam4(missionItem.param4());
    QVERIFY(_spyVisualItem->checkNoSignals());

    missionItem.setParam1(missionItem.param1() + 1);
    QVERIFY(_spyVisualItem->checkOnlySignalByMask(dirtyChangedMask));
    _spyVisualItem->clearAllSignals();
    missionItem.setParam1(missionItem.param2() + 1);
    QVERIFY(_spyVisualItem->checkOnlySignalByMask(dirtyChangedMask));
    _spyVisualItem->clearAllSignals();
    missionItem.setParam1(missionItem.param3() + 1);
    QVERIFY(_spyVisualItem->checkOnlySignalByMask(dirtyChangedMask));
    _spyVisualItem->clearAllSignals();
    missionItem.setParam1(missionItem.param4() + 1);
    QVERIFY(_spyVisualItem->checkOnlySignalByMask(dirtyChangedMask));
    _spyVisualItem->clearAllSignals();

    // Check frameChanged signalling. Calling setFrame should signal:
    //      frameChanged
    //      dirtyChanged
    //      friendlyEditAllowedChanged - this signal is not very smart on when it gets sent
    //      coordinateHasRelativeAltitudeChanged

    missionItem.setFrame(MAV_FRAME_GLOBAL_RELATIVE_ALT);
    QVERIFY(_spyVisualItem->checkNoSignals());
    QVERIFY(_spySimpleItem->checkNoSignals());

    missionItem.setFrame(MAV_FRAME_GLOBAL);
    QVERIFY(_spySimpleItem->checkOnlySignalByMask(frameChangedMask | dirtyChangedMask | friendlyEditAllowedChangedMask | coordinateHasRelativeAltitudeChangedMask));
    _spySimpleItem->clearAllSignals();
    _spyVisualItem->clearAllSignals();

    // Check commandChanged signalling. Call setCommand should trigger:
    //      commandChanged
    //      commandNameChanged
    //      dirtyChanged
    //      coordinateChanged - since altitude will be set back to default

    _simpleItem->setCommand(MavlinkQmlSingleton::MAV_CMD_NAV_WAYPOINT);
    QVERIFY(_spyVisualItem->checkNoSignals());
    QVERIFY(_spySimpleItem->checkNoSignals());

    _simpleItem->setCommand(MavlinkQmlSingleton::MAV_CMD_NAV_LOITER_TIME);
    QVERIFY(_spySimpleItem->checkSignalsByMask(commandChangedMask));
    QVERIFY(_spyVisualItem->checkSignalsByMask(commandNameChangedMask | dirtyChangedMask | coordinateChangedMask));
}

void SimpleMissionItemTest::_testCameraSectionDirty(void)
{
    CameraSection* cameraSection = _simpleItem->cameraSection();

    QVERIFY(!cameraSection->dirty());
    QVERIFY(!_simpleItem->dirty());

    // Dirtying the camera section should also dirty the item
    cameraSection->setDirty(true);
    QVERIFY(_simpleItem->dirty());

    // Clearing the dirty bit from the item should also clear the dirty bit on the camera section
    _simpleItem->setDirty(false);
    QVERIFY(!cameraSection->dirty());
}

void SimpleMissionItemTest::_testSpeedSectionDirty(void)
{
    SpeedSection* speedSection = _simpleItem->speedSection();

    QVERIFY(!speedSection->dirty());
    QVERIFY(!_simpleItem->dirty());

    // Dirtying the speed section should also dirty the item
    speedSection->setDirty(true);
    QVERIFY(_simpleItem->dirty());

    // Clearing the dirty bit from the item should also clear the dirty bit on the camera section
    _simpleItem->setDirty(false);
    QVERIFY(!speedSection->dirty());
}

void SimpleMissionItemTest::_testCameraSection(void)
{
    // No gimbal yaw to start with
    QVERIFY(qIsNaN(_simpleItem->specifiedGimbalYaw()));
    QVERIFY(qIsNaN(_simpleItem->missionGimbalYaw()));
    QCOMPARE(_simpleItem->dirty(), false);

    double gimbalYaw = 10.1234;
    _simpleItem->cameraSection()->setSpecifyGimbal(true);
    _simpleItem->cameraSection()->gimbalYaw()->setRawValue(gimbalYaw);
    QCOMPARE(_simpleItem->specifiedGimbalYaw(), gimbalYaw);
    QVERIFY(qIsNaN(_simpleItem->missionGimbalYaw()));
    QCOMPARE(_spyVisualItem->checkSignalsByMask(specifiedGimbalYawChangedMask), true);
    QCOMPARE(_simpleItem->dirty(), true);
}


void SimpleMissionItemTest::_testSpeedSection(void)
{
    // No flight speed
    QVERIFY(qIsNaN(_simpleItem->specifiedFlightSpeed()));
    QCOMPARE(_simpleItem->dirty(), false);

    double flightSpeed = 10.1234;
    _simpleItem->speedSection()->setSpecifyFlightSpeed(true);
    _simpleItem->speedSection()->flightSpeed()->setRawValue(flightSpeed);
    QCOMPARE(_simpleItem->specifiedFlightSpeed(), flightSpeed);
    QCOMPARE(_spyVisualItem->checkSignalsByMask(specifiedFlightSpeedChangedMask), true);
    QCOMPARE(_simpleItem->dirty(), true);
}
