#include "AccelCalibrationMachineTest.h"
#include "AccelCalibrationMachine.h"
#include "APMSensorsComponentController.h"
#include "APMAutoPilotPlugin.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "MockLink.h"

#include <QtCore/QCoreApplication>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void AccelCalibrationMachineTest::_testStateTransitions()
{
    // Test that the state machine transitions through all 6 positions
    _connectMockLink(MAV_AUTOPILOT_ARDUPILOTMEGA);
    QVERIFY(_vehicle);

    // Create a mock controller (we can't easily test without the full QML setup)
    // For now, test basic state machine behavior
    AccelCalibrationMachine stateMachine(nullptr, this);

    QVERIFY(!stateMachine.isCalibrating());
    QVERIFY(!stateMachine.isRunning());

    // Start the state machine
    stateMachine.start();
    QCoreApplication::processEvents();

    QVERIFY(stateMachine.isRunning());

    _disconnectMockLink();
}

void AccelCalibrationMachineTest::_testPositionCommands()
{
    // Test handling of position commands
    _connectMockLink(MAV_AUTOPILOT_ARDUPILOTMEGA);
    QVERIFY(_vehicle);

    AccelCalibrationMachine stateMachine(nullptr, this);
    QSignalSpy positionSpy(&stateMachine, &AccelCalibrationMachine::positionChanged);

    stateMachine.start();
    QCoreApplication::processEvents();

    // Simulate start event
    stateMachine.startCalibration();
    QCoreApplication::processEvents();

    // Wait for calibration to start
    QVERIFY(QTest::qWaitFor([&]() { return positionSpy.count() > 0; }, 1000));

    // Should have received Level position
    QCOMPARE(static_cast<AccelCalibrationMachine::Position>(positionSpy.at(0).at(0).toInt()),
             AccelCalibrationMachine::Position::Level);

    _disconnectMockLink();
}

void AccelCalibrationMachineTest::_testCancelCalibration()
{
    _connectMockLink(MAV_AUTOPILOT_ARDUPILOTMEGA);
    QVERIFY(_vehicle);

    AccelCalibrationMachine stateMachine(nullptr, this);
    QSignalSpy cancelledSpy(&stateMachine, &AccelCalibrationMachine::calibrationCancelled);

    stateMachine.startCalibration();
    QCoreApplication::processEvents();

    QVERIFY(QTest::qWaitFor([&]() { return stateMachine.isRunning(); }, 1000));

    stateMachine.cancelCalibration();
    QCoreApplication::processEvents();

    if (cancelledSpy.count() == 0) {
        QVERIFY(cancelledSpy.wait(1000));
    }

    QCOMPARE(cancelledSpy.count(), 1);

    _disconnectMockLink();
}

void AccelCalibrationMachineTest::_testProgressTracking()
{
    _connectMockLink(MAV_AUTOPILOT_ARDUPILOTMEGA);
    QVERIFY(_vehicle);

    AccelCalibrationMachine stateMachine(nullptr, this);

    stateMachine.startCalibration();
    QCoreApplication::processEvents();

    QVERIFY(QTest::qWaitFor([&]() { return stateMachine.isCalibrating(); }, 1000));

    // Check initial position index
    QCOMPARE(stateMachine.currentPositionIndex(), 0);

    // Simulate position commands
    stateMachine.handlePositionCommand(AccelCalibrationMachine::Position::Left);
    QCoreApplication::processEvents();
    QCOMPARE(stateMachine.currentPositionIndex(), 1);

    stateMachine.handlePositionCommand(AccelCalibrationMachine::Position::Right);
    QCoreApplication::processEvents();
    QCOMPARE(stateMachine.currentPositionIndex(), 2);

    _disconnectMockLink();
}

void AccelCalibrationMachineTest::_testFailedCalibration()
{
    _connectMockLink(MAV_AUTOPILOT_ARDUPILOTMEGA);
    QVERIFY(_vehicle);

    AccelCalibrationMachine stateMachine(nullptr, this);
    QSignalSpy completeSpy(&stateMachine, &AccelCalibrationMachine::calibrationComplete);

    stateMachine.startCalibration();
    QCoreApplication::processEvents();

    QVERIFY(QTest::qWaitFor([&]() { return stateMachine.isCalibrating(); }, 1000));

    // Send failed position command
    stateMachine.handlePositionCommand(AccelCalibrationMachine::Position::Failed);
    QCoreApplication::processEvents();

    if (completeSpy.count() == 0) {
        QVERIFY(completeSpy.wait(1000));
    }

    QCOMPARE(completeSpy.count(), 1);
    QCOMPARE(completeSpy.at(0).at(0).toBool(), false); // success = false

    _disconnectMockLink();
}
