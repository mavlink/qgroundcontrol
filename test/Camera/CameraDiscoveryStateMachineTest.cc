#include "CameraDiscoveryStateMachineTest.h"
#include "CameraDiscoveryStateMachine.h"
#include "QGCCameraManager.h"
#include "Vehicle.h"
#include "MultiVehicleManager.h"
#include "MockLink.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void CameraDiscoveryStateMachineTest::_testStateCreation()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    QGCCameraManager manager(_vehicle);
    CameraDiscoveryStateMachine stateMachine(&manager, MAV_COMP_ID_CAMERA, _vehicle, this);

    QCOMPARE(stateMachine.machineName(), QStringLiteral("CameraDiscovery"));
    QCOMPARE(stateMachine.compId(), MAV_COMP_ID_CAMERA);
    QVERIFY(!stateMachine.isInfoReceived());
    QVERIFY(!stateMachine.isTimedOut());

    _disconnectMockLink();
}

void CameraDiscoveryStateMachineTest::_testStartDiscovery()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    QGCCameraManager manager(_vehicle);
    CameraDiscoveryStateMachine stateMachine(&manager, MAV_COMP_ID_CAMERA, _vehicle, this);

    QSignalSpy completeSpy(&stateMachine, &CameraDiscoveryStateMachine::discoveryComplete);
    QSignalSpy failedSpy(&stateMachine, &CameraDiscoveryStateMachine::discoveryFailed);

    stateMachine.startDiscovery();

    QVERIFY(stateMachine.isRunning());
    QVERIFY(!stateMachine.isInfoReceived());

    // No signals yet
    QCOMPARE(completeSpy.count(), 0);
    QCOMPARE(failedSpy.count(), 0);

    _disconnectMockLink();
}

void CameraDiscoveryStateMachineTest::_testInfoReceived()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    QGCCameraManager manager(_vehicle);
    CameraDiscoveryStateMachine stateMachine(&manager, MAV_COMP_ID_CAMERA, _vehicle, this);

    QSignalSpy completeSpy(&stateMachine, &CameraDiscoveryStateMachine::discoveryComplete);

    stateMachine.startDiscovery();
    QVERIFY(stateMachine.isRunning());

    // Simulate receiving camera info
    stateMachine.handleInfoReceived();

    // Wait for state machine to process
    QTest::qWait(50);

    QVERIFY(stateMachine.isInfoReceived());
    QCOMPARE(completeSpy.count(), 1);

    // Verify the signal carried the correct compId
    QList<QVariant> arguments = completeSpy.takeFirst();
    QCOMPARE(arguments.at(0).toUInt(), static_cast<uint>(MAV_COMP_ID_CAMERA));

    _disconnectMockLink();
}

void CameraDiscoveryStateMachineTest::_testRequestFailed()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    QGCCameraManager manager(_vehicle);
    CameraDiscoveryStateMachine stateMachine(&manager, MAV_COMP_ID_CAMERA, _vehicle, this);

    stateMachine.startDiscovery();
    QVERIFY(stateMachine.isRunning());

    // Simulate a few failed requests (but not max)
    stateMachine.handleRequestFailed();
    QTest::qWait(50);

    // Should still be running (retrying)
    QVERIFY(!stateMachine.isInfoReceived());

    stateMachine.handleRequestFailed();
    QTest::qWait(50);

    // Still not at max retries
    QVERIFY(!stateMachine.isInfoReceived());

    _disconnectMockLink();
}

void CameraDiscoveryStateMachineTest::_testMaxRetries()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    QGCCameraManager manager(_vehicle);
    CameraDiscoveryStateMachine stateMachine(&manager, MAV_COMP_ID_CAMERA, _vehicle, this);

    QSignalSpy failedSpy(&stateMachine, &CameraDiscoveryStateMachine::discoveryFailed);

    stateMachine.startDiscovery();
    QVERIFY(stateMachine.isRunning());

    // Simulate max retries worth of failures (10)
    for (int i = 0; i < 10; i++) {
        stateMachine.handleRequestFailed();
        QTest::qWait(10);
    }

    // Wait for state machine to process
    QTest::qWait(100);

    QVERIFY(!stateMachine.isInfoReceived());
    QCOMPARE(failedSpy.count(), 1);

    // Verify the signal carried the correct compId
    QList<QVariant> arguments = failedSpy.takeFirst();
    QCOMPARE(arguments.at(0).toUInt(), static_cast<uint>(MAV_COMP_ID_CAMERA));

    _disconnectMockLink();
}

void CameraDiscoveryStateMachineTest::_testHeartbeatTracking()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    QGCCameraManager manager(_vehicle);
    CameraDiscoveryStateMachine stateMachine(&manager, MAV_COMP_ID_CAMERA, _vehicle, this);

    // Start discovery so we're tracking heartbeats
    stateMachine.startDiscovery();
    stateMachine.handleInfoReceived();
    QTest::qWait(50);

    QVERIFY(stateMachine.isInfoReceived());

    // Time since last heartbeat should be small
    QVERIFY(stateMachine.timeSinceLastHeartbeat() < 1000);

    // Simulate heartbeat
    stateMachine.handleHeartbeat();

    // Time should reset
    QVERIFY(stateMachine.timeSinceLastHeartbeat() < 100);

    _disconnectMockLink();
}

void CameraDiscoveryStateMachineTest::_testTimeout()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    QGCCameraManager manager(_vehicle);
    CameraDiscoveryStateMachine stateMachine(&manager, MAV_COMP_ID_CAMERA, _vehicle, this);

    stateMachine.startDiscovery();
    stateMachine.handleInfoReceived();
    QTest::qWait(50);

    QVERIFY(stateMachine.isInfoReceived());

    // Initially not timed out
    QVERIFY(!stateMachine.isTimedOut());

    // Wait for timeout (5 seconds + buffer)
    QTest::qWait(5500);

    // Should now be timed out
    QVERIFY(stateMachine.isTimedOut());

    // Heartbeat should reset timeout
    stateMachine.handleHeartbeat();
    QVERIFY(!stateMachine.isTimedOut());

    _disconnectMockLink();
}
