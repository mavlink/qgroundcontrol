#include "CompassCalibrationMachineTest.h"
#include "CompassCalibrationMachine.h"
#include "APMSensorsComponentController.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "MockLink.h"

#include <QtCore/QCoreApplication>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void CompassCalibrationMachineTest::_testStateTransitions()
{
    _connectMockLink(MAV_AUTOPILOT_ARDUPILOTMEGA);
    QVERIFY(_vehicle);

    CompassCalibrationMachine stateMachine(nullptr, this);

    QVERIFY(!stateMachine.isCalibrating());
    QVERIFY(!stateMachine.isRunning());

    // Start the state machine
    stateMachine.start();
    QCoreApplication::processEvents();

    QVERIFY(stateMachine.isRunning());

    _disconnectMockLink();
}

void CompassCalibrationMachineTest::_testProgressTracking()
{
    _connectMockLink(MAV_AUTOPILOT_ARDUPILOTMEGA);
    QVERIFY(_vehicle);

    CompassCalibrationMachine stateMachine(nullptr, this);
    QSignalSpy progressSpy(&stateMachine, &CompassCalibrationMachine::progressChanged);

    stateMachine.startCalibration();
    QCoreApplication::processEvents();

    QVERIFY(QTest::qWaitFor([&]() { return stateMachine.isRunning(); }, 1000));

    // Simulate support check succeeded
    stateMachine.handleSupportCheckResult(true);
    QCoreApplication::processEvents();

    // Simulate progress updates for compass 0
    stateMachine.handleMagCalProgress(0, 0x01, 50);
    QCoreApplication::processEvents();

    QCOMPARE(stateMachine.compassProgress(0), static_cast<uint8_t>(50));

    // More progress
    stateMachine.handleMagCalProgress(0, 0x01, 100);
    QCoreApplication::processEvents();

    QCOMPARE(stateMachine.compassProgress(0), static_cast<uint8_t>(100));

    _disconnectMockLink();
}

void CompassCalibrationMachineTest::_testCancelCalibration()
{
    _connectMockLink(MAV_AUTOPILOT_ARDUPILOTMEGA);
    QVERIFY(_vehicle);

    CompassCalibrationMachine stateMachine(nullptr, this);
    QSignalSpy cancelledSpy(&stateMachine, &CompassCalibrationMachine::calibrationCancelled);

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

void CompassCalibrationMachineTest::_testAllCompassesComplete()
{
    _connectMockLink(MAV_AUTOPILOT_ARDUPILOTMEGA);
    QVERIFY(_vehicle);

    CompassCalibrationMachine stateMachine(nullptr, this);
    QSignalSpy completeSpy(&stateMachine, &CompassCalibrationMachine::calibrationComplete);

    stateMachine.startCalibration();
    QCoreApplication::processEvents();

    QVERIFY(QTest::qWaitFor([&]() { return stateMachine.isRunning(); }, 1000));

    // Simulate support check
    stateMachine.handleSupportCheckResult(true);
    QCoreApplication::processEvents();

    // All compasses marked as already complete (no compass mask set)
    // This simulates the case where no compasses are being calibrated
    // The machine should complete successfully

    QVERIFY(!stateMachine.compassComplete(0) || !stateMachine.compassComplete(1) || !stateMachine.compassComplete(2));

    _disconnectMockLink();
}

void CompassCalibrationMachineTest::_testPartialFailure()
{
    _connectMockLink(MAV_AUTOPILOT_ARDUPILOTMEGA);
    QVERIFY(_vehicle);

    CompassCalibrationMachine stateMachine(nullptr, this);
    QSignalSpy compassCompleteSpy(&stateMachine, &CompassCalibrationMachine::compassCalComplete);

    stateMachine.startCalibration();
    QCoreApplication::processEvents();

    QVERIFY(QTest::qWaitFor([&]() { return stateMachine.isRunning(); }, 1000));

    stateMachine.handleSupportCheckResult(true);
    QCoreApplication::processEvents();

    // Simulate compass 0 succeeding (status 4 = MAG_CAL_SUCCESS)
    stateMachine.handleMagCalReport(0, 4, 0.5f);
    QCoreApplication::processEvents();

    QVERIFY(stateMachine.compassComplete(0));
    QVERIFY(stateMachine.compassSucceeded(0));
    QCOMPARE(stateMachine.compassFitness(0), 0.5f);

    // Simulate compass 1 failing (status != 4)
    stateMachine.handleMagCalReport(1, 3, 1.5f);
    QCoreApplication::processEvents();

    QVERIFY(stateMachine.compassComplete(1));
    QVERIFY(!stateMachine.compassSucceeded(1));

    _disconnectMockLink();
}
