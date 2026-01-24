#include "SpeedSectionTest.h"
#include "SpeedSection.h"
#include "SimpleMissionItem.h"
#include "MultiSignalSpy.h"
#include "Vehicle.h"

#include <QtTest/QTest>

void SpeedSectionTest::init()
{
    SectionTest::init();

    _speedSection = _simpleItem->speedSection();
    VERIFY_NOT_NULL(_speedSection);
    _createSpy(_speedSection, &_spySpeed);
    VERIFY_NOT_NULL(_spySpeed);
    SectionTest::_createSpy(_speedSection, &_spySection);
    VERIFY_NOT_NULL(_spySection);
}

void SpeedSectionTest::cleanup()
{
    delete _spySpeed;
    delete _spySection;
    SectionTest::cleanup();
}

void SpeedSectionTest::_createSpy(SpeedSection* speedSection, MultiSignalSpy** speedSpy)
{
    *speedSpy = nullptr;
    MultiSignalSpy* spy = new MultiSignalSpy();
    QVERIFY(spy->init(speedSection));
    *speedSpy = spy;
}

void SpeedSectionTest::_testDirty()
{
    // Check for dirty not signalled if same value
    _speedSection->setSpecifyFlightSpeed(_speedSection->specifyFlightSpeed());
    QVERIFY_NO_SIGNALS(*_spySection);
    QVERIFY(!_speedSection->dirty());
    _speedSection->flightSpeed()->setRawValue(_speedSection->flightSpeed()->rawValue());
    QVERIFY_NO_SIGNALS(*_spySection);
    QVERIFY(!_speedSection->dirty());

    // Check for no duplicate dirty signalling on change
    _speedSection->setSpecifyFlightSpeed(!_speedSection->specifyFlightSpeed());
    QVERIFY(_spySection->checkSignalByMask(_spySection->mask("dirtyChanged")));
    QVERIFY(_spySection->pullBoolFromSignal("dirtyChanged"));
    QVERIFY(_speedSection->dirty());
    _spySection->clearAllSignals();
    _speedSection->setSpecifyFlightSpeed(!_speedSection->specifyFlightSpeed());
    QVERIFY(_spySection->checkNoSignalByMask(_spySection->mask("dirtyChanged")));
    QVERIFY(_speedSection->dirty());
    _spySection->clearAllSignals();

    // Check that the dirty bit can be cleared
    _speedSection->setDirty(false);
    QVERIFY(_spySection->checkSignalByMask(_spySection->mask("dirtyChanged")));
    QVERIFY(!_spySection->pullBoolFromSignal("dirtyChanged"));
    QVERIFY(!_speedSection->dirty());
    _spySection->clearAllSignals();

    // Flight speed change should only signal if specifyFlightSpeed is set

    _speedSection->setSpecifyFlightSpeed(false);
    _speedSection->setDirty(false);
    _spySection->clearAllSignals();
    _speedSection->flightSpeed()->setRawValue(_speedSection->flightSpeed()->rawValue().toDouble() + 1);
    QVERIFY(_spySection->checkNoSignalByMask(_spySection->mask("dirtyChanged")));
    QVERIFY(!_speedSection->dirty());

    _speedSection->setSpecifyFlightSpeed(true);
    _speedSection->setDirty(false);
    _spySection->clearAllSignals();
    _speedSection->flightSpeed()->setRawValue(_speedSection->flightSpeed()->rawValue().toDouble() + 1);
    QVERIFY(_spySection->checkSignalByMask(_spySection->mask("dirtyChanged")));
    QVERIFY(_spySection->pullBoolFromSignal("dirtyChanged"));
    QVERIFY(_speedSection->dirty());
}

void SpeedSectionTest::_testSettingsAvailable()
{
    // No settings specified to start
    QVERIFY(!_speedSection->specifyFlightSpeed());
    QVERIFY(!_speedSection->settingsSpecified());

    // Check correct reaction to specifyFlightSpeed on/off

    _speedSection->setSpecifyFlightSpeed(true);
    QVERIFY(_speedSection->specifyFlightSpeed());
    QVERIFY(_speedSection->settingsSpecified());
    QVERIFY(_spySpeed->checkSignalByMask(_spySpeed->mask("specifyFlightSpeedChanged")));
    QVERIFY(_spySpeed->pullBoolFromSignal("specifyFlightSpeedChanged"));
    QVERIFY(_spySection->checkSignalByMask(_spySection->mask("settingsSpecifiedChanged")));
    QVERIFY(_spySection->pullBoolFromSignal("settingsSpecifiedChanged"));
    _spySection->clearAllSignals();
    _spySpeed->clearAllSignals();

    _speedSection->setSpecifyFlightSpeed(false);
    QVERIFY(!_speedSection->specifyFlightSpeed());
    QVERIFY(!_speedSection->settingsSpecified());
    QVERIFY(_spySpeed->checkSignalByMask(_spySpeed->mask("specifyFlightSpeedChanged")));
    QVERIFY(!_spySpeed->pullBoolFromSignal("specifyFlightSpeedChanged"));
    QVERIFY(_spySection->checkSignalByMask(_spySection->mask("settingsSpecifiedChanged")));
    QVERIFY(!_spySection->pullBoolFromSignal("settingsSpecifiedChanged"));
    _spySection->clearAllSignals();
    _spySpeed->clearAllSignals();
}

void SpeedSectionTest::_checkAvailable()
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
    VERIFY_NOT_NULL(item->speedSection());
    QVERIFY(!item->speedSection()->available());
}

void SpeedSectionTest::_testItemCount()
{
    // No settings specified to start
    QCOMPARE_EQ(_speedSection->itemCount(), 0);

    _speedSection->setSpecifyFlightSpeed(true);
    QCOMPARE_EQ(_speedSection->itemCount(), 1);
    QVERIFY(_spySection->checkSignalByMask(_spySection->mask("itemCountChanged")));
    QCOMPARE_EQ(_spySection->pullIntFromSignal("itemCountChanged"), 1);
    _spySection->clearAllSignals();
    _spySpeed->clearAllSignals();

    _speedSection->setSpecifyFlightSpeed(false);
    QCOMPARE_EQ(_speedSection->itemCount(), 0);
    QVERIFY(_spySection->checkSignalByMask(_spySection->mask("itemCountChanged")));
    QCOMPARE_EQ(_spySection->pullIntFromSignal("itemCountChanged"), 0);
    _spySection->clearAllSignals();
    _spySpeed->clearAllSignals();
}

void SpeedSectionTest::_testAppendSectionItems()
{
    int seqNum = 0;
    QList<MissionItem*> rgMissionItems;

    // No settings specified to start
    QCOMPARE_EQ(_speedSection->itemCount(), 0);

    _speedSection->appendSectionItems(rgMissionItems, this, seqNum);
    QGC_VERIFY_EMPTY(rgMissionItems);
    QCOMPARE_EQ(seqNum, 0);
    rgMissionItems.clear();

    _speedSection->setSpecifyFlightSpeed(true);
    _speedSection->appendSectionItems(rgMissionItems, this, seqNum);
    QCOMPARE_EQ(rgMissionItems.count(), 1);
    QCOMPARE_EQ(seqNum, 1);
    MissionItem expectedSpeedItem(0, MAV_CMD_DO_CHANGE_SPEED, MAV_FRAME_MISSION, _controllerVehicle->multiRotor() ? 1 : 0, _speedSection->flightSpeed()->rawValue().toDouble(), -1, 0, 0, 0, 0, true, false);
    _missionItemsEqual(*rgMissionItems[0], expectedSpeedItem);
}

void SpeedSectionTest::_testScanForSection()
{
    QVERIFY(_speedSection->available());

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
    QVERIFY(_speedSection->scanForSection(&visualItems, scanIndex));
    QCOMPARE_EQ(visualItems.count(), 0);
    QVERIFY(_speedSection->settingsSpecified());
    QVERIFY(_speedSection->specifyFlightSpeed());
    QGC_COMPARE_FLOAT(_speedSection->flightSpeed()->rawValue().toDouble(), flightSpeed);
    _speedSection->setSpecifyFlightSpeed(false);
    visualItems.clear();
    scanIndex = 0;

    // Flight speed command but incorrect settings

    simpleMissionItem = validSpeedItem;
    simpleMissionItem.setParam1(_controllerVehicle->multiRotor() ? 0 : 1);
    visualItems.append(&simpleItem);
    QVERIFY(!_speedSection->scanForSection(&visualItems, scanIndex));
    QCOMPARE_EQ(visualItems.count(), 1);
    QVERIFY(!_speedSection->settingsSpecified());
    visualItems.clear();
    scanIndex = 0;

    simpleMissionItem = validSpeedItem;
    simpleMissionItem.setParam3(50);
    visualItems.append(&simpleItem);
    QVERIFY(!_speedSection->scanForSection(&visualItems, scanIndex));
    QCOMPARE_EQ(visualItems.count(), 1);
    QVERIFY(!_speedSection->settingsSpecified());
    visualItems.clear();
    scanIndex = 0;

    simpleMissionItem = validSpeedItem;
    simpleMissionItem.setParam4(1);
    visualItems.append(&simpleItem);
    QVERIFY(!_speedSection->scanForSection(&visualItems, scanIndex));
    QCOMPARE_EQ(visualItems.count(), 1);
    QVERIFY(!_speedSection->settingsSpecified());
    visualItems.clear();
    scanIndex = 0;

    simpleMissionItem = validSpeedItem;
    simpleMissionItem.setParam5(1);
    visualItems.append(&simpleItem);
    QVERIFY(!_speedSection->scanForSection(&visualItems, scanIndex));
    QCOMPARE_EQ(visualItems.count(), 1);
    QVERIFY(!_speedSection->settingsSpecified());
    visualItems.clear();
    scanIndex = 0;

    simpleMissionItem = validSpeedItem;
    simpleMissionItem.setParam6(1);
    visualItems.append(&simpleItem);
    QVERIFY(!_speedSection->scanForSection(&visualItems, scanIndex));
    QCOMPARE_EQ(visualItems.count(), 1);
    QVERIFY(!_speedSection->settingsSpecified());
    visualItems.clear();
    scanIndex = 0;

    simpleMissionItem = validSpeedItem;
    simpleMissionItem.setParam7(1);
    visualItems.append(&simpleItem);
    QVERIFY(!_speedSection->scanForSection(&visualItems, scanIndex));
    QCOMPARE_EQ(visualItems.count(), 1);
    QVERIFY(!_speedSection->settingsSpecified());
    visualItems.clear();
    scanIndex = 0;

    // Valid item in wrong position
    MissionItem waypointMissionItem(0, MAV_CMD_NAV_WAYPOINT, MAV_FRAME_GLOBAL_RELATIVE_ALT, 0, 0, 0, 0, 0, 0, 0, true, false);
    SimpleMissionItem simpleWaypointItem(_masterController, false /* flyView */, waypointMissionItem);
    simpleMissionItem = validSpeedItem;
    visualItems.append(&simpleWaypointItem);
    visualItems.append(&simpleMissionItem);
    QVERIFY(!_speedSection->scanForSection(&visualItems, scanIndex));
    QCOMPARE_EQ(visualItems.count(), 2);
    QVERIFY(!_speedSection->settingsSpecified());
    visualItems.clear();
    scanIndex = 0;
}

void SpeedSectionTest::_testSpecifiedFlightSpeedChanged()
{
    // specifiedFlightSpeedChanged SHOULD NOT signal if flight speed is changed when specifyFlightSpeed IS NOT set
    _speedSection->setSpecifyFlightSpeed(false);
    _spySpeed->clearAllSignals();
    _speedSection->flightSpeed()->setRawValue(_speedSection->flightSpeed()->rawValue().toDouble() + 1);
    QVERIFY(_spySpeed->checkNoSignalByMask(_spySpeed->mask("specifiedFlightSpeedChanged")));

    // specifiedFlightSpeedChanged SHOULD signal if flight speed is changed when specifyFlightSpeed IS set
    _speedSection->setSpecifyFlightSpeed(true);
    _spySpeed->clearAllSignals();
    _speedSection->flightSpeed()->setRawValue(_speedSection->flightSpeed()->rawValue().toDouble() + 1);
    QVERIFY(_spySpeed->checkSignalByMask(_spySpeed->mask("specifiedFlightSpeedChanged")));
}
