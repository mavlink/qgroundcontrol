#include "PowerCalibrationStateMachineTest.h"
#include "PowerCalibrationStateMachine.h"
#include "PowerComponentController.h"
#include "Vehicle.h"
#include "MultiVehicleManager.h"
#include "MockLink.h"

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void PowerCalibrationStateMachineTest::_testStateCreation()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    PowerComponentController controller;
    controller._vehicle = _vehicle;

    PowerCalibrationStateMachine stateMachine(&controller, _vehicle, this);

    QCOMPARE(stateMachine.machineName(), QStringLiteral("PowerCalibration"));
    QVERIFY(!stateMachine.isCalibrating());
    QCOMPARE(stateMachine.calibrationType(), PowerCalibrationStateMachine::CalibrationNone);
    QVERIFY(stateMachine.warningMessages().isEmpty());

    _disconnectMockLink();
}

void PowerCalibrationStateMachineTest::_testStartEscCalibration()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    PowerComponentController controller;
    controller._vehicle = _vehicle;

    PowerCalibrationStateMachine stateMachine(&controller, _vehicle, this);

    stateMachine.startEscCalibration();

    QVERIFY(stateMachine.isRunning());
    QVERIFY(stateMachine.isCalibrating());
    QCOMPARE(stateMachine.calibrationType(), PowerCalibrationStateMachine::CalibrationEsc);

    stateMachine.stop();

    _disconnectMockLink();
}

void PowerCalibrationStateMachineTest::_testStartBusConfig()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    PowerComponentController controller;
    controller._vehicle = _vehicle;

    PowerCalibrationStateMachine stateMachine(&controller, _vehicle, this);

    stateMachine.startBusConfig();

    QVERIFY(stateMachine.isRunning());
    QVERIFY(stateMachine.isCalibrating());
    QCOMPARE(stateMachine.calibrationType(), PowerCalibrationStateMachine::CalibrationBusConfig);

    stateMachine.stop();

    _disconnectMockLink();
}

void PowerCalibrationStateMachineTest::_testConnectBatteryMessage()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    PowerComponentController controller;
    controller._vehicle = _vehicle;

    PowerCalibrationStateMachine stateMachine(&controller, _vehicle, this);

    QSignalSpy connectBatterySpy(&stateMachine, &PowerCalibrationStateMachine::connectBattery);

    stateMachine.startEscCalibration();
    QVERIFY(stateMachine.isCalibrating());

    // Simulate connect battery message
    stateMachine.handleTextMessage("[cal] Connect battery now");

    QTest::qWait(50);

    QCOMPARE(connectBatterySpy.count(), 1);

    stateMachine.stop();

    _disconnectMockLink();
}

void PowerCalibrationStateMachineTest::_testBatteryConnectedMessage()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    PowerComponentController controller;
    controller._vehicle = _vehicle;

    PowerCalibrationStateMachine stateMachine(&controller, _vehicle, this);

    QSignalSpy batteryConnectedSpy(&stateMachine, &PowerCalibrationStateMachine::batteryConnected);

    stateMachine.startEscCalibration();

    // Progress to waiting battery state
    stateMachine.handleTextMessage("[cal] Connect battery now");
    QTest::qWait(50);

    // Simulate battery connected
    stateMachine.handleTextMessage("[cal] Battery connected");
    QTest::qWait(50);

    QCOMPARE(batteryConnectedSpy.count(), 1);

    stateMachine.stop();

    _disconnectMockLink();
}

void PowerCalibrationStateMachineTest::_testCalibrationSuccess()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    PowerComponentController controller;
    controller._vehicle = _vehicle;

    PowerCalibrationStateMachine stateMachine(&controller, _vehicle, this);

    QSignalSpy successSpy(&stateMachine, &PowerCalibrationStateMachine::calibrationSuccess);

    stateMachine.startEscCalibration();

    // Progress through states
    stateMachine.handleTextMessage("[cal] Connect battery now");
    QTest::qWait(50);
    stateMachine.handleTextMessage("[cal] Battery connected");
    QTest::qWait(50);

    // Calibration done
    stateMachine.handleTextMessage("[cal] calibration done: success");
    QTest::qWait(50);

    QCOMPARE(successSpy.count(), 1);
    QVERIFY(!stateMachine.isCalibrating());

    _disconnectMockLink();
}

void PowerCalibrationStateMachineTest::_testCalibrationFailed()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    PowerComponentController controller;
    controller._vehicle = _vehicle;

    PowerCalibrationStateMachine stateMachine(&controller, _vehicle, this);

    QSignalSpy failedSpy(&stateMachine, &PowerCalibrationStateMachine::calibrationFailed);

    stateMachine.startEscCalibration();

    // Simulate failure
    stateMachine.handleTextMessage("[cal] calibration failed: ESC not responding");
    QTest::qWait(50);

    QCOMPARE(failedSpy.count(), 1);
    QVERIFY(!stateMachine.isCalibrating());

    // Check error message
    QList<QVariant> arguments = failedSpy.takeFirst();
    QVERIFY(arguments.at(0).toString().contains("ESC not responding"));

    _disconnectMockLink();
}

void PowerCalibrationStateMachineTest::_testDisconnectBattery()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    PowerComponentController controller;
    controller._vehicle = _vehicle;

    PowerCalibrationStateMachine stateMachine(&controller, _vehicle, this);

    QSignalSpy disconnectSpy(&stateMachine, &PowerCalibrationStateMachine::disconnectBattery);

    stateMachine.startEscCalibration();

    // Simulate disconnect battery failure
    stateMachine.handleTextMessage("[cal] calibration failed: Disconnect battery and try again");
    QTest::qWait(50);

    QCOMPARE(disconnectSpy.count(), 1);
    QVERIFY(!stateMachine.isCalibrating());

    _disconnectMockLink();
}

void PowerCalibrationStateMachineTest::_testWarningMessages()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    PowerComponentController controller;
    controller._vehicle = _vehicle;

    PowerCalibrationStateMachine stateMachine(&controller, _vehicle, this);

    stateMachine.startEscCalibration();

    // Add some warnings
    stateMachine.handleTextMessage("[cal] config warning: ESC 1 calibration marginal");
    stateMachine.handleTextMessage("[cal] config warning: ESC 3 calibration marginal");

    QCOMPARE(stateMachine.warningMessages().count(), 2);
    QVERIFY(stateMachine.warningMessages().at(0).contains("ESC 1"));
    QVERIFY(stateMachine.warningMessages().at(1).contains("ESC 3"));

    stateMachine.stop();

    _disconnectMockLink();
}

void PowerCalibrationStateMachineTest::_testFullEscCalibrationWorkflow()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    PowerComponentController controller;
    controller._vehicle = _vehicle;

    PowerCalibrationStateMachine stateMachine(&controller, _vehicle, this);

    QSignalSpy connectBatterySpy(&stateMachine, &PowerCalibrationStateMachine::connectBattery);
    QSignalSpy batteryConnectedSpy(&stateMachine, &PowerCalibrationStateMachine::batteryConnected);
    QSignalSpy successSpy(&stateMachine, &PowerCalibrationStateMachine::calibrationSuccess);

    // Start calibration
    stateMachine.startEscCalibration();
    QVERIFY(stateMachine.isCalibrating());
    QCOMPARE(stateMachine.calibrationType(), PowerCalibrationStateMachine::CalibrationEsc);

    // Calibration started message
    stateMachine.handleTextMessage("[cal] calibration started: 1 esc");
    QTest::qWait(50);

    // Connect battery
    stateMachine.handleTextMessage("[cal] Connect battery now");
    QTest::qWait(50);
    QCOMPARE(connectBatterySpy.count(), 1);

    // Battery connected
    stateMachine.handleTextMessage("[cal] Battery connected");
    QTest::qWait(50);
    QCOMPARE(batteryConnectedSpy.count(), 1);

    // Add a warning
    stateMachine.handleTextMessage("[cal] config warning: ESC 2 slow response");
    QCOMPARE(stateMachine.warningMessages().count(), 1);

    // Calibration done
    stateMachine.handleTextMessage("[cal] calibration done: success");
    QTest::qWait(50);

    QCOMPARE(successSpy.count(), 1);
    QVERIFY(!stateMachine.isCalibrating());

    // Check warnings passed to success signal
    QList<QVariant> arguments = successSpy.takeFirst();
    QStringList warnings = arguments.at(0).toStringList();
    QCOMPARE(warnings.count(), 1);

    _disconnectMockLink();
}

void PowerCalibrationStateMachineTest::_testStop()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    PowerComponentController controller;
    controller._vehicle = _vehicle;

    PowerCalibrationStateMachine stateMachine(&controller, _vehicle, this);

    stateMachine.startEscCalibration();
    QVERIFY(stateMachine.isCalibrating());

    stateMachine.stop();

    QVERIFY(!stateMachine.isCalibrating());
    QCOMPARE(stateMachine.calibrationType(), PowerCalibrationStateMachine::CalibrationNone);

    _disconnectMockLink();
}
