#include "PlanManagerTest.h"
#include "PlanManager.h"
#include "MissionItem.h"
#include "MultiSignalSpy.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "MockLink.h"

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

PlanManagerTest::PlanManagerTest(void)
{
    _rgPlanManagerSignals[newMissionItemsAvailableSignalIndex] = SIGNAL(newMissionItemsAvailable(bool));
    _rgPlanManagerSignals[inProgressChangedSignalIndex] = SIGNAL(inProgressChanged(bool));
    _rgPlanManagerSignals[errorSignalIndex] = SIGNAL(error(int,QString));
    _rgPlanManagerSignals[removeAllCompleteSignalIndex] = SIGNAL(removeAllComplete(bool));
    _rgPlanManagerSignals[sendCompleteSignalIndex] = SIGNAL(sendComplete(bool));
}

void PlanManagerTest::cleanup(void)
{
    delete _multiSpyPlanManager;
    _multiSpyPlanManager = nullptr;
    _planManager = nullptr;

    UnitTest::cleanup();
}

void PlanManagerTest::_initForFirmwareType(MAV_AUTOPILOT firmwareType)
{
    _connectMockLink(firmwareType);
    QVERIFY(_vehicle);

    _planManager = new PlanManager(_vehicle, MAV_MISSION_TYPE_MISSION);
    _setupSignalSpy();
}

void PlanManagerTest::_setupSignalSpy()
{
    _multiSpyPlanManager = new MultiSignalSpy();
    QCOMPARE(_multiSpyPlanManager->init(_planManager, _rgPlanManagerSignals, _cPlanManagerSignals), true);
}

void PlanManagerTest::_checkInProgressValues(bool inProgress)
{
    QCOMPARE(_planManager->inProgress(), inProgress);
}

void PlanManagerTest::_testInitialState()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);

    QVERIFY(!_planManager->inProgress());
    QCOMPARE(_planManager->missionItems().count(), 0);
    QCOMPARE(_planManager->currentIndex(), -1);
    QCOMPARE(_planManager->lastCurrentIndex(), -1);

    _disconnectMockLink();
}

void PlanManagerTest::_testInProgressTracking()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);

    QVERIFY(!_planManager->inProgress());

    _planManager->loadFromVehicle();

    QVERIFY(_planManager->inProgress());
    QVERIFY(_multiSpyPlanManager->checkSignalByMask(inProgressChangedSignalMask));

    _multiSpyPlanManager->clearAllSignals();

    _multiSpyPlanManager->waitForSignalByIndex(inProgressChangedSignalIndex, _planManagerSignalWaitTime);
    QVERIFY(!_planManager->inProgress());

    _disconnectMockLink();
}

void PlanManagerTest::_testLoadFromVehicle()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);

    _planManager->loadFromVehicle();

    QVERIFY(_planManager->inProgress());
    QVERIFY(_multiSpyPlanManager->checkSignalByMask(inProgressChangedSignalMask));
    _checkInProgressValues(true);

    _multiSpyPlanManager->clearAllSignals();

    _multiSpyPlanManager->waitForSignalByIndex(inProgressChangedSignalIndex, _planManagerSignalWaitTime);
    QVERIFY(_multiSpyPlanManager->checkSignalByMask(newMissionItemsAvailableSignalMask | inProgressChangedSignalMask));
    _checkInProgressValues(false);

    _disconnectMockLink();
}

void PlanManagerTest::_testLoadFromVehicleCancel()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);

    _planManager->loadFromVehicle();
    QVERIFY(_planManager->inProgress());

    _multiSpyPlanManager->clearAllSignals();

    _multiSpyPlanManager->waitForSignalByIndex(inProgressChangedSignalIndex, _planManagerSignalWaitTime);
    QVERIFY(!_planManager->inProgress());

    _disconnectMockLink();
}

void PlanManagerTest::_testWriteMissionItems()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);

    QList<MissionItem*> missionItems;

    MissionItem* homeItem = new MissionItem(nullptr, this);
    homeItem->setCommand(MAV_CMD_NAV_WAYPOINT);
    homeItem->setParam5(47.3769);
    homeItem->setParam6(8.549444);
    homeItem->setParam7(0);
    homeItem->setSequenceNumber(0);
    missionItems.append(homeItem);

    MissionItem* waypointItem = new MissionItem(this);
    waypointItem->setCommand(MAV_CMD_NAV_WAYPOINT);
    waypointItem->setFrame(MAV_FRAME_GLOBAL_RELATIVE_ALT);
    waypointItem->setParam5(47.3770);
    waypointItem->setParam6(8.5495);
    waypointItem->setParam7(100);
    waypointItem->setSequenceNumber(1);
    waypointItem->setAutoContinue(true);
    missionItems.append(waypointItem);

    _planManager->writeMissionItems(missionItems);

    QVERIFY(_planManager->inProgress());
    QVERIFY(_multiSpyPlanManager->checkSignalByMask(inProgressChangedSignalMask));
    _checkInProgressValues(true);

    _multiSpyPlanManager->clearAllSignals();

    _multiSpyPlanManager->waitForSignalByIndex(sendCompleteSignalIndex, _planManagerSignalWaitTime);
    QVERIFY(_multiSpyPlanManager->checkSignalByMask(inProgressChangedSignalMask | sendCompleteSignalMask));
    _checkInProgressValues(false);

    QSignalSpy* spy = _multiSpyPlanManager->getSpyByIndex(sendCompleteSignalIndex);
    QList<QVariant> signalArgs = spy->takeFirst();
    QCOMPARE(signalArgs.count(), 1);
    QCOMPARE(signalArgs[0].toBool(), false);

    _disconnectMockLink();
}

void PlanManagerTest::_testWriteMissionItemsEmpty()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);

    QList<MissionItem*> missionItems;

    _planManager->writeMissionItems(missionItems);

    QVERIFY(_planManager->inProgress());

    _multiSpyPlanManager->clearAllSignals();

    _multiSpyPlanManager->waitForSignalByIndex(sendCompleteSignalIndex, _planManagerSignalWaitTime);
    QVERIFY(!_planManager->inProgress());

    _disconnectMockLink();
}

void PlanManagerTest::_testRemoveAll()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);

    _planManager->removeAll();

    QVERIFY(_planManager->inProgress());
    QVERIFY(_multiSpyPlanManager->checkSignalByMask(inProgressChangedSignalMask));
    _checkInProgressValues(true);

    _multiSpyPlanManager->clearAllSignals();

    _multiSpyPlanManager->waitForSignalByIndex(removeAllCompleteSignalIndex, _planManagerSignalWaitTime);
    QVERIFY(_multiSpyPlanManager->checkSignalByMask(inProgressChangedSignalMask | removeAllCompleteSignalMask));
    _checkInProgressValues(false);

    QSignalSpy* spy = _multiSpyPlanManager->getSpyByIndex(removeAllCompleteSignalIndex);
    QList<QVariant> signalArgs = spy->takeFirst();
    QCOMPARE(signalArgs.count(), 1);
    QCOMPARE(signalArgs[0].toBool(), false);

    _disconnectMockLink();
}

void PlanManagerTest::_testRemoveAllNoItems()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);

    QCOMPARE(_planManager->missionItems().count(), 0);

    _planManager->removeAll();

    QVERIFY(_planManager->inProgress());

    _multiSpyPlanManager->clearAllSignals();

    _multiSpyPlanManager->waitForSignalByIndex(removeAllCompleteSignalIndex, _planManagerSignalWaitTime);
    QVERIFY(!_planManager->inProgress());

    _disconnectMockLink();
}

void PlanManagerTest::_testConcurrentOperationsPrevented()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);

    _planManager->loadFromVehicle();
    QVERIFY(_planManager->inProgress());

    _multiSpyPlanManager->clearAllSignals();

    _planManager->loadFromVehicle();
    QVERIFY(_multiSpyPlanManager->getSpyByIndex(inProgressChangedSignalIndex)->count() == 0);

    QList<MissionItem*> missionItems;
    MissionItem* item = new MissionItem(this);
    item->setCommand(MAV_CMD_NAV_WAYPOINT);
    item->setSequenceNumber(0);
    missionItems.append(item);

    _planManager->writeMissionItems(missionItems);
    QVERIFY(_multiSpyPlanManager->getSpyByIndex(sendCompleteSignalIndex)->count() == 0);

    _planManager->removeAll();
    QVERIFY(_multiSpyPlanManager->getSpyByIndex(removeAllCompleteSignalIndex)->count() == 0);

    _multiSpyPlanManager->waitForSignalByIndex(inProgressChangedSignalIndex, _planManagerSignalWaitTime);
    QVERIFY(!_planManager->inProgress());

    _disconnectMockLink();
}

void PlanManagerTest::_testTimeoutRetry()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);

    _mockLink->setMissionItemFailureMode(MockLinkMissionItemHandler::FailReadRequestListFirstResponse, MAV_MISSION_ERROR);

    _planManager->loadFromVehicle();
    QVERIFY(_planManager->inProgress());

    _multiSpyPlanManager->clearAllSignals();

    _multiSpyPlanManager->waitForSignalByIndex(inProgressChangedSignalIndex, _planManagerSignalWaitTime);
    QVERIFY(_multiSpyPlanManager->checkSignalByMask(newMissionItemsAvailableSignalMask | inProgressChangedSignalMask));
    _checkInProgressValues(false);

    _disconnectMockLink();
}

void PlanManagerTest::_testMaxRetryExceeded()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);

    _mockLink->setMissionItemFailureMode(MockLinkMissionItemHandler::FailReadRequestListNoResponse, MAV_MISSION_ERROR);

    _planManager->loadFromVehicle();
    QVERIFY(_planManager->inProgress());

    _multiSpyPlanManager->clearAllSignals();

    _multiSpyPlanManager->waitForSignalByIndex(inProgressChangedSignalIndex, _planManagerSignalWaitTime);
    QVERIFY(_multiSpyPlanManager->checkSignalByMask(newMissionItemsAvailableSignalMask | inProgressChangedSignalMask | errorSignalMask));
    _checkInProgressValues(false);

    QSignalSpy* spy = _multiSpyPlanManager->getSpyByIndex(errorSignalIndex);
    QList<QVariant> signalArgs = spy->takeFirst();
    QCOMPARE(signalArgs.count(), 2);
    QCOMPARE(signalArgs[0].toInt(), static_cast<int>(PlanManager::MaxRetryExceeded));

    _disconnectMockLink();
}

void PlanManagerTest::_testMissionTypeMismatch()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);

    PlanManager* fenceManager = new PlanManager(_vehicle, MAV_MISSION_TYPE_FENCE);

    MultiSignalSpy* fenceSpy = new MultiSignalSpy();
    QCOMPARE(fenceSpy->init(fenceManager, _rgPlanManagerSignals, _cPlanManagerSignals), true);

    fenceManager->loadFromVehicle();
    QVERIFY(fenceManager->inProgress());

    fenceSpy->clearAllSignals();

    fenceSpy->waitForSignalByIndex(inProgressChangedSignalIndex, _planManagerSignalWaitTime);
    QVERIFY(!fenceManager->inProgress());

    delete fenceSpy;
    delete fenceManager;

    _disconnectMockLink();
}
