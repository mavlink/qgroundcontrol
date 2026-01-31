#include "PX4SensorCalibrationStateMachineTest.h"
#include "PX4SensorCalibrationStateMachine.h"
#include "SensorsComponentController.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "MockLink.h"

#include <QtCore/QCoreApplication>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

void PX4SensorCalibrationStateMachineTest::_testInitialState()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    PX4SensorCalibrationStateMachine stateMachine(nullptr, this);

    QVERIFY(!stateMachine.isCalibrating());
    QCOMPARE(stateMachine.currentCalibrationType(), QGCMAVLink::CalibrationNone);

    _disconnectMockLink();
}

void PX4SensorCalibrationStateMachineTest::_testCompassCalibration()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    PX4SensorCalibrationStateMachine stateMachine(nullptr, this);
    QSignalSpy startedSpy(&stateMachine, &PX4SensorCalibrationStateMachine::calibrationStarted);

    stateMachine.calibrateCompass();
    QCoreApplication::processEvents();

    QCOMPARE(startedSpy.count(), 1);
    QCOMPARE(static_cast<QGCMAVLink::CalibrationType>(startedSpy.at(0).at(0).toInt()),
             QGCMAVLink::CalibrationMag);
    QCOMPARE(stateMachine.currentCalibrationType(), QGCMAVLink::CalibrationMag);

    _disconnectMockLink();
}

void PX4SensorCalibrationStateMachineTest::_testGyroCalibration()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    PX4SensorCalibrationStateMachine stateMachine(nullptr, this);
    QSignalSpy startedSpy(&stateMachine, &PX4SensorCalibrationStateMachine::calibrationStarted);

    stateMachine.calibrateGyro();
    QCoreApplication::processEvents();

    QCOMPARE(startedSpy.count(), 1);
    QCOMPARE(static_cast<QGCMAVLink::CalibrationType>(startedSpy.at(0).at(0).toInt()),
             QGCMAVLink::CalibrationGyro);

    _disconnectMockLink();
}

void PX4SensorCalibrationStateMachineTest::_testAccelCalibration()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    PX4SensorCalibrationStateMachine stateMachine(nullptr, this);
    QSignalSpy startedSpy(&stateMachine, &PX4SensorCalibrationStateMachine::calibrationStarted);

    stateMachine.calibrateAccel();
    QCoreApplication::processEvents();

    QCOMPARE(startedSpy.count(), 1);
    QCOMPARE(static_cast<QGCMAVLink::CalibrationType>(startedSpy.at(0).at(0).toInt()),
             QGCMAVLink::CalibrationAccel);

    _disconnectMockLink();
}

void PX4SensorCalibrationStateMachineTest::_testLevelCalibration()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    PX4SensorCalibrationStateMachine stateMachine(nullptr, this);
    QSignalSpy startedSpy(&stateMachine, &PX4SensorCalibrationStateMachine::calibrationStarted);

    stateMachine.calibrateLevel();
    QCoreApplication::processEvents();

    QCOMPARE(startedSpy.count(), 1);
    QCOMPARE(static_cast<QGCMAVLink::CalibrationType>(startedSpy.at(0).at(0).toInt()),
             QGCMAVLink::CalibrationLevel);

    _disconnectMockLink();
}

void PX4SensorCalibrationStateMachineTest::_testAirspeedCalibration()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    PX4SensorCalibrationStateMachine stateMachine(nullptr, this);
    QSignalSpy startedSpy(&stateMachine, &PX4SensorCalibrationStateMachine::calibrationStarted);

    stateMachine.calibrateAirspeed();
    QCoreApplication::processEvents();

    QCOMPARE(startedSpy.count(), 1);
    QCOMPARE(static_cast<QGCMAVLink::CalibrationType>(startedSpy.at(0).at(0).toInt()),
             QGCMAVLink::CalibrationPX4Airspeed);

    _disconnectMockLink();
}

void PX4SensorCalibrationStateMachineTest::_testCancelCalibration()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    PX4SensorCalibrationStateMachine stateMachine(nullptr, this);
    QSignalSpy completeSpy(&stateMachine, &PX4SensorCalibrationStateMachine::calibrationComplete);

    // Start a calibration
    stateMachine.calibrateGyro();
    QCoreApplication::processEvents();

    QVERIFY(stateMachine.isCalibrating());

    // Cancel it
    stateMachine.cancelCalibration();
    QCoreApplication::processEvents();

    // The cancel should trigger calibration complete (with failure)
    // after the firmware acknowledges

    _disconnectMockLink();
}

void PX4SensorCalibrationStateMachineTest::_testTextMessageHandling()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    PX4SensorCalibrationStateMachine stateMachine(nullptr, this);

    // Start calibration
    stateMachine.calibrateAccel();
    QCoreApplication::processEvents();

    // Handle calibration started message
    stateMachine.handleTextMessage("[cal] calibration started: 2 accel");
    QCoreApplication::processEvents();

    // Handle orientation detection
    stateMachine.handleTextMessage("[cal] down orientation detected");
    QCoreApplication::processEvents();

    // Handle side done
    stateMachine.handleTextMessage("[cal] down side done, rotate to a different side");
    QCoreApplication::processEvents();

    _disconnectMockLink();
}

void PX4SensorCalibrationStateMachineTest::_testProgressParsing()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    PX4SensorCalibrationStateMachine stateMachine(nullptr, this);

    stateMachine.calibrateLevel();
    QCoreApplication::processEvents();

    // Handle progress messages (these are HTML-encoded in some cases)
    stateMachine.handleTextMessage("progress <50>");
    QCoreApplication::processEvents();

    stateMachine.handleTextMessage("progress &lt;75&gt;");
    QCoreApplication::processEvents();

    _disconnectMockLink();
}

void PX4SensorCalibrationStateMachineTest::_testFirmwareVersionCheck()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    PX4SensorCalibrationStateMachine stateMachine(nullptr, this);

    stateMachine.calibrateAccel();
    QCoreApplication::processEvents();

    // Handle unsupported firmware version
    stateMachine.handleTextMessage("[cal] calibration started: 99 accel");
    QCoreApplication::processEvents();

    // Handle supported firmware version
    stateMachine.handleTextMessage("[cal] calibration started: 2 accel");
    QCoreApplication::processEvents();

    _disconnectMockLink();
}
