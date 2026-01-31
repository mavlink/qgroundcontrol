#pragma once

#include "UnitTest.h"
#include "FirmwareUpgradeStateMachine.h"
#include "FirmwareUpgradeController.h"

#include <QtQuick/QQuickItem>

class MockFirmwareUpgradeController : public FirmwareUpgradeController
{
    Q_OBJECT

public:
    MockFirmwareUpgradeController(QObject* parent = nullptr);

    FirmwareUpgradeStateMachine* stateMachine() const { return _getStateMachine(); }

    void setBoardInfo(uint32_t boardId, uint32_t flashSize);
};

class FirmwareUpgradeStateMachineTest : public UnitTest
{
    Q_OBJECT

private slots:
    void cleanup(void);

    void _testInitialState();
    void _testStartBoardSearch();
    void _testBoardFoundTransitions();
    void _testBootloaderConnection();
    void _testCancelOperation();
    void _testErrorHandling();
    void _testProgressTracking();

private:
    void _initStateMachine();

    MockFirmwareUpgradeController* _controller = nullptr;
    FirmwareUpgradeStateMachine* _stateMachine = nullptr;
    QQuickItem* _statusLog = nullptr;
    QQuickItem* _progressBar = nullptr;
};
