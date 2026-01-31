#include "MotorAssignmentTest.h"
#include "MotorAssignment.h"
#include "QmlObjectListModel.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "MockLink.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void MotorAssignmentTest::_testCreation()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    QmlObjectListModel actuators;
    MotorAssignment assignment(this, _vehicle, &actuators);

    QVERIFY(!assignment.active());
    QVERIFY(assignment.message().isEmpty());

    _disconnectMockLink();
}

void MotorAssignmentTest::_testInitAssignment()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    QmlObjectListModel actuators;
    MotorAssignment assignment(this, _vehicle, &actuators);

    QSignalSpy messageSpy(&assignment, &MotorAssignment::messageChanged);

    // Without proper actuators, initialization will set an error message
    bool result = assignment.initAssignment(0, 1, 4);

    // Message should have been set (either success or error)
    QVERIFY(messageSpy.count() > 0);
    QVERIFY(!assignment.message().isEmpty());

    Q_UNUSED(result);

    _disconnectMockLink();
}

void MotorAssignmentTest::_testStart()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    QmlObjectListModel actuators;
    MotorAssignment assignment(this, _vehicle, &actuators);

    QSignalSpy activeSpy(&assignment, &MotorAssignment::activeChanged);

    assignment.start();

    // Should emit activeChanged
    QVERIFY(activeSpy.count() > 0 || activeSpy.wait(100));
    QVERIFY(assignment.active());

    assignment.abort();

    _disconnectMockLink();
}

void MotorAssignmentTest::_testSelectMotor()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    QmlObjectListModel actuators;
    MotorAssignment assignment(this, _vehicle, &actuators);

    // Select motor when not active should be ignored
    assignment.selectMotor(0);
    QVERIFY(!assignment.active());

    _disconnectMockLink();
}

void MotorAssignmentTest::_testAbort()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    QmlObjectListModel actuators;
    MotorAssignment assignment(this, _vehicle, &actuators);

    QSignalSpy abortSpy(&assignment, &MotorAssignment::onAbort);

    assignment.start();
    QVERIFY(assignment.active());

    assignment.abort();

    QVERIFY(abortSpy.count() > 0 || abortSpy.wait(100));

    _disconnectMockLink();
}

void MotorAssignmentTest::_testActiveProperty()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    QmlObjectListModel actuators;
    MotorAssignment assignment(this, _vehicle, &actuators);

    QVERIFY(!assignment.active());

    assignment.start();
    QTest::qWait(50);  // Allow state transitions
    QVERIFY(assignment.active());

    assignment.abort();
    QTest::qWait(100);  // Allow abort to complete

    _disconnectMockLink();
}

void MotorAssignmentTest::_testMessageProperty()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    QmlObjectListModel actuators;
    MotorAssignment assignment(this, _vehicle, &actuators);

    // Initial message is empty
    QVERIFY(assignment.message().isEmpty());

    // After init, message should be set
    assignment.initAssignment(0, 1, 4);
    QVERIFY(!assignment.message().isEmpty());

    _disconnectMockLink();
}
