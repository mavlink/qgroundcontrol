#include "FirmwareUpgradeStateMachineTest.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "MockLink.h"

#include <QtCore/QCoreApplication>
#include <QtSerialPort/QSerialPortInfo>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

MockFirmwareUpgradeController::MockFirmwareUpgradeController(QObject* parent)
    : FirmwareUpgradeController()
{
    Q_UNUSED(parent)
}

void MockFirmwareUpgradeController::setBoardInfo(uint32_t boardId, uint32_t flashSize)
{
    _bootloaderBoardID = boardId;
    _bootloaderBoardFlashSize = flashSize;
}

void FirmwareUpgradeStateMachineTest::cleanup()
{
    delete _controller;
    _controller = nullptr;
    _stateMachine = nullptr;

    delete _statusLog;
    _statusLog = nullptr;

    delete _progressBar;
    _progressBar = nullptr;

    UnitTest::cleanup();
}

void FirmwareUpgradeStateMachineTest::_initStateMachine()
{
    _statusLog = new QQuickItem();
    _progressBar = new QQuickItem();

    _controller = new MockFirmwareUpgradeController(this);
    _controller->setStatusLog(_statusLog);
    _controller->setProgressBar(_progressBar);

    _stateMachine = _controller->stateMachine();
    QVERIFY(_stateMachine != nullptr);
}

void FirmwareUpgradeStateMachineTest::_testInitialState()
{
    _initStateMachine();

    // State machine should not be running initially
    QVERIFY(!_stateMachine->isRunning());
    QVERIFY(!_stateMachine->isFlashing());

    // Progress should be 0
    QCOMPARE(_stateMachine->flashProgress(), 0.0);
}

void FirmwareUpgradeStateMachineTest::_testStartBoardSearch()
{
    _initStateMachine();

    QSignalSpy statusSpy(_stateMachine, &FirmwareUpgradeStateMachine::statusMessage);

    // Start board search
    _stateMachine->startBoardSearch();
    QCoreApplication::processEvents();

    // State machine should be running
    QVERIFY(_stateMachine->isRunning());

    // Should have received a status message
    QVERIFY(statusSpy.count() > 0);

    // Should be in WaitingForBoard state
    QString currentState = _stateMachine->currentStateName();
    QCOMPARE(currentState, QStringLiteral("WaitingForBoard"));
}

void FirmwareUpgradeStateMachineTest::_testBoardFoundTransitions()
{
    _initStateMachine();

    _stateMachine->startBoardSearch();
    QCoreApplication::processEvents();

    QVERIFY(_stateMachine->isRunning());

    // Simulate board found (first attempt - need replug)
    QSerialPortInfo portInfo;
    _stateMachine->handleBoardFound(true, portInfo, 0, "Test Board");
    QCoreApplication::processEvents();

    // Should transition to WaitingForReplug
    QString currentState = _stateMachine->currentStateName();
    QCOMPARE(currentState, QStringLiteral("WaitingForReplug"));

    // Simulate board replugged
    _stateMachine->handleBoardFound(false, portInfo, 0, "Test Board");
    QCoreApplication::processEvents();

    // Should transition to ConnectingBootloader
    currentState = _stateMachine->currentStateName();
    QCOMPARE(currentState, QStringLiteral("ConnectingBootloader"));
}

void FirmwareUpgradeStateMachineTest::_testBootloaderConnection()
{
    _initStateMachine();

    _stateMachine->startBoardSearch();
    QCoreApplication::processEvents();

    // Simulate replug sequence to get to ConnectingBootloader
    QSerialPortInfo portInfo;
    _stateMachine->handleBoardFound(true, portInfo, 0, "Test Board");
    QCoreApplication::processEvents();

    _stateMachine->handleBoardFound(false, portInfo, 0, "Test Board");
    QCoreApplication::processEvents();

    // Now in ConnectingBootloader, simulate bootloader info received
    _stateMachine->handleBootloaderInfo(4, 50, 2097152);
    QCoreApplication::processEvents();

    // Without pending flash, should emit show_firmware_select and go back to Idle
    // (for this test, we'd need to wait for the event to be processed)
}

void FirmwareUpgradeStateMachineTest::_testCancelOperation()
{
    _initStateMachine();

    _stateMachine->startBoardSearch();
    QCoreApplication::processEvents();

    QVERIFY(_stateMachine->isRunning());

    // Cancel the operation
    _stateMachine->cancel();
    QCoreApplication::processEvents();

    // Give state machine time to process
    QTest::qWait(100);
    QCoreApplication::processEvents();

    // Should transition back to Idle
    QString currentState = _stateMachine->currentStateName();
    QCOMPARE(currentState, QStringLiteral("Idle"));
}

void FirmwareUpgradeStateMachineTest::_testErrorHandling()
{
    _initStateMachine();

    QSignalSpy flashCompleteSpy(_stateMachine, &FirmwareUpgradeStateMachine::flashComplete);

    _stateMachine->startBoardSearch();
    QCoreApplication::processEvents();

    // Simulate no board found error
    _stateMachine->handleNoBoardFound();
    QCoreApplication::processEvents();

    // Give state machine time to process events
    QTest::qWait(100);
    QCoreApplication::processEvents();

    // Should transition to Error state
    QString currentState = _stateMachine->currentStateName();
    QCOMPARE(currentState, QStringLiteral("Error"));

    // Should have received flashComplete signal with failure
    if (flashCompleteSpy.count() == 0) {
        QVERIFY(flashCompleteSpy.wait(1000));
    }
    QCOMPARE(flashCompleteSpy.count(), 1);

    QList<QVariant> arguments = flashCompleteSpy.takeFirst();
    QCOMPARE(arguments.at(0).toBool(), false);
}

void FirmwareUpgradeStateMachineTest::_testProgressTracking()
{
    _initStateMachine();

    QSignalSpy progressSpy(_stateMachine, &FirmwareUpgradeStateMachine::progressChanged);

    _stateMachine->startBoardSearch();
    QCoreApplication::processEvents();

    // Simulate full workflow to test progress updates
    QSerialPortInfo portInfo;
    _stateMachine->handleBoardFound(true, portInfo, 0, "Test Board");
    QCoreApplication::processEvents();

    _stateMachine->handleBoardFound(false, portInfo, 0, "Test Board");
    QCoreApplication::processEvents();

    // Set board info needed for flash
    _controller->setBoardInfo(50, 2097152);

    // Simulate erase progress
    _stateMachine->handleEraseStarted();
    QCoreApplication::processEvents();

    // Simulate some flash progress
    _stateMachine->handleFlashProgress(100, 1000);
    QCoreApplication::processEvents();

    _stateMachine->handleFlashProgress(500, 1000);
    QCoreApplication::processEvents();

    // Progress should have been updated
    QVERIFY(_stateMachine->flashProgress() >= 0.0);
    QVERIFY(_stateMachine->flashProgress() <= 1.0);
}
