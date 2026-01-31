#include "ActuatorTestingStateMachineTest.h"
#include "ActuatorTestingStateMachine.h"
#include "ActuatorTesting.h"
#include "QmlObjectListModel.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "MockLink.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void ActuatorTestingStateMachineTest::_testStateCreation()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    QmlObjectListModel actuators;
    ActuatorTestingStateMachine stateMachine(_vehicle, &actuators, this);

    QCOMPARE(stateMachine.machineName(), QStringLiteral("ActuatorTesting"));
    QVERIFY(!stateMachine.isTestingActive());
    QVERIFY(!stateMachine.hadFailure());

    _disconnectMockLink();
}

void ActuatorTestingStateMachineTest::_testActivateDeactivate()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    QmlObjectListModel actuators;
    ActuatorTestingStateMachine stateMachine(_vehicle, &actuators, this);

    QVERIFY(!stateMachine.isTestingActive());

    stateMachine.setActive(true);
    QVERIFY(stateMachine.isTestingActive());
    QVERIFY(stateMachine.isRunning());

    stateMachine.setActive(false);
    QVERIFY(!stateMachine.isTestingActive());

    _disconnectMockLink();
}

void ActuatorTestingStateMachineTest::_testSetChannelTo()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    QmlObjectListModel actuators;

    // Add a test actuator
    auto* actuator = new ActuatorTesting::Actuator(&actuators, "Motor1", 0.0f, 1.0f, 0.0f, 1, true);
    actuators.append(actuator);

    ActuatorTestingStateMachine stateMachine(_vehicle, &actuators, this);
    stateMachine.resetStates();

    // Setting channel when not active should be ignored
    stateMachine.setChannelTo(0, 0.5f);
    QCOMPARE(stateMachine.actuatorState(0), ActuatorTestingStateMachine::ActuatorState::NotActive);

    // Activate and try again
    stateMachine.setActive(true);
    QTest::qWait(50);

    QSignalSpy stateChangeSpy(&stateMachine, &ActuatorTestingStateMachine::actuatorStateChanged);

    stateMachine.setChannelTo(0, 0.5f);
    QCOMPARE(stateMachine.actuatorState(0), ActuatorTestingStateMachine::ActuatorState::Active);
    QVERIFY(stateChangeSpy.count() > 0);

    stateMachine.setActive(false);

    _disconnectMockLink();
}

void ActuatorTestingStateMachineTest::_testStopControl()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    QmlObjectListModel actuators;
    auto* actuator = new ActuatorTesting::Actuator(&actuators, "Motor1", 0.0f, 1.0f, 0.0f, 1, true);
    actuators.append(actuator);

    ActuatorTestingStateMachine stateMachine(_vehicle, &actuators, this);
    stateMachine.resetStates();
    stateMachine.setActive(true);
    QTest::qWait(50);

    // Set channel active first
    stateMachine.setChannelTo(0, 0.5f);
    QCOMPARE(stateMachine.actuatorState(0), ActuatorTestingStateMachine::ActuatorState::Active);

    // Stop control
    stateMachine.stopControl(0);
    QCOMPARE(stateMachine.actuatorState(0), ActuatorTestingStateMachine::ActuatorState::StopRequest);

    stateMachine.setActive(false);

    _disconnectMockLink();
}

void ActuatorTestingStateMachineTest::_testStopAllControl()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    QmlObjectListModel actuators;
    auto* actuator1 = new ActuatorTesting::Actuator(&actuators, "Motor1", 0.0f, 1.0f, 0.0f, 1, true);
    auto* actuator2 = new ActuatorTesting::Actuator(&actuators, "Motor2", 0.0f, 1.0f, 0.0f, 2, true);
    actuators.append(actuator1);
    actuators.append(actuator2);

    ActuatorTestingStateMachine stateMachine(_vehicle, &actuators, this);
    stateMachine.resetStates();
    stateMachine.setActive(true);
    QTest::qWait(50);

    // Set both channels active
    stateMachine.setChannelTo(0, 0.5f);
    stateMachine.setChannelTo(1, 0.7f);
    QCOMPARE(stateMachine.actuatorState(0), ActuatorTestingStateMachine::ActuatorState::Active);
    QCOMPARE(stateMachine.actuatorState(1), ActuatorTestingStateMachine::ActuatorState::Active);

    // Stop all (-1)
    stateMachine.stopControl(-1);
    QCOMPARE(stateMachine.actuatorState(0), ActuatorTestingStateMachine::ActuatorState::StopRequest);
    QCOMPARE(stateMachine.actuatorState(1), ActuatorTestingStateMachine::ActuatorState::StopRequest);

    stateMachine.setActive(false);

    _disconnectMockLink();
}

void ActuatorTestingStateMachineTest::_testActuatorStateTracking()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    QmlObjectListModel actuators;
    auto* actuator = new ActuatorTesting::Actuator(&actuators, "Motor1", 0.0f, 1.0f, 0.0f, 1, true);
    actuators.append(actuator);

    ActuatorTestingStateMachine stateMachine(_vehicle, &actuators, this);
    stateMachine.resetStates();

    // Initial state should be NotActive
    QCOMPARE(stateMachine.actuatorState(0), ActuatorTestingStateMachine::ActuatorState::NotActive);

    // Invalid index should return NotActive
    QCOMPARE(stateMachine.actuatorState(99), ActuatorTestingStateMachine::ActuatorState::NotActive);
    QCOMPARE(stateMachine.actuatorState(-1), ActuatorTestingStateMachine::ActuatorState::NotActive);

    _disconnectMockLink();
}

void ActuatorTestingStateMachineTest::_testHasActiveActuators()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    QmlObjectListModel actuators;
    auto* actuator = new ActuatorTesting::Actuator(&actuators, "Motor1", 0.0f, 1.0f, 0.0f, 1, true);
    actuators.append(actuator);

    ActuatorTestingStateMachine stateMachine(_vehicle, &actuators, this);
    stateMachine.resetStates();

    QVERIFY(!stateMachine.hasActiveActuators());

    stateMachine.setActive(true);
    QTest::qWait(50);

    stateMachine.setChannelTo(0, 0.5f);
    QVERIFY(stateMachine.hasActiveActuators());

    stateMachine.setActive(false);

    _disconnectMockLink();
}

void ActuatorTestingStateMachineTest::_testResetStates()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    QmlObjectListModel actuators;
    auto* actuator1 = new ActuatorTesting::Actuator(&actuators, "Motor1", 0.0f, 1.0f, 0.0f, 1, true);
    auto* actuator2 = new ActuatorTesting::Actuator(&actuators, "Motor2", 0.0f, 1.0f, 0.0f, 2, true);
    actuators.append(actuator1);
    actuators.append(actuator2);

    ActuatorTestingStateMachine stateMachine(_vehicle, &actuators, this);

    // Before reset, no states
    QCOMPARE(stateMachine.actuatorState(0), ActuatorTestingStateMachine::ActuatorState::NotActive);

    stateMachine.resetStates();

    // After reset, states should be initialized
    QCOMPARE(stateMachine.actuatorState(0), ActuatorTestingStateMachine::ActuatorState::NotActive);
    QCOMPARE(stateMachine.actuatorState(1), ActuatorTestingStateMachine::ActuatorState::NotActive);

    _disconnectMockLink();
}
