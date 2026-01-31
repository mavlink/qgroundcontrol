#pragma once

#include "UnitTest.h"
#include "RemoteControlCalibrationController.h"

#include <QtQuick/QQuickItem>

class TestRemoteControlCalibrationController : public RemoteControlCalibrationController
{
    Q_OBJECT

public:
    TestRemoteControlCalibrationController(QObject *parent = nullptr);

    bool saveCalledFlag() const { return _saveCalled; }
    bool readCalledFlag() const { return _readCalled; }
    void resetFlags() { _saveCalled = false; _readCalled = false; }

    void setChannelCount(int count) { _chanCount = count; }

private:
    void _saveStoredCalibrationValues() override { _saveCalled = true; }
    void _readStoredCalibrationValues() override { _readCalled = true; }

    bool _saveCalled = false;
    bool _readCalled = false;
};

class RemoteControlCalibrationControllerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void cleanup(void);

    void _testInitialState();
    void _testTransmitterMode();
    void _testTransmitterModeInvalid();
    void _testCenteredThrottle();
    void _testJoystickMode();
    void _testChannelMappingInitial();
    void _testCalibratingState();
    void _testRawChannelValuesChanged();
    void _testChannelCountUpdate();
    void _testStickDisplayPositions();
    void _testCalibrationStateTransitions();
    void _testStickDetection();

private:
    void _initController();

    TestRemoteControlCalibrationController* _controller = nullptr;
    QQuickItem* _statusText = nullptr;
    QQuickItem* _cancelButton = nullptr;
    QQuickItem* _nextButton = nullptr;
};
