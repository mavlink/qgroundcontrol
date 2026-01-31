#include "PX4OrientationCalibrationMachineTest.h"
#include "PX4OrientationCalibrationMachine.h"
#include "SensorCalibrationSide.h"
#include "SensorsComponentController.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "MockLink.h"

#include <QtCore/QCoreApplication>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void PX4OrientationCalibrationMachineTest::_testStateTransitions()
{
    // Test basic state machine transitions
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    PX4OrientationCalibrationMachine stateMachine(nullptr, this);

    QVERIFY(!stateMachine.isCalibrating());
    QVERIFY(!stateMachine.isRunning());

    // Start the state machine
    stateMachine.start();
    QCoreApplication::processEvents();

    QVERIFY(stateMachine.isRunning());

    _disconnectMockLink();
}

void PX4OrientationCalibrationMachineTest::_testSideDetection()
{
    // Test that side detection messages are handled correctly
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    PX4OrientationCalibrationMachine stateMachine(nullptr, this);
    QSignalSpy sideStartedSpy(&stateMachine, &PX4OrientationCalibrationMachine::sideStarted);

    // Start accel calibration (all 6 sides)
    stateMachine.startCalibration(PX4OrientationCalibrationMachine::CalibrationType::Accel, 0x3F);
    QCoreApplication::processEvents();

    QVERIFY(QTest::qWaitFor([&]() { return stateMachine.isCalibrating(); }, 1000));

    // Simulate "down orientation detected" message
    bool handled = stateMachine.handleCalibrationMessage("down orientation detected");
    QCoreApplication::processEvents();

    QVERIFY(handled);
    QCOMPARE(sideStartedSpy.count(), 1);
    QCOMPARE(static_cast<CalibrationSide>(sideStartedSpy.at(0).at(0).toInt()),
             CalibrationSide::Down);

    _disconnectMockLink();
}

void PX4OrientationCalibrationMachineTest::_testSideCompletion()
{
    // Test that side completion messages are handled correctly
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    PX4OrientationCalibrationMachine stateMachine(nullptr, this);
    QSignalSpy sideCompletedSpy(&stateMachine, &PX4OrientationCalibrationMachine::sideCompleted);

    stateMachine.startCalibration(PX4OrientationCalibrationMachine::CalibrationType::Accel, 0x3F);
    QCoreApplication::processEvents();

    QVERIFY(QTest::qWaitFor([&]() { return stateMachine.isCalibrating(); }, 1000));

    // Simulate orientation detected then completed
    stateMachine.handleCalibrationMessage("down orientation detected");
    QCoreApplication::processEvents();

    bool handled = stateMachine.handleCalibrationMessage("down side done, rotate to a different side");
    QCoreApplication::processEvents();

    QVERIFY(handled);
    QCOMPARE(sideCompletedSpy.count(), 1);
    QCOMPARE(static_cast<CalibrationSide>(sideCompletedSpy.at(0).at(0).toInt()),
             CalibrationSide::Down);

    _disconnectMockLink();
}

void PX4OrientationCalibrationMachineTest::_testMagCalibrationRotate()
{
    // Test that mag calibration sets rotate flag
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    PX4OrientationCalibrationMachine stateMachine(nullptr, this);

    // Start mag calibration
    stateMachine.startCalibration(PX4OrientationCalibrationMachine::CalibrationType::Mag, 0x3F);
    QCoreApplication::processEvents();

    QVERIFY(QTest::qWaitFor([&]() { return stateMachine.isCalibrating(); }, 1000));
    QCOMPARE(stateMachine.calibrationType(), PX4OrientationCalibrationMachine::CalibrationType::Mag);

    _disconnectMockLink();
}

void PX4OrientationCalibrationMachineTest::_testCancelCalibration()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    PX4OrientationCalibrationMachine stateMachine(nullptr, this);
    QSignalSpy cancelledSpy(&stateMachine, &PX4OrientationCalibrationMachine::calibrationCancelled);

    stateMachine.startCalibration(PX4OrientationCalibrationMachine::CalibrationType::Accel, 0x3F);
    QCoreApplication::processEvents();

    QVERIFY(QTest::qWaitFor([&]() { return stateMachine.isCalibrating(); }, 1000));

    stateMachine.cancelCalibration();
    QCoreApplication::processEvents();

    if (cancelledSpy.count() == 0) {
        QVERIFY(cancelledSpy.wait(1000));
    }

    QCOMPARE(cancelledSpy.count(), 1);

    _disconnectMockLink();
}

void PX4OrientationCalibrationMachineTest::_testCalibrationComplete()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    PX4OrientationCalibrationMachine stateMachine(nullptr, this);
    QSignalSpy completeSpy(&stateMachine, &PX4OrientationCalibrationMachine::calibrationComplete);

    stateMachine.startCalibration(PX4OrientationCalibrationMachine::CalibrationType::Accel, 0x3F);
    QCoreApplication::processEvents();

    QVERIFY(QTest::qWaitFor([&]() { return stateMachine.isCalibrating(); }, 1000));

    // Simulate calibration done message
    bool handled = stateMachine.handleCalibrationMessage("calibration done: accel");
    QCoreApplication::processEvents();

    QVERIFY(handled);

    if (completeSpy.count() == 0) {
        QVERIFY(completeSpy.wait(1000));
    }

    QCOMPARE(completeSpy.count(), 1);
    QCOMPARE(completeSpy.at(0).at(0).toBool(), true); // success = true

    _disconnectMockLink();
}

void PX4OrientationCalibrationMachineTest::_testCalibrationFailed()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    PX4OrientationCalibrationMachine stateMachine(nullptr, this);
    QSignalSpy completeSpy(&stateMachine, &PX4OrientationCalibrationMachine::calibrationComplete);

    stateMachine.startCalibration(PX4OrientationCalibrationMachine::CalibrationType::Accel, 0x3F);
    QCoreApplication::processEvents();

    QVERIFY(QTest::qWaitFor([&]() { return stateMachine.isCalibrating(); }, 1000));

    // Simulate calibration failed message
    bool handled = stateMachine.handleCalibrationMessage("calibration failed");
    QCoreApplication::processEvents();

    QVERIFY(handled);

    if (completeSpy.count() == 0) {
        QVERIFY(completeSpy.wait(1000));
    }

    QCOMPARE(completeSpy.count(), 1);
    QCOMPARE(completeSpy.at(0).at(0).toBool(), false); // success = false

    _disconnectMockLink();
}
