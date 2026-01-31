#include "RemoteControlCalibrationControllerTest.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "MockLink.h"

#include <QtCore/QCoreApplication>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

TestRemoteControlCalibrationController::TestRemoteControlCalibrationController(QObject *parent)
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

void RemoteControlCalibrationControllerTest::cleanup()
{
    delete _controller;
    _controller = nullptr;

    delete _statusText;
    _statusText = nullptr;

    delete _cancelButton;
    _cancelButton = nullptr;

    delete _nextButton;
    _nextButton = nullptr;

    UnitTest::cleanup();
}

void RemoteControlCalibrationControllerTest::_initController()
{
    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY(_vehicle);

    _statusText = new QQuickItem();
    _cancelButton = new QQuickItem();
    _nextButton = new QQuickItem();

    _controller = new TestRemoteControlCalibrationController(this);
    _controller->setProperty("statusText", QVariant::fromValue(_statusText));
    _controller->setProperty("cancelButton", QVariant::fromValue(_cancelButton));
    _controller->setProperty("nextButton", QVariant::fromValue(_nextButton));
    _controller->setProperty("joystickMode", false);
}

void RemoteControlCalibrationControllerTest::_testInitialState()
{
    _initController();

    QVERIFY(!_controller->calibrating());
    QCOMPARE(_controller->channelCount(), 0);
    QCOMPARE(_controller->transmitterMode(), 2);
    QVERIFY(!_controller->centeredThrottle());
    QVERIFY(!_controller->joystickMode());

    QVERIFY(!_controller->rollChannelMapped());
    QVERIFY(!_controller->pitchChannelMapped());
    QVERIFY(!_controller->yawChannelMapped());
    QVERIFY(!_controller->throttleChannelMapped());

    QVERIFY(!_controller->rollChannelReversed());
    QVERIFY(!_controller->pitchChannelReversed());
    QVERIFY(!_controller->yawChannelReversed());
    QVERIFY(!_controller->throttleChannelReversed());

    QCOMPARE(_controller->rollDeadband(), 0);
    QCOMPARE(_controller->pitchDeadband(), 0);
    QCOMPARE(_controller->yawDeadband(), 0);
    QCOMPARE(_controller->throttleDeadband(), 0);

    _disconnectMockLink();
}

void RemoteControlCalibrationControllerTest::_testTransmitterMode()
{
    _initController();

    QSignalSpy modeSpy(_controller, &RemoteControlCalibrationController::transmitterModeChanged);

    _controller->setTransmitterMode(1);
    QCOMPARE(_controller->transmitterMode(), 1);
    QCOMPARE(modeSpy.count(), 1);

    modeSpy.clear();
    _controller->setTransmitterMode(3);
    QCOMPARE(_controller->transmitterMode(), 3);
    QCOMPARE(modeSpy.count(), 1);

    modeSpy.clear();
    _controller->setTransmitterMode(4);
    QCOMPARE(_controller->transmitterMode(), 4);
    QCOMPARE(modeSpy.count(), 1);

    modeSpy.clear();
    _controller->setTransmitterMode(4);
    QCOMPARE(modeSpy.count(), 0);

    _disconnectMockLink();
}

void RemoteControlCalibrationControllerTest::_testTransmitterModeInvalid()
{
    _initController();

    _controller->setTransmitterMode(0);
    QCOMPARE(_controller->transmitterMode(), 2);

    _controller->setTransmitterMode(5);
    QCOMPARE(_controller->transmitterMode(), 2);

    _controller->setTransmitterMode(-1);
    QCOMPARE(_controller->transmitterMode(), 2);

    _disconnectMockLink();
}

void RemoteControlCalibrationControllerTest::_testCenteredThrottle()
{
    _initController();

    QSignalSpy centeredSpy(_controller, &RemoteControlCalibrationController::centeredThrottleChanged);

    QVERIFY(!_controller->centeredThrottle());

    _controller->setCenteredThrottle(true);
    QVERIFY(_controller->centeredThrottle());
    QCOMPARE(centeredSpy.count(), 1);
    QCOMPARE(centeredSpy.takeFirst().at(0).toBool(), true);

    centeredSpy.clear();
    _controller->setCenteredThrottle(true);
    QCOMPARE(centeredSpy.count(), 0);

    _controller->setCenteredThrottle(false);
    QVERIFY(!_controller->centeredThrottle());
    QCOMPARE(centeredSpy.count(), 1);
    QCOMPARE(centeredSpy.takeFirst().at(0).toBool(), false);

    _disconnectMockLink();
}

void RemoteControlCalibrationControllerTest::_testJoystickMode()
{
    _initController();

    QSignalSpy joystickSpy(_controller, &RemoteControlCalibrationController::joystickModeChanged);
    QSignalSpy centeredSpy(_controller, &RemoteControlCalibrationController::centeredThrottleChanged);

    QVERIFY(!_controller->joystickMode());
    QVERIFY(!_controller->centeredThrottle());

    _controller->setJoystickMode(true);
    QVERIFY(_controller->joystickMode());
    QVERIFY(_controller->centeredThrottle());
    QCOMPARE(joystickSpy.count(), 1);
    QCOMPARE(centeredSpy.count(), 1);

    joystickSpy.clear();
    centeredSpy.clear();
    _controller->setJoystickMode(true);
    QCOMPARE(joystickSpy.count(), 0);

    _controller->setJoystickMode(false);
    QVERIFY(!_controller->joystickMode());
    QCOMPARE(joystickSpy.count(), 1);

    _disconnectMockLink();
}

void RemoteControlCalibrationControllerTest::_testChannelMappingInitial()
{
    _initController();

    QVERIFY(!_controller->rollChannelMapped());
    QVERIFY(!_controller->pitchChannelMapped());
    QVERIFY(!_controller->yawChannelMapped());
    QVERIFY(!_controller->throttleChannelMapped());

    _disconnectMockLink();
}

void RemoteControlCalibrationControllerTest::_testCalibratingState()
{
    _initController();

    QSignalSpy calibratingSpy(_controller, &RemoteControlCalibrationController::calibratingChanged);

    QVERIFY(!_controller->calibrating());

    _controller->setChannelCount(8);

    _controller->start();
    QVERIFY(_controller->readCalledFlag());

    _controller->resetFlags();

    _controller->nextButtonClicked();

    // State machine starts async, process events to allow it to run
    QCoreApplication::processEvents();

    // Wait for calibrating signal if not already received
    if (calibratingSpy.count() == 0) {
        QVERIFY(calibratingSpy.wait(1000));
    }

    QVERIFY(_controller->calibrating());
    QCOMPARE(calibratingSpy.count(), 1);
    QCOMPARE(calibratingSpy.takeFirst().at(0).toBool(), true);

    calibratingSpy.clear();

    _controller->cancelButtonClicked();

    // Process events for cancel
    QCoreApplication::processEvents();

    // Wait for calibrating signal if not already received
    if (calibratingSpy.count() == 0) {
        QVERIFY(calibratingSpy.wait(1000));
    }

    QVERIFY(!_controller->calibrating());
    QCOMPARE(calibratingSpy.count(), 1);
    QCOMPARE(calibratingSpy.takeFirst().at(0).toBool(), false);

    _disconnectMockLink();
}

void RemoteControlCalibrationControllerTest::_testRawChannelValuesChanged()
{
    _initController();

    QSignalSpy rawChannelSpy(_controller, &RemoteControlCalibrationController::rawChannelValueChanged);

    QVector<int> channelValues = {1500, 1500, 1500, 1500, 1000, 2000, 1500, 1500};

    _controller->rawChannelValuesChanged(channelValues);

    QCOMPARE(rawChannelSpy.count(), 8);

    QList<QVariant> args = rawChannelSpy.at(0);
    QCOMPARE(args.at(0).toInt(), 0);
    QCOMPARE(args.at(1).toInt(), 1500);

    args = rawChannelSpy.at(4);
    QCOMPARE(args.at(0).toInt(), 4);
    QCOMPARE(args.at(1).toInt(), 1000);

    args = rawChannelSpy.at(5);
    QCOMPARE(args.at(0).toInt(), 5);
    QCOMPARE(args.at(1).toInt(), 2000);

    _disconnectMockLink();
}

void RemoteControlCalibrationControllerTest::_testChannelCountUpdate()
{
    _initController();

    QSignalSpy channelCountSpy(_controller, &RemoteControlCalibrationController::channelCountChanged);

    QCOMPARE(_controller->channelCount(), 0);

    QVector<int> channelValues = {1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500};
    _controller->rawChannelValuesChanged(channelValues);

    QCOMPARE(_controller->channelCount(), 8);
    QCOMPARE(channelCountSpy.count(), 1);
    QCOMPARE(channelCountSpy.takeFirst().at(0).toInt(), 8);

    channelCountSpy.clear();

    _controller->rawChannelValuesChanged(channelValues);
    QCOMPARE(channelCountSpy.count(), 0);

    QVector<int> sixChannels = {1500, 1500, 1500, 1500, 1500, 1500};
    _controller->rawChannelValuesChanged(sixChannels);
    QCOMPARE(_controller->channelCount(), 6);
    QCOMPARE(channelCountSpy.count(), 1);

    _disconnectMockLink();
}

void RemoteControlCalibrationControllerTest::_testStickDisplayPositions()
{
    _initController();

    QSignalSpy positionSpy(_controller, &RemoteControlCalibrationController::stickDisplayPositionsChanged);

    QList<int> positions = _controller->stickDisplayPositions();
    QCOMPARE(positions.size(), 4);
    QCOMPARE(positions[0], 0);
    QCOMPARE(positions[1], 0);
    QCOMPARE(positions[2], 0);
    QCOMPARE(positions[3], 0);

    _disconnectMockLink();
}

void RemoteControlCalibrationControllerTest::_testCalibrationStateTransitions()
{
    _initController();

    QSignalSpy calibratingSpy(_controller, &RemoteControlCalibrationController::calibratingChanged);
    QSignalSpy stickPositionSpy(_controller, &RemoteControlCalibrationController::stickDisplayPositionsChanged);

    _controller->setChannelCount(8);
    _controller->start();

    // Start calibration
    _controller->nextButtonClicked();
    QCoreApplication::processEvents();

    if (calibratingSpy.count() == 0) {
        QVERIFY(calibratingSpy.wait(1000));
    }

    QVERIFY(_controller->calibrating());

    // Verify stick positions changed for the first calibration step
    QVERIFY(stickPositionSpy.count() > 0);

    _disconnectMockLink();
}

void RemoteControlCalibrationControllerTest::_testStickDetection()
{
    _initController();

    _controller->setChannelCount(8);
    _controller->start();

    QSignalSpy calibratingSpy(_controller, &RemoteControlCalibrationController::calibratingChanged);

    // Start calibration
    _controller->nextButtonClicked();
    QCoreApplication::processEvents();

    if (calibratingSpy.count() == 0) {
        QVERIFY(calibratingSpy.wait(1000));
    }

    QVERIFY(_controller->calibrating());

    // Simulate centered sticks, then click next to proceed
    QVector<int> centeredValues(8, 1500);
    _controller->rawChannelValuesChanged(centeredValues);
    QCoreApplication::processEvents();

    // Click next to save trims and move to throttle detection
    _controller->nextButtonClicked();
    QCoreApplication::processEvents();

    // Verify still calibrating
    QVERIFY(_controller->calibrating());

    _disconnectMockLink();
}
