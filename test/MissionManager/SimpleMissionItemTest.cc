/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SimpleMissionItemTest.h"
#include "QGCApplication.h"
#include "QGroundControlQmlGlobal.h"
#include "SettingsManager.h"
#include "PlanMasterController.h"

static const ItemInfo_t _rgItemInfo[] = {
    { MAV_CMD_NAV_WAYPOINT,     MAV_FRAME_GLOBAL_RELATIVE_ALT },
    { MAV_CMD_NAV_LOITER_UNLIM, MAV_FRAME_GLOBAL },
    { MAV_CMD_NAV_LOITER_TURNS, MAV_FRAME_GLOBAL_RELATIVE_ALT },
    { MAV_CMD_NAV_LOITER_TIME,  MAV_FRAME_GLOBAL },
    { MAV_CMD_NAV_LAND,         MAV_FRAME_GLOBAL_RELATIVE_ALT },
    { MAV_CMD_NAV_TAKEOFF,      MAV_FRAME_GLOBAL },
    { MAV_CMD_DO_JUMP,          MAV_FRAME_MISSION },
};

static const FactValue_t _rgFactValuesWaypoint[] = {
    { "Hold",   QGCMAVLink::VehicleClassMultiRotor, false,  1 },
    { "Yaw",    QGCMAVLink::VehicleClassMultiRotor, true,   4 },
};

static const FactValue_t _rgFactValuesLoiterUnlim[] = {
    { "Radius", QGCMAVLink::VehicleClassFixedWing,  false,  3 },
    { "Yaw",    QGCMAVLink::VehicleClassMultiRotor, true,   4 },
};

static const FactValue_t _rgFactValuesLoiterTurns[] = {
    { "Turns",  QGCMAVLink::VehicleClassFixedWing,  false, 1 },
    { "Radius", QGCMAVLink::VehicleClassFixedWing,  false, 3 },
};

static const FactValue_t _rgFactValuesLoiterTime[] = {
    { "Loiter Time",    QGCMAVLink::VehicleClassGeneric,    false, 1 },
    { "Radius",         QGCMAVLink::VehicleClassFixedWing,  false, 3 },
};

static const FactValue_t _rgFactValuesLand[] = {
    { "Yaw", QGCMAVLink::VehicleClassMultiRotor, true, 4 },
};

static const FactValue_t _rgFactValuesTakeoff[] = {
    { "Pitch",  QGCMAVLink::VehicleClassFixedWing,  false,  1 },
    { "Yaw",    QGCMAVLink::VehicleClassMultiRotor, true,   4 },
};

static const FactValue_t _rgFactValuesDoJump[] = {
    { "Item #", QGCMAVLink::VehicleClassGeneric, false, 1 },
    { "Repeat", QGCMAVLink::VehicleClassGeneric, false, 2 },
};

const ItemExpected_t _rgItemExpected[] = {
    { sizeof(_rgFactValuesWaypoint)/sizeof(_rgFactValuesWaypoint[0]),       _rgFactValuesWaypoint,      70.1234567, QGroundControlQmlGlobal::AltitudeModeRelative },
    { sizeof(_rgFactValuesLoiterUnlim)/sizeof(_rgFactValuesLoiterUnlim[0]), _rgFactValuesLoiterUnlim,   70.1234567, QGroundControlQmlGlobal::AltitudeModeAbsolute },
    { sizeof(_rgFactValuesLoiterTurns)/sizeof(_rgFactValuesLoiterTurns[0]), _rgFactValuesLoiterTurns,   70.1234567, QGroundControlQmlGlobal::AltitudeModeRelative },
    { sizeof(_rgFactValuesLoiterTime)/sizeof(_rgFactValuesLoiterTime[0]),   _rgFactValuesLoiterTime,    70.1234567, QGroundControlQmlGlobal::AltitudeModeAbsolute },
    { sizeof(_rgFactValuesLand)/sizeof(_rgFactValuesLand[0]),               _rgFactValuesLand,          70.1234567, QGroundControlQmlGlobal::AltitudeModeRelative },
    { sizeof(_rgFactValuesTakeoff)/sizeof(_rgFactValuesTakeoff[0]),         _rgFactValuesTakeoff,       70.1234567, QGroundControlQmlGlobal::AltitudeModeAbsolute },
    { sizeof(_rgFactValuesDoJump)/sizeof(_rgFactValuesDoJump[0]),           _rgFactValuesDoJump,        qQNaN(),    QGroundControlQmlGlobal::AltitudeModeRelative },
};

SimpleMissionItemTest::SimpleMissionItemTest(void)
    : _simpleItem(nullptr)
{    
    rgSimpleItemSignals[commandChangedIndex] =                          SIGNAL(commandChanged(int));
    rgSimpleItemSignals[altitudeModeChangedIndex] =                     SIGNAL(altitudeModeChanged());
    rgSimpleItemSignals[friendlyEditAllowedChangedIndex] =              SIGNAL(friendlyEditAllowedChanged(bool));
    rgSimpleItemSignals[headingDegreesChangedIndex] =                   SIGNAL(headingDegreesChanged(double));
    rgSimpleItemSignals[rawEditChangedIndex] =                          SIGNAL(rawEditChanged(bool));
    rgSimpleItemSignals[cameraSectionChangedIndex] =                    SIGNAL(cameraSectionChanged(QObject*));
    rgSimpleItemSignals[speedSectionChangedIndex] =                     SIGNAL(speedSectionChanged(QObject*));
}

void SimpleMissionItemTest::init(void)
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

    // It's important top check that the right signals are emitted at the right time since that drives ui change.
    // It's also important to check that things are not being over-signalled when they should not be, since that can lead
    // to incorrect ui or perf impact of uneeded signals propogating ui change.

    _spySimpleItem = new MultiSignalSpy();
    QCOMPARE(_spySimpleItem->init(_simpleItem, rgSimpleItemSignals, cSimpleItemSignals), true);
    VisualMissionItemTest::_createSpy(_simpleItem, &_spyVisualItem);
}

void SimpleMissionItemTest::cleanup(void)
{
    VisualMissionItemTest::cleanup();

    // These items go away from _masterController is deleted
    _simpleItem = nullptr;
}

bool SimpleMissionItemTest::_classMatch(QGCMAVLink::VehicleClass_t vehicleClass, QGCMAVLink::VehicleClass_t testClass)
{
    return vehicleClass == QGCMAVLink::VehicleClassGeneric || vehicleClass == testClass;
}

void SimpleMissionItemTest::_testEditorFactsWorker(QGCMAVLink::VehicleClass_t vehicleClass, QGCMAVLink::VehicleClass_t vtolMode, const ItemExpected_t* rgExpected)
{
    qDebug() << "vehicleClass:vtolMode" << QGCMAVLink::vehicleClassToString(vehicleClass) << QGCMAVLink::vehicleClassToString(vtolMode);

    PlanMasterController planController(MAV_AUTOPILOT_PX4, QGCMAVLink::vehicleClassToMavType(vehicleClass));

    QGCMAVLink::VehicleClass_t commandVehicleClass = vtolMode == QGCMAVLink::VehicleClassGeneric ? vehicleClass : vtolMode;

    for (size_t i=0; i<sizeof(_rgItemInfo)/sizeof(_rgItemInfo[0]); i++) {
        const ItemInfo_t*       info        = &_rgItemInfo[i];
        const ItemExpected_t*   expected    = &rgExpected[i];

        qDebug() << "Command" << info->command;

        // Determine how many fact values we should get back
        int cExpectedTextFieldFacts = 0;
        int cExpectedNaNFieldFacts  = 0;
        for (size_t j=0; j<expected->cFactValues; j++) {
            const FactValue_t* factValue = &expected->rgFactValues[j];

            if (!_classMatch(factValue->vehicleClass, commandVehicleClass)) {
                continue;
            }
            if (factValue->nanValue) {
                cExpectedNaNFieldFacts++;

            } else {
                cExpectedTextFieldFacts++;
            }
        }

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
        SimpleMissionItem simpleMissionItem(&planController, false /* flyView */, missionItem);

        MissionController::MissionFlightStatus_t missionFlightStatus;
        missionFlightStatus.vtolMode        = vtolMode;
        missionFlightStatus.vehicleSpeed    = 10;
        missionFlightStatus.gimbalYaw       = qQNaN();
        missionFlightStatus.gimbalPitch     = qQNaN();
        simpleMissionItem.setMissionFlightStatus(missionFlightStatus);

        // Validate that the fact values are correctly returned

        int foundTextFieldCount = 0;
        for (int i=0; i<simpleMissionItem.textFieldFacts()->count(); i++) {
            Fact* fact = qobject_cast<Fact*>(simpleMissionItem.textFieldFacts()->get(i));

            bool found = false;
            for (size_t j=0; j<expected->cFactValues; j++) {
                const FactValue_t* factValue = &expected->rgFactValues[j];

                if (!_classMatch(factValue->vehicleClass, commandVehicleClass)) {
                    continue;
                }

                if (factValue->name == fact->name()) {
                    QCOMPARE(fact->rawValue().toDouble(), (factValue->paramIndex * 10.0) + 0.1234567);
                    foundTextFieldCount ++;
                    found = true;
                    break;
                }
            }

            qDebug() << "textFieldFact" << fact->name();
            QVERIFY(found);
        }
        QCOMPARE(foundTextFieldCount, cExpectedTextFieldFacts);

        int foundNaNFieldCount = 0;
        for (int i=0; i<simpleMissionItem.nanFacts()->count(); i++) {
            Fact* fact = qobject_cast<Fact*>(simpleMissionItem.nanFacts()->get(i));

            bool found = false;
            for (size_t j=0; j<expected->cFactValues; j++) {
                const FactValue_t* factValue = &expected->rgFactValues[j];

                if (!_classMatch(factValue->vehicleClass, commandVehicleClass)) {
                    continue;
                }

                if (factValue->name == fact->name()) {
                    QCOMPARE(fact->rawValue().toDouble(), (factValue->paramIndex * 10.0) + 0.1234567);
                    foundNaNFieldCount ++;
                    found = true;
                    break;
                }
            }

            qDebug() << "nanFieldFact" << fact->name();
            QVERIFY(found);
        }
        QCOMPARE(foundNaNFieldCount, cExpectedNaNFieldFacts);

        if (!qIsNaN(expected->altValue)) {
            QCOMPARE(simpleMissionItem.altitudeMode(), expected->altMode);
            QCOMPARE(simpleMissionItem.altitude()->rawValue().toDouble(), expected->altValue);
        }
    }
}

void SimpleMissionItemTest::_testEditorFacts(void)
{
    _testEditorFactsWorker(QGCMAVLink::VehicleClassMultiRotor,  QGCMAVLink::VehicleClassGeneric,    _rgItemExpected);
    _testEditorFactsWorker(QGCMAVLink::VehicleClassFixedWing,   QGCMAVLink::VehicleClassGeneric,    _rgItemExpected);
    _testEditorFactsWorker(QGCMAVLink::VehicleClassVTOL,        QGCMAVLink::VehicleClassMultiRotor, _rgItemExpected);
    _testEditorFactsWorker(QGCMAVLink::VehicleClassVTOL,        QGCMAVLink::VehicleClassFixedWing,  _rgItemExpected);
}

void SimpleMissionItemTest::_testDefaultValues(void)
{
    SimpleMissionItem item(_masterController, false /* flyView */, false /* forLoad */);

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
    QVERIFY(_spyVisualItem->checkOnlySignalByMask(coordinateChangedMask | exitCoordinateChangedMask | dirtyChangedMask));
    _spyVisualItem->clearAllSignals();
    _simpleItem->setCoordinate(QGeoCoordinate(missionItem.param5(), missionItem.param6() + 1, missionItem.param7()));
    QVERIFY(_spyVisualItem->checkOnlySignalByMask(coordinateChangedMask | exitCoordinateChangedMask | dirtyChangedMask));
    _spyVisualItem->clearAllSignals();

    // Altitude in coordinate is not used in setCoordinate
    _simpleItem->setCoordinate(QGeoCoordinate(missionItem.param5(), missionItem.param6(), missionItem.param7() + 1));
    QVERIFY(_spyVisualItem->checkNoSignals());
    QVERIFY(_spySimpleItem->checkNoSignals());
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

    _simpleItem->setAltitudeMode(_simpleItem->altitudeMode() == QGroundControlQmlGlobal::AltitudeModeRelative ? QGroundControlQmlGlobal::AltitudeModeAbsolute : QGroundControlQmlGlobal::AltitudeModeRelative);
    QVERIFY(_spySimpleItem->checkOnlySignalByMask(dirtyChangedMask | friendlyEditAllowedChangedMask | altitudeModeChangedMask));
    _spySimpleItem->clearAllSignals();
    _spyVisualItem->clearAllSignals();

    // Check commandChanged signalling. Call setCommand should trigger:
    //      commandChanged
    //      commandNameChanged
    //      dirtyChanged
    //      coordinateChanged - since altitude will be set back to default

    _simpleItem->setCommand(MAV_CMD_NAV_WAYPOINT);
    QVERIFY(_spyVisualItem->checkNoSignals());
    QVERIFY(_spySimpleItem->checkNoSignals());

    _simpleItem->setCommand(MAV_CMD_NAV_LOITER_TIME);
    QVERIFY(_spySimpleItem->checkSignalsByMask(commandChangedMask));
    QVERIFY(_spyVisualItem->checkSignalsByMask(commandNameChangedMask | dirtyChangedMask));
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

void SimpleMissionItemTest::_testAltitudePropogation(void)
{
    // Make sure that changes to altitude propogate to param 7 of the mission item

    _simpleItem->setAltitudeMode(QGroundControlQmlGlobal::AltitudeModeRelative);
    _simpleItem->altitude()->setRawValue(_simpleItem->altitude()->rawValue().toDouble() + 1);
    QCOMPARE(_simpleItem->altitude()->rawValue().toDouble(), _simpleItem->missionItem().param7());
    QCOMPARE(_simpleItem->missionItem().frame(), MAV_FRAME_GLOBAL_RELATIVE_ALT);

    _simpleItem->setAltitudeMode(QGroundControlQmlGlobal::AltitudeModeAbsolute);
    _simpleItem->altitude()->setRawValue(_simpleItem->altitude()->rawValue().toDouble() + 1);
    QCOMPARE(_simpleItem->altitude()->rawValue().toDouble(), _simpleItem->missionItem().param7());
    QCOMPARE(_simpleItem->missionItem().frame(), MAV_FRAME_GLOBAL);
}
