/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SpeedSectionTest.h"
#include "PlanMasterController.h"

SpeedSectionTest::SpeedSectionTest(void)
    : _spySpeed(nullptr)
    , _spySection(nullptr)
    , _speedSection(nullptr)
{
    
}

void SpeedSectionTest::init(void)
{
    SectionTest::init();

    rgSpeedSignals[specifyFlightSpeedChangedIndex] = SIGNAL(specifyFlightSpeedChanged(bool));

    _speedSection = _simpleItem->speedSection();
    _createSpy(_speedSection, &_spySpeed);
    QVERIFY(_spySpeed);
    SectionTest::_createSpy(_speedSection, &_spySection);
    QVERIFY(_spySection);
}

void SpeedSectionTest::cleanup(void)
{
    delete _spySpeed;
    delete _spySection;
    SectionTest::cleanup();
}

void SpeedSectionTest::_createSpy(SpeedSection* speedSection, MultiSignalSpy** speedSpy)
{
    *speedSpy = nullptr;
    MultiSignalSpy* spy = new MultiSignalSpy();
    QCOMPARE(spy->init(speedSection, rgSpeedSignals, cSpeedSignals), true);
    *speedSpy = spy;
}

void SpeedSectionTest::_testDirty(void)
{
    // Check for dirty not signalled if same value
    _speedSection->setSpecifyFlightSpeed(_speedSection->specifyFlightSpeed());
    QVERIFY(_spySection->checkNoSignals());
    QCOMPARE(_speedSection->dirty(), false);
    _speedSection->flightSpeed()->setRawValue(_speedSection->flightSpeed()->rawValue());
    QVERIFY(_spySection->checkNoSignals());
    QCOMPARE(_speedSection->dirty(), false);

    // Check for no duplicate dirty signalling on change
    _speedSection->setSpecifyFlightSpeed(!_speedSection->specifyFlightSpeed());
    QVERIFY(_spySection->checkSignalByMask(dirtyChangedMask));
    QCOMPARE(_spySection->pullBoolFromSignalIndex(dirtyChangedIndex), true);
    QCOMPARE(_speedSection->dirty(), true);
    _spySection->clearAllSignals();
    _speedSection->setSpecifyFlightSpeed(!_speedSection->specifyFlightSpeed());
    QVERIFY(_spySection->checkNoSignalByMask(dirtyChangedMask));
    QCOMPARE(_speedSection->dirty(), true);
    _spySection->clearAllSignals();

    // Check that the dirty bit can be cleared
    _speedSection->setDirty(false);
    QVERIFY(_spySection->checkSignalByMask(dirtyChangedMask));
    QCOMPARE(_spySection->pullBoolFromSignalIndex(dirtyChangedIndex), false);
    QCOMPARE(_speedSection->dirty(), false);
    _spySection->clearAllSignals();

    // Flight speed change should only signal if specifyFlightSpeed is set

    _speedSection->setSpecifyFlightSpeed(false);
    _speedSection->setDirty(false);
    _spySection->clearAllSignals();
    _speedSection->flightSpeed()->setRawValue(_speedSection->flightSpeed()->rawValue().toDouble() + 1);
    QVERIFY(_spySection->checkNoSignalByMask(dirtyChangedMask));
    QCOMPARE(_speedSection->dirty(), false);

    _speedSection->setSpecifyFlightSpeed(true);
    _speedSection->setDirty(false);
    _spySection->clearAllSignals();
    _speedSection->flightSpeed()->setRawValue(_speedSection->flightSpeed()->rawValue().toDouble() + 1);
    QVERIFY(_spySection->checkSignalByMask(dirtyChangedMask));
    QCOMPARE(_spySection->pullBoolFromSignalIndex(dirtyChangedIndex), true);
    QCOMPARE(_speedSection->dirty(), true);
}

void SpeedSectionTest::_testSettingsAvailable(void)
{
    // No settings specified to start
    QCOMPARE(_speedSection->specifyFlightSpeed(), false);
    QCOMPARE(_speedSection->settingsSpecified(), false);

    // Check correct reaction to specifyFlightSpeed on/off

    _speedSection->setSpecifyFlightSpeed(true);
    QCOMPARE(_speedSection->specifyFlightSpeed(), true);
    QCOMPARE(_speedSection->settingsSpecified(), true);
    QVERIFY(_spySpeed->checkSignalByMask(specifyFlightSpeedChangedMask));
    QCOMPARE(_spySpeed->pullBoolFromSignalIndex(specifyFlightSpeedChangedIndex), true);
    QVERIFY(_spySection->checkSignalByMask(settingsSpecifiedChangedMask));
    QCOMPARE(_spySection->pullBoolFromSignalIndex(settingsSpecifiedChangedIndex), true);
    _spySection->clearAllSignals();
    _spySpeed->clearAllSignals();

    _speedSection->setSpecifyFlightSpeed(false);
    QCOMPARE(_speedSection->specifyFlightSpeed(), false);
    QCOMPARE(_speedSection->settingsSpecified(), false);
    QVERIFY(_spySpeed->checkSignalByMask(specifyFlightSpeedChangedMask));
    QCOMPARE(_spySpeed->pullBoolFromSignalIndex(specifyFlightSpeedChangedIndex), false);
    QVERIFY(_spySection->checkSignalByMask(settingsSpecifiedChangedMask));
    QCOMPARE(_spySection->pullBoolFromSignalIndex(settingsSpecifiedChangedIndex), false);
    _spySection->clearAllSignals();
    _spySpeed->clearAllSignals();
}

void SpeedSectionTest::_checkAvailable(void)
{
    MissionItem missionItem(1,              // sequence number
                            MAV_CMD_NAV_TAKEOFF,
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
    SimpleMissionItem* item = new SimpleMissionItem(_masterController, false /* flyView */, missionItem);
    QVERIFY(item->speedSection());
    QCOMPARE(item->speedSection()->available(), false);
}

void SpeedSectionTest::_testItemCount(void)
{
    // No settings specified to start
    QCOMPARE(_speedSection->itemCount(), 0);

    _speedSection->setSpecifyFlightSpeed(true);
    QCOMPARE(_speedSection->itemCount(), 1);
    QVERIFY(_spySection->checkSignalByMask(itemCountChangedMask));
    QCOMPARE(_spySection->pullIntFromSignalIndex(itemCountChangedIndex), 1);
    _spySection->clearAllSignals();
    _spySpeed->clearAllSignals();

    _speedSection->setSpecifyFlightSpeed(false);
    QCOMPARE(_speedSection->itemCount(), 0);
    QVERIFY(_spySection->checkSignalByMask(itemCountChangedMask));
    QCOMPARE(_spySection->pullIntFromSignalIndex(itemCountChangedIndex), 0);
    _spySection->clearAllSignals();
    _spySpeed->clearAllSignals();
}

void SpeedSectionTest::_testAppendSectionItems(void)
{
    int seqNum = 0;
    QList<MissionItem*> rgMissionItems;

    // No settings specified to start
    QCOMPARE(_speedSection->itemCount(), 0);

    _speedSection->appendSectionItems(rgMissionItems, this, seqNum);
    QCOMPARE(rgMissionItems.count(), 0);
    QCOMPARE(seqNum, 0);
    rgMissionItems.clear();

    _speedSection->setSpecifyFlightSpeed(true);
    _speedSection->appendSectionItems(rgMissionItems, this, seqNum);
    QCOMPARE(rgMissionItems.count(), 1);
    QCOMPARE(seqNum, 1);
    MissionItem expectedSpeedItem(0, MAV_CMD_DO_CHANGE_SPEED, MAV_FRAME_MISSION, _controllerVehicle->multiRotor() ? 1 : 0, _speedSection->flightSpeed()->rawValue().toDouble(), -1, 0, 0, 0, 0, true, false);
    _missionItemsEqual(*rgMissionItems[0], expectedSpeedItem);
}

void SpeedSectionTest::_testScanForSection(void)
{
    QCOMPARE(_speedSection->available(), true);

    int scanIndex = 0;
    QmlObjectListModel visualItems;

    _commonScanTest(_speedSection);

    // Check for a scan success

    double flightSpeed = 10.123456;
    MissionItem validSpeedItem(0, MAV_CMD_DO_CHANGE_SPEED, MAV_FRAME_MISSION, _controllerVehicle->multiRotor() ? 1 : 0, flightSpeed, -1, 0, 0, 0, 0, true, false);
    SimpleMissionItem simpleItem(_masterController, false /* flyView */, validSpeedItem);
    MissionItem& simpleMissionItem = simpleItem.missionItem();
    visualItems.append(&simpleItem);
    scanIndex = 0;
    QCOMPARE(_speedSection->scanForSection(&visualItems, scanIndex), true);
    QCOMPARE(visualItems.count(), 0);
    QCOMPARE(_speedSection->settingsSpecified(), true);
    QCOMPARE(_speedSection->specifyFlightSpeed(), true);
    QCOMPARE(_speedSection->flightSpeed()->rawValue().toDouble(), flightSpeed);
    _speedSection->setSpecifyFlightSpeed(false);
    visualItems.clear();
    scanIndex = 0;

    // Flight speed command but incorrect settings

    simpleMissionItem = validSpeedItem;
    simpleMissionItem.setParam1(_controllerVehicle->multiRotor() ? 0 : 1);
    visualItems.append(&simpleItem);
    QCOMPARE(_speedSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_speedSection->settingsSpecified(), false);
    visualItems.clear();
    scanIndex = 0;

    simpleMissionItem = validSpeedItem;
    simpleMissionItem.setParam3(50);
    visualItems.append(&simpleItem);
    QCOMPARE(_speedSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_speedSection->settingsSpecified(), false);
    visualItems.clear();
    scanIndex = 0;

    simpleMissionItem = validSpeedItem;
    simpleMissionItem.setParam4(1);
    visualItems.append(&simpleItem);
    QCOMPARE(_speedSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_speedSection->settingsSpecified(), false);
    visualItems.clear();
    scanIndex = 0;

    simpleMissionItem = validSpeedItem;
    simpleMissionItem.setParam5(1);
    visualItems.append(&simpleItem);
    QCOMPARE(_speedSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_speedSection->settingsSpecified(), false);
    visualItems.clear();
    scanIndex = 0;

    simpleMissionItem = validSpeedItem;
    simpleMissionItem.setParam6(1);
    visualItems.append(&simpleItem);
    QCOMPARE(_speedSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_speedSection->settingsSpecified(), false);
    visualItems.clear();
    scanIndex = 0;

    simpleMissionItem = validSpeedItem;
    simpleMissionItem.setParam7(1);
    visualItems.append(&simpleItem);
    QCOMPARE(_speedSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 1);
    QCOMPARE(_speedSection->settingsSpecified(), false);
    visualItems.clear();
    scanIndex = 0;

    // Valid item in wrong position
    MissionItem waypointMissionItem(0, MAV_CMD_NAV_WAYPOINT, MAV_FRAME_GLOBAL_RELATIVE_ALT, 0, 0, 0, 0, 0, 0, 0, true, false);
    SimpleMissionItem simpleWaypointItem(_masterController, false /* flyView */, waypointMissionItem);
    simpleMissionItem = validSpeedItem;
    visualItems.append(&simpleWaypointItem);
    visualItems.append(&simpleMissionItem);
    QCOMPARE(_speedSection->scanForSection(&visualItems, scanIndex), false);
    QCOMPARE(visualItems.count(), 2);
    QCOMPARE(_speedSection->settingsSpecified(), false);
    visualItems.clear();
    scanIndex = 0;
}

void SpeedSectionTest::_testSpecifiedFlightSpeedChanged(void)
{
    // specifiedFlightSpeedChanged SHOULD NOT signal if flight speed is changed when specifyFlightSpeed IS NOT set
    _speedSection->setSpecifyFlightSpeed(false);
    _spySpeed->clearAllSignals();
    _speedSection->flightSpeed()->setRawValue(_speedSection->flightSpeed()->rawValue().toDouble() + 1);
    QVERIFY(_spySpeed->checkNoSignalByMask(specifiedFlightSpeedChangedMask));

    // specifiedFlightSpeedChanged SHOULD signal if flight speed is changed when specifyFlightSpeed IS set
    _speedSection->setSpecifyFlightSpeed(true);
    _spySpeed->clearAllSignals();
    _speedSection->flightSpeed()->setRawValue(_speedSection->flightSpeed()->rawValue().toDouble() + 1);
    QVERIFY(_spySpeed->checkSignalByMask(specifiedFlightSpeedChangedMask));
}
