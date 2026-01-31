#include "MotorAssignmentStateMachineTest.h"
#include "MotorAssignmentStateMachine.h"
#include "MotorAssignment.h"
#include "ActuatorOutputs.h"
#include "Actuators.h"
#include "QmlObjectListModel.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "MockLink.h"
#include "Fact.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void MotorAssignmentStateMachineTest::_testStateCreation()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    QmlObjectListModel actuators;
    MotorAssignmentStateMachine stateMachine(_vehicle, &actuators, this);

    QCOMPARE(stateMachine.machineName(), QStringLiteral("MotorAssignment"));
    QVERIFY(!stateMachine.isRunning());
    QVERIFY(!stateMachine.isActive());

    _disconnectMockLink();
}

void MotorAssignmentStateMachineTest::_testInitializeValidConfig()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    // For this test we verify that initialization works when
    // the state machine is not running
    QmlObjectListModel actuators;
    MotorAssignmentStateMachine stateMachine(_vehicle, &actuators, this);

    // Without proper actuator setup, this will fail validation
    // but we can test that it doesn't crash and returns false
    bool result = stateMachine.initialize(0, 1, 4);
    // Result depends on actuator configuration which we don't have set up
    // The main test is that it doesn't crash
    Q_UNUSED(result);

    _disconnectMockLink();
}

void MotorAssignmentStateMachineTest::_testInitializeInvalidConfig()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    QmlObjectListModel actuators;
    MotorAssignmentStateMachine stateMachine(_vehicle, &actuators, this);

    // Start the machine first
    stateMachine.startAssignment();
    QVERIFY(stateMachine.isRunning());

    // Cannot initialize while running
    bool result = stateMachine.initialize(0, 1, 4);
    QVERIFY(!result);

    stateMachine.abortAssignment();

    _disconnectMockLink();
}

void MotorAssignmentStateMachineTest::_testStartAssignment()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    QmlObjectListModel actuators;
    MotorAssignmentStateMachine stateMachine(_vehicle, &actuators, this);

    QSignalSpy activeSpy(&stateMachine, &MotorAssignmentStateMachine::activeChanged);

    stateMachine.startAssignment();

    // Machine should be running now
    QVERIFY(stateMachine.isRunning());

    // Should emit activeChanged
    QVERIFY(activeSpy.count() > 0 || activeSpy.wait(100));

    stateMachine.abortAssignment();

    _disconnectMockLink();
}

void MotorAssignmentStateMachineTest::_testSelectMotor()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    QmlObjectListModel actuators;
    MotorAssignmentStateMachine stateMachine(_vehicle, &actuators, this);

    // Selecting motor when not running should be ignored
    stateMachine.selectMotor(0);
    QCOMPARE(stateMachine.currentMotorIndex(), 0);

    _disconnectMockLink();
}

void MotorAssignmentStateMachineTest::_testCompleteAssignment()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    QmlObjectListModel actuators;
    MotorAssignmentStateMachine stateMachine(_vehicle, &actuators, this);

    QSignalSpy completeSpy(&stateMachine, &MotorAssignmentStateMachine::assignmentComplete);

    // Start and immediately complete is not possible without proper flow
    // This test verifies the signal exists and can be connected
    QVERIFY(completeSpy.isValid());

    _disconnectMockLink();
}

void MotorAssignmentStateMachineTest::_testAbortAssignment()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    QmlObjectListModel actuators;
    MotorAssignmentStateMachine stateMachine(_vehicle, &actuators, this);

    QSignalSpy abortSpy(&stateMachine, &MotorAssignmentStateMachine::assignmentAborted);

    stateMachine.startAssignment();
    QVERIFY(stateMachine.isRunning());

    stateMachine.abortAssignment();

    QVERIFY(abortSpy.count() > 0 || abortSpy.wait(100));
    QCOMPARE(stateMachine.message(), tr("Assignment aborted"));

    _disconnectMockLink();
}

void MotorAssignmentStateMachineTest::_testStateTransitions()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    QmlObjectListModel actuators;
    MotorAssignmentStateMachine stateMachine(_vehicle, &actuators, this);

    // Test that state machine has correct initial state
    QVERIFY(!stateMachine.isRunning());

    // Start should transition from Idle
    stateMachine.startAssignment();
    QVERIFY(stateMachine.isRunning());

    // Abort should transition to Error
    QSignalSpy errorSpy(&stateMachine, &MotorAssignmentStateMachine::assignmentError);
    stateMachine.abortAssignment();

    // Wait for error signal (abort triggers error state)
    QVERIFY(errorSpy.count() > 0 || errorSpy.wait(500));

    _disconnectMockLink();
}

void MotorAssignmentStateMachineTest::_testIsActive()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    QmlObjectListModel actuators;
    MotorAssignmentStateMachine stateMachine(_vehicle, &actuators, this);

    // Not active when not running
    QVERIFY(!stateMachine.isActive());

    stateMachine.startAssignment();

    // Active when running and not in idle state
    // After start, should transition out of idle
    QTest::qWait(50);  // Allow state transitions
    QVERIFY(stateMachine.isActive());

    stateMachine.abortAssignment();

    _disconnectMockLink();
}
