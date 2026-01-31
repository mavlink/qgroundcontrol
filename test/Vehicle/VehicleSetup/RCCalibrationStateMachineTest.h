#pragma once

#include "UnitTest.h"
#include "RCCalibrationStateMachine.h"
#include "RemoteControlCalibrationController.h"

#include <QtQuick/QQuickItem>

class TestRCController : public RemoteControlCalibrationController
{
    Q_OBJECT

public:
    TestRCController(QObject* parent = nullptr);

    void setChannelCount(int count) { _chanCount = count; }

private:
    void _saveStoredCalibrationValues() override {}
    void _readStoredCalibrationValues() override {}
};

class RCCalibrationStateMachineTest : public UnitTest
{
    Q_OBJECT

private slots:
    void cleanup(void);

    void _testInitialState();
    void _testStartCalibration();
    void _testCancelCalibration();
    void _testStateTransitions();
    void _testStickDetection();
    void _testInputProcessing();

private:
    void _initStateMachine();

    TestRCController* _controller = nullptr;
    RCCalibrationStateMachine* _stateMachine = nullptr;
    QQuickItem* _statusText = nullptr;
    QQuickItem* _cancelButton = nullptr;
    QQuickItem* _nextButton = nullptr;
};
