#include "RCCalibrationStateMachineTest.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "MockLink.h"

#include <QtCore/QCoreApplication>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

TestRCController::TestRCController(QObject* parent)
    : RemoteControlCalibrationController(parent)
{
    _calValidMinValue = 100;
    _calValidMaxValue = 1900;
    _calCenterPoint = 1500;
    _calDefaultMinValue = 1000;
    _calDefaultMaxValue = 2000;
    _calRoughCenterDelta = 50;
    _calMoveDelta = 300;
    _calSettleDelta = 20;
}

void RCCalibrationStateMachineTest::cleanup()
{
    delete _controller;
    _controller = nullptr;
    _stateMachine = nullptr;

    delete _statusText;
    _statusText = nullptr;

    delete _cancelButton;
    _cancelButton = nullptr;

    delete _nextButton;
    _nextButton = nullptr;

    UnitTest::cleanup();
}

void RCCalibrationStateMachineTest::_initStateMachine()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    _statusText = new QQuickItem();
    _cancelButton = new QQuickItem();
    _nextButton = new QQuickItem();

    _controller = new TestRCController(this);
    _controller->setProperty("statusText", QVariant::fromValue(_statusText));
    _controller->setProperty("cancelButton", QVariant::fromValue(_cancelButton));
    _controller->setProperty("nextButton", QVariant::fromValue(_nextButton));
    _controller->setProperty("joystickMode", false);
    _controller->setChannelCount(8);

    // Get access to the state machine through the controller's friend relationship
    // For testing, we create a new state machine directly
    _stateMachine = new RCCalibrationStateMachine(_controller, this);
}

void RCCalibrationStateMachineTest::_testInitialState()
{
    _initStateMachine();

    QVERIFY(!_stateMachine->isCalibrating());
    QVERIFY(!_stateMachine->isRunning());

    _disconnectMockLink();
}

void RCCalibrationStateMachineTest::_testStartCalibration()
{
    _initStateMachine();

    QSignalSpy startedSpy(_stateMachine, &RCCalibrationStateMachine::calibrationStarted);
    QSignalSpy stepChangedSpy(_stateMachine, &RCCalibrationStateMachine::stepChanged);

    QVERIFY(!_stateMachine->isCalibrating());

    _stateMachine->startCalibration();
    QCoreApplication::processEvents();

    // Wait for calibration to start
    if (startedSpy.count() == 0) {
        QVERIFY(startedSpy.wait(1000));
    }

    QCOMPARE(startedSpy.count(), 1);
    QVERIFY(_stateMachine->isRunning());

    // Should have transitioned to StickNeutral state
    QCoreApplication::processEvents();
    QVERIFY(_stateMachine->isCalibrating());

    _disconnectMockLink();
}

void RCCalibrationStateMachineTest::_testCancelCalibration()
{
    _initStateMachine();

    QSignalSpy cancelledSpy(_stateMachine, &RCCalibrationStateMachine::calibrationCancelled);

    _stateMachine->startCalibration();
    QCoreApplication::processEvents();

    // Wait for state machine to start
    QVERIFY(QTest::qWaitFor([this]() { return _stateMachine->isCalibrating(); }, 1000));

    _stateMachine->cancelCalibration();
    QCoreApplication::processEvents();

    // Wait for cancel signal
    if (cancelledSpy.count() == 0) {
        QVERIFY(cancelledSpy.wait(1000));
    }

    QCOMPARE(cancelledSpy.count(), 1);

    _disconnectMockLink();
}

void RCCalibrationStateMachineTest::_testStateTransitions()
{
    _initStateMachine();

    QSignalSpy stepChangedSpy(_stateMachine, &RCCalibrationStateMachine::stepChanged);

    _stateMachine->startCalibration();
    QCoreApplication::processEvents();

    QVERIFY(QTest::qWaitFor([this]() { return _stateMachine->isCalibrating(); }, 1000));

    // Click next to advance from StickNeutral to ThrottleUp
    _stateMachine->nextButtonPressed();
    QCoreApplication::processEvents();

    // Should have received step change signals
    QVERIFY(stepChangedSpy.count() > 0);

    _disconnectMockLink();
}

void RCCalibrationStateMachineTest::_testStickDetection()
{
    _initStateMachine();

    _stateMachine->startCalibration();
    QCoreApplication::processEvents();

    QVERIFY(QTest::qWaitFor([this]() { return _stateMachine->isCalibrating(); }, 1000));

    // Send centered channel values during StickNeutral state
    for (int i = 0; i < 8; i++) {
        _stateMachine->processChannelInput(i, 1500);
    }
    QCoreApplication::processEvents();

    // Click next to proceed
    _stateMachine->nextButtonPressed();
    QCoreApplication::processEvents();

    // Should still be calibrating
    QVERIFY(_stateMachine->isCalibrating());

    _disconnectMockLink();
}

void RCCalibrationStateMachineTest::_testInputProcessing()
{
    _initStateMachine();

    _stateMachine->startCalibration();
    QCoreApplication::processEvents();

    QVERIFY(QTest::qWaitFor([this]() { return _stateMachine->isCalibrating(); }, 1000));

    // Process input should not crash when not calibrating or in wrong state
    _stateMachine->processChannelInput(0, 1500);
    _stateMachine->processChannelInput(1, 1000);
    _stateMachine->processChannelInput(2, 2000);

    QCoreApplication::processEvents();

    // Should still be in valid state
    QVERIFY(_stateMachine->isCalibrating());

    _disconnectMockLink();
}
