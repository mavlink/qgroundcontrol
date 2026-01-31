#include "ActuatorTestingTest.h"
#include "ActuatorTesting.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "MockLink.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

using namespace ActuatorTesting;

void ActuatorTestingTest::_testCreation()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    ActuatorTest test(_vehicle);

    QVERIFY(test.actuators() != nullptr);
    QVERIFY(test.allMotorsActuator() == nullptr);  // No actuators added yet
    QVERIFY(!test.hadFailure());

    _disconnectMockLink();
}

void ActuatorTestingTest::_testUpdateFunctions()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    ActuatorTest test(_vehicle);

    QSignalSpy actuatorsSpy(&test, &ActuatorTest::actuatorsChanged);

    QList<Actuator*> actuators;
    actuators.append(new Actuator(&test, "Motor1", 0.0f, 1.0f, 0.0f, 1, true));
    actuators.append(new Actuator(&test, "Servo1", -1.0f, 1.0f, 0.0f, 2, false));

    test.updateFunctions(actuators);

    QCOMPARE(actuatorsSpy.count(), 1);
    QCOMPARE(test.actuators()->count(), 2);

    _disconnectMockLink();
}

void ActuatorTestingTest::_testSetActive()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    ActuatorTest test(_vehicle);

    // Add some actuators first
    QList<Actuator*> actuators;
    actuators.append(new Actuator(&test, "Motor1", 0.0f, 1.0f, 0.0f, 1, true));
    test.updateFunctions(actuators);

    // Activate/deactivate should not crash
    test.setActive(true);
    QTest::qWait(50);

    test.setActive(false);
    QTest::qWait(50);

    _disconnectMockLink();
}

void ActuatorTestingTest::_testSetChannelTo()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    ActuatorTest test(_vehicle);

    QList<Actuator*> actuators;
    actuators.append(new Actuator(&test, "Motor1", 0.0f, 1.0f, 0.0f, 1, true));
    test.updateFunctions(actuators);

    test.setActive(true);
    QTest::qWait(50);

    // Setting channel should not crash
    test.setChannelTo(0, 0.5f);

    test.setActive(false);

    _disconnectMockLink();
}

void ActuatorTestingTest::_testStopControl()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    ActuatorTest test(_vehicle);

    QList<Actuator*> actuators;
    actuators.append(new Actuator(&test, "Motor1", 0.0f, 1.0f, 0.0f, 1, true));
    test.updateFunctions(actuators);

    test.setActive(true);
    QTest::qWait(50);

    test.setChannelTo(0, 0.5f);
    test.stopControl(0);

    // Stop all
    test.setChannelTo(0, 0.5f);
    test.stopControl(-1);

    test.setActive(false);

    _disconnectMockLink();
}

void ActuatorTestingTest::_testAllMotorsActuator()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    ActuatorTest test(_vehicle);

    // Without motors, allMotorsActuator is null
    QList<Actuator*> servos;
    servos.append(new Actuator(&test, "Servo1", -1.0f, 1.0f, 0.0f, 1, false));
    test.updateFunctions(servos);
    QVERIFY(test.allMotorsActuator() == nullptr);

    // With a motor, allMotorsActuator should be created
    QList<Actuator*> motors;
    motors.append(new Actuator(&test, "Motor1", 0.0f, 1.0f, 0.0f, 1, true));
    test.updateFunctions(motors);
    QVERIFY(test.allMotorsActuator() != nullptr);
    QCOMPARE(test.allMotorsActuator()->label(), tr("All Motors"));
    QVERIFY(test.allMotorsActuator()->isMotor());

    _disconnectMockLink();
}
