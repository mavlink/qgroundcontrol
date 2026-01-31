#include "FTPListDirectoryStateMachineTest.h"
#include "FTPListDirectoryStateMachine.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "MockLink.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void FTPListDirectoryStateMachineTest::_testCreation()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPListDirectoryStateMachine stateMachine(_vehicle, this);

    QVERIFY(!stateMachine.isOperationInProgress());
    QCOMPARE(stateMachine.componentId(), static_cast<uint8_t>(MAV_COMP_ID_AUTOPILOT1));

    _disconnectMockLink();
}

void FTPListDirectoryStateMachineTest::_testMachineName()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPListDirectoryStateMachine stateMachine(_vehicle, this);

    QCOMPARE(stateMachine.machineName(), QStringLiteral("FTPListDirectory"));

    _disconnectMockLink();
}

void FTPListDirectoryStateMachineTest::_testStart()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPListDirectoryStateMachine stateMachine(_vehicle, this);

    bool started = stateMachine.listDirectory(MAV_COMP_ID_AUTOPILOT1, "/");
    QVERIFY(started);
    QVERIFY(stateMachine.isOperationInProgress());

    // Clean up
    stateMachine.cancel();

    _disconnectMockLink();
}

void FTPListDirectoryStateMachineTest::_testCancel()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPListDirectoryStateMachine stateMachine(_vehicle, this);

    QSignalSpy completeSpy(&stateMachine, &FTPListDirectoryStateMachine::listDirectoryComplete);

    stateMachine.listDirectory(MAV_COMP_ID_AUTOPILOT1, "/");
    QVERIFY(stateMachine.isOperationInProgress());

    stateMachine.cancel();

    QVERIFY(completeSpy.count() > 0 || completeSpy.wait(100));
    QVERIFY(!stateMachine.isOperationInProgress());

    // Verify the signal arguments
    QList<QVariant> arguments = completeSpy.takeFirst();
    QVERIFY(arguments[0].toStringList().isEmpty());
    QCOMPARE(arguments[1].toString(), QStringLiteral("Aborted"));

    _disconnectMockLink();
}

void FTPListDirectoryStateMachineTest::_testDoubleStartPrevented()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPListDirectoryStateMachine stateMachine(_vehicle, this);

    // Start first operation
    bool started1 = stateMachine.listDirectory(MAV_COMP_ID_AUTOPILOT1, "/");
    QVERIFY(started1);
    QVERIFY(stateMachine.isOperationInProgress());

    // Second start should fail
    bool started2 = stateMachine.listDirectory(MAV_COMP_ID_AUTOPILOT1, "/other");
    QVERIFY(!started2);

    // Clean up
    stateMachine.cancel();

    _disconnectMockLink();
}

void FTPListDirectoryStateMachineTest::_testComponentIdHandling()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    FTPListDirectoryStateMachine stateMachine(_vehicle, this);

    // MAV_COMP_ID_ALL should be converted to MAV_COMP_ID_AUTOPILOT1
    stateMachine.listDirectory(MAV_COMP_ID_ALL, "/");
    QCOMPARE(stateMachine.componentId(), static_cast<uint8_t>(MAV_COMP_ID_AUTOPILOT1));

    stateMachine.cancel();

    _disconnectMockLink();
}
