#include "PlanManagerStateMachineTest.h"
#include "PlanManagerStateMachine.h"
#include "PlanManager.h"
#include "MissionItem.h"
#include "MultiSignalSpy.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "MockLink.h"

#include <QtCore/QCoreApplication>
#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

PlanManagerStateMachineTest::PlanManagerStateMachineTest(void)
{
    _rgSignals[transactionCompleteSignalIndex] = SIGNAL(transactionComplete(bool));
    _rgSignals[readCompleteSignalIndex] = SIGNAL(readComplete(bool));
    _rgSignals[writeCompleteSignalIndex] = SIGNAL(writeComplete(bool));
    _rgSignals[removeAllCompleteSignalIndex] = SIGNAL(removeAllComplete(bool));
    _rgSignals[progressChangedSignalIndex] = SIGNAL(progressChanged(double));
    _rgSignals[errorOccurredSignalIndex] = SIGNAL(errorOccurred(int,QString));
}

void PlanManagerStateMachineTest::cleanup(void)
{
    delete _multiSpy;
    _multiSpy = nullptr;

    delete _stateMachine;
    _stateMachine = nullptr;

    delete _planManager;
    _planManager = nullptr;

    UnitTest::cleanup();
}

void PlanManagerStateMachineTest::_initForFirmwareType(MAV_AUTOPILOT firmwareType)
{
    _connectMockLink(firmwareType);
    QVERIFY(_vehicle);

    _planManager = new PlanManager(_vehicle, MAV_MISSION_TYPE_MISSION);
    _stateMachine = new PlanManagerStateMachine(_planManager, this);
    _setupSignalSpy();
}

void PlanManagerStateMachineTest::_setupSignalSpy()
{
    _multiSpy = new MultiSignalSpy();
    QCOMPARE(_multiSpy->init(_stateMachine, _rgSignals, _cSignals), true);
}

void PlanManagerStateMachineTest::_testInitialState()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);

    QVERIFY(!_stateMachine->inProgress());
    QVERIFY(!_stateMachine->isRunning());
    QCOMPARE(_stateMachine->retryCount(), 0);

    _disconnectMockLink();
}

void PlanManagerStateMachineTest::_testStartRead()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);

    QVERIFY(!_stateMachine->inProgress());

    _stateMachine->startRead();
    QCoreApplication::processEvents();

    QVERIFY(_stateMachine->inProgress());
    QVERIFY(_stateMachine->isRunning());

    // Wait for transaction to complete (timeout or success)
    _multiSpy->waitForSignalByIndex(transactionCompleteSignalIndex, _signalWaitTime);

    QVERIFY(!_stateMachine->inProgress());

    _disconnectMockLink();
}

void PlanManagerStateMachineTest::_testStartWrite()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);

    QList<MissionItem*> missionItems;

    MissionItem* waypointItem = new MissionItem(this);
    waypointItem->setCommand(MAV_CMD_NAV_WAYPOINT);
    waypointItem->setFrame(MAV_FRAME_GLOBAL_RELATIVE_ALT);
    waypointItem->setParam5(47.3770);
    waypointItem->setParam6(8.5495);
    waypointItem->setParam7(100);
    waypointItem->setSequenceNumber(0);
    waypointItem->setAutoContinue(true);
    missionItems.append(waypointItem);

    QVERIFY(!_stateMachine->inProgress());

    _stateMachine->startWrite(missionItems);
    QCoreApplication::processEvents();

    QVERIFY(_stateMachine->inProgress());

    // Wait for transaction to complete
    _multiSpy->waitForSignalByIndex(transactionCompleteSignalIndex, _signalWaitTime);

    QVERIFY(!_stateMachine->inProgress());

    _disconnectMockLink();
}

void PlanManagerStateMachineTest::_testStartRemoveAll()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);

    QVERIFY(!_stateMachine->inProgress());

    _stateMachine->startRemoveAll();
    QCoreApplication::processEvents();

    QVERIFY(_stateMachine->inProgress());

    // Wait for transaction to complete
    _multiSpy->waitForSignalByIndex(transactionCompleteSignalIndex, _signalWaitTime);

    QVERIFY(!_stateMachine->inProgress());
    QVERIFY(_multiSpy->checkSignalByMask(removeAllCompleteSignalMask | transactionCompleteSignalMask));

    _disconnectMockLink();
}

void PlanManagerStateMachineTest::_testCancelTransaction()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);

    _stateMachine->startRead();
    QCoreApplication::processEvents();

    QVERIFY(_stateMachine->inProgress());

    _stateMachine->cancel();
    QCoreApplication::processEvents();

    // Transaction should complete (possibly with timeout)
    _multiSpy->waitForSignalByIndex(transactionCompleteSignalIndex, _signalWaitTime);

    _disconnectMockLink();
}

void PlanManagerStateMachineTest::_testInProgressTracking()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);

    QVERIFY(!_stateMachine->inProgress());

    _stateMachine->startRead();
    QCoreApplication::processEvents();

    QVERIFY(_stateMachine->inProgress());

    _multiSpy->waitForSignalByIndex(transactionCompleteSignalIndex, _signalWaitTime);

    QVERIFY(!_stateMachine->inProgress());

    _disconnectMockLink();
}

void PlanManagerStateMachineTest::_testConcurrentOperationsPrevented()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);

    _stateMachine->startRead();
    QCoreApplication::processEvents();

    QVERIFY(_stateMachine->inProgress());

    // Try to start another operation while one is in progress
    QSignalSpy progressSpy(_stateMachine, &PlanManagerStateMachine::progressChanged);

    _stateMachine->startRead();
    QCoreApplication::processEvents();

    // Should not start a new transaction - no new progress signals
    QCOMPARE(progressSpy.count(), 0);

    // Wait for original transaction to complete
    _multiSpy->waitForSignalByIndex(transactionCompleteSignalIndex, _signalWaitTime);

    _disconnectMockLink();
}

void PlanManagerStateMachineTest::_testReadStateTransitions()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);

    QSignalSpy progressSpy(_stateMachine, &PlanManagerStateMachine::progressChanged);

    _stateMachine->startRead();
    QCoreApplication::processEvents();

    QVERIFY(_stateMachine->inProgress());

    // Wait for read to complete
    _multiSpy->waitForSignalByIndex(readCompleteSignalIndex, _signalWaitTime);

    QVERIFY(_multiSpy->checkSignalByMask(readCompleteSignalMask));

    _disconnectMockLink();
}

void PlanManagerStateMachineTest::_testWriteStateTransitions()
{
    _initForFirmwareType(MAV_AUTOPILOT_PX4);

    QList<MissionItem*> missionItems;

    MissionItem* item = new MissionItem(this);
    item->setCommand(MAV_CMD_NAV_WAYPOINT);
    item->setSequenceNumber(0);
    missionItems.append(item);

    _stateMachine->startWrite(missionItems);
    QCoreApplication::processEvents();

    QVERIFY(_stateMachine->inProgress());

    // Wait for write to complete
    _multiSpy->waitForSignalByIndex(writeCompleteSignalIndex, _signalWaitTime);

    QVERIFY(_multiSpy->checkSignalByMask(writeCompleteSignalMask));

    _disconnectMockLink();
}
