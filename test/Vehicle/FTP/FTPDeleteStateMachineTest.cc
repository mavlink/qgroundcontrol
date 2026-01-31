#include "FTPDeleteStateMachineTest.h"
#include "FTPDeleteStateMachine.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "MockLink.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void FTPDeleteStateMachineTest::_testCreation()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPDeleteStateMachine stateMachine(_vehicle, this);

    QVERIFY(!stateMachine.isOperationInProgress());
    QCOMPARE(stateMachine.componentId(), static_cast<uint8_t>(MAV_COMP_ID_AUTOPILOT1));

    _disconnectMockLink();
}

void FTPDeleteStateMachineTest::_testMachineName()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPDeleteStateMachine stateMachine(_vehicle, this);

    QCOMPARE(stateMachine.machineName(), QStringLiteral("FTPDelete"));

    _disconnectMockLink();
}

void FTPDeleteStateMachineTest::_testStart()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPDeleteStateMachine stateMachine(_vehicle, this);

    QSignalSpy completeSpy(&stateMachine, &FTPDeleteStateMachine::deleteComplete);

    bool started = stateMachine.deleteFile(MAV_COMP_ID_AUTOPILOT1, "/test/file.txt");
    QVERIFY(started);
    QVERIFY(stateMachine.isOperationInProgress());

    // Clean up - cancel the operation
    stateMachine.cancel();

    _disconnectMockLink();
}

void FTPDeleteStateMachineTest::_testCancel()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPDeleteStateMachine stateMachine(_vehicle, this);

    QSignalSpy completeSpy(&stateMachine, &FTPDeleteStateMachine::deleteComplete);

    stateMachine.deleteFile(MAV_COMP_ID_AUTOPILOT1, "/test/file.txt");
    QVERIFY(stateMachine.isOperationInProgress());

    stateMachine.cancel();

    // Should have received completion signal with abort message
    QVERIFY(completeSpy.count() > 0 || completeSpy.wait(100));
    QVERIFY(!stateMachine.isOperationInProgress());

    // Verify the signal arguments
    QList<QVariant> arguments = completeSpy.takeFirst();
    QCOMPARE(arguments[0].toString(), QStringLiteral("/test/file.txt"));
    QCOMPARE(arguments[1].toString(), QStringLiteral("Aborted"));

    _disconnectMockLink();
}

void FTPDeleteStateMachineTest::_testDoubleStartPrevented()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPDeleteStateMachine stateMachine(_vehicle, this);

    // Start first operation
    bool started1 = stateMachine.deleteFile(MAV_COMP_ID_AUTOPILOT1, "/test/file1.txt");
    QVERIFY(started1);
    QVERIFY(stateMachine.isOperationInProgress());

    // Second start should fail
    bool started2 = stateMachine.deleteFile(MAV_COMP_ID_AUTOPILOT1, "/test/file2.txt");
    QVERIFY(!started2);

    // Clean up
    stateMachine.cancel();

    _disconnectMockLink();
}

void FTPDeleteStateMachineTest::_testComponentIdHandling()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPDeleteStateMachine stateMachine(_vehicle, this);

    // MAV_COMP_ID_ALL should be converted to MAV_COMP_ID_AUTOPILOT1
    stateMachine.deleteFile(MAV_COMP_ID_ALL, "/test/file.txt");
    QCOMPARE(stateMachine.componentId(), static_cast<uint8_t>(MAV_COMP_ID_AUTOPILOT1));

    stateMachine.cancel();

    _disconnectMockLink();
}
