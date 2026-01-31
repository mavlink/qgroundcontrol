#include "FirmwareUpgradeStateMachine.h"
#include "FirmwareUpgradeController.h"
#include "FirmwareImage.h"
#include "PX4FirmwareUpgradeThread.h"
#include "QGCFileDownload.h"
#include "LinkManager.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QTimer>
#include "QGCSerialPortInfo.h"

QGC_LOGGING_CATEGORY(FirmwareUpgradeStateMachineLog, "FirmwareUpgradeStateMachine")

FirmwareUpgradeStateMachine::FirmwareUpgradeStateMachine(FirmwareUpgradeController* controller, QObject* parent)
    : QGCStateMachine("FirmwareUpgrade", nullptr, parent)
    , _controller(controller)
{
    _buildStateMachine();
    _wireTransitions();
    _wireProgressTracking();

    // Connect erase progress timer for simulated progress
    connect(&_eraseProgressTimer, &QTimer::timeout, this, [this]() {
        _eraseTickCount++;
        double progress = static_cast<double>(_eraseTickCount * _eraseTickMsec) / static_cast<double>(_eraseTotalMsec);
        setSubProgress(qMin(progress, 0.99));
        emit progressChanged(this->progress());
    });

    connect(this, &QStateMachine::runningChanged, this, [this]() {
        qCDebug(FirmwareUpgradeStateMachineLog) << "Running changed:" << isRunning();
    });
}

void FirmwareUpgradeStateMachine::_buildStateMachine()
{
    // Idle state - simple state, no async operation
    _idleState = new QGCState("Idle", this);
    connect(_idleState, &QAbstractState::entered, this, [this]() {
        qCDebug(FirmwareUpgradeStateMachineLog) << "Entered Idle state";
        _firmwareUrl.clear();
        _errorMessage.clear();
        _waitingForBootloader = false;
        _eraseProgressTimer.stop();
    });

    // WaitingForBoard - async wait for board detection
    _waitingForBoardState = new AsyncFunctionState(
        "WaitingForBoard", this,
        [this](AsyncFunctionState* state) { _setupWaitingForBoard(state); },
        _boardSearchTimeoutMsec
    );

    // WaitingForReplug - async wait for user to replug board
    _waitingForReplugState = new AsyncFunctionState(
        "WaitingForReplug", this,
        [this](AsyncFunctionState* state) { _setupWaitingForReplug(state); },
        0  // No timeout - wait indefinitely for user action
    );

    // ConnectingBootloader - async wait for bootloader connection
    _connectingBootloaderState = new AsyncFunctionState(
        "ConnectingBootloader", this,
        [this](AsyncFunctionState* state) { _setupConnectingBootloader(state); },
        _bootloaderTimeoutMsec
    );

    // DownloadingFirmware - async download
    _downloadingFirmwareState = new AsyncFunctionState(
        "DownloadingFirmware", this,
        [this](AsyncFunctionState* state) { _setupDownloadingFirmware(state); },
        _downloadTimeoutMsec
    );

    // ValidatingImage - async validation
    _validatingImageState = new AsyncFunctionState(
        "ValidatingImage", this,
        [this](AsyncFunctionState* state) { _setupValidatingImage(state); },
        _validateTimeoutMsec
    );

    // Erasing - async erase operation
    _erasingState = new AsyncFunctionState(
        "Erasing", this,
        [this](AsyncFunctionState* state) { _setupErasing(state); },
        _eraseTimeoutMsec
    );

    // Flashing - async flash operation
    _flashingState = new AsyncFunctionState(
        "Flashing", this,
        [this](AsyncFunctionState* state) { _setupFlashing(state); },
        _flashTimeoutMsec
    );

    // Rebooting - async wait for board disconnect
    _rebootingState = new AsyncFunctionState(
        "Rebooting", this,
        [this](AsyncFunctionState* state) { _setupRebooting(state); },
        _rebootTimeoutMsec
    );

    // Complete - function state that signals completion
    _completeState = new FunctionState("Complete", this, [this]() { _onComplete(); });

    // Error - function state that signals error
    _errorState = new FunctionState("Error", this, [this]() { _onError(); });

    // Final state
    _finalState = new QGCFinalState("Final", this);

    // Set initial state
    setInitialState(_idleState);

    // Set global error state for automatic error transitions
    setGlobalErrorState(_errorState);
}

void FirmwareUpgradeStateMachine::_wireTransitions()
{
    // Idle -> WaitingForBoard (on start_search event)
    _idleState->addTransition(new MachineEventTransition("start_search", _waitingForBoardState));

    // WaitingForBoard -> WaitingForReplug (on board_found_first)
    // This happens when board is first detected and needs replug
    _waitingForBoardState->addTransition(new MachineEventTransition("board_found_first", _waitingForReplugState));

    // WaitingForBoard -> ConnectingBootloader (on board_found_replug - direct bootloader connection)
    _waitingForBoardState->addTransition(new MachineEventTransition("board_found_replug", _connectingBootloaderState));

    // WaitingForBoard timeout -> Error
    _waitingForBoardState->addTransition(_waitingForBoardState, &WaitStateBase::timedOut, _errorState);

    // WaitingForReplug -> ConnectingBootloader (on completed - board was replugged)
    _waitingForReplugState->addTransition(_waitingForReplugState, &WaitStateBase::completed, _connectingBootloaderState);

    // ConnectingBootloader -> DownloadingFirmware (on start_flash event)
    _connectingBootloaderState->addTransition(new MachineEventTransition("start_flash", _downloadingFirmwareState));

    // ConnectingBootloader -> Idle (on show_firmware_select - no pending flash)
    _connectingBootloaderState->addTransition(new MachineEventTransition("show_firmware_select", _idleState));

    // ConnectingBootloader timeout -> Error
    _connectingBootloaderState->addTransition(_connectingBootloaderState, &WaitStateBase::timedOut, _errorState);

    // DownloadingFirmware -> ValidatingImage (on completed)
    _downloadingFirmwareState->addTransition(_downloadingFirmwareState, &WaitStateBase::completed, _validatingImageState);
    _downloadingFirmwareState->addTransition(_downloadingFirmwareState, &WaitStateBase::timedOut, _errorState);

    // ValidatingImage -> Erasing (on completed)
    _validatingImageState->addTransition(_validatingImageState, &WaitStateBase::completed, _erasingState);
    _validatingImageState->addTransition(_validatingImageState, &WaitStateBase::timedOut, _errorState);

    // Erasing -> Flashing (on completed)
    _erasingState->addTransition(_erasingState, &WaitStateBase::completed, _flashingState);
    _erasingState->addTransition(_erasingState, &WaitStateBase::timedOut, _errorState);

    // Flashing -> Rebooting (on completed)
    _flashingState->addTransition(_flashingState, &WaitStateBase::completed, _rebootingState);
    _flashingState->addTransition(_flashingState, &WaitStateBase::timedOut, _errorState);

    // Rebooting -> Complete (on completed)
    _rebootingState->addTransition(_rebootingState, &WaitStateBase::completed, _completeState);
    _rebootingState->addTransition(_rebootingState, &WaitStateBase::timedOut, _errorState);

    // Complete -> Final
    _completeState->addTransition(_completeState, &QGCState::advance, _finalState);

    // Error -> Final
    _errorState->addTransition(_errorState, &QGCState::advance, _finalState);

    // Cancel transitions - all async states can be cancelled
    for (auto* state : {_waitingForBoardState, _waitingForReplugState, _connectingBootloaderState,
                        _downloadingFirmwareState, _validatingImageState, _erasingState,
                        _flashingState, _rebootingState}) {
        state->addTransition(new MachineEventTransition("cancel", _idleState));
    }
}

void FirmwareUpgradeStateMachine::_wireProgressTracking()
{
    setProgressWeights({
        {_idleState, 0},
        {_waitingForBoardState, 0},
        {_waitingForReplugState, 0},
        {_connectingBootloaderState, 1},
        {_downloadingFirmwareState, 2},
        {_validatingImageState, 1},
        {_erasingState, 3},
        {_flashingState, 5},
        {_rebootingState, 1},
        {_completeState, 0},
        {_errorState, 0},
        {_finalState, 0}
    });
}

// -----------------------------------------------------------------------------
// State Setup Functions
// -----------------------------------------------------------------------------

void FirmwareUpgradeStateMachine::_setupWaitingForBoard(AsyncFunctionState* state)
{
    Q_UNUSED(state)
    qCDebug(FirmwareUpgradeStateMachineLog) << "Setting up WaitingForBoard";
    emit statusMessage(tr("Plug in your board via USB..."));
    // Board detection happens via handleBoardFound() callback
}

void FirmwareUpgradeStateMachine::_setupWaitingForReplug(AsyncFunctionState* state)
{
    Q_UNUSED(state)
    qCDebug(FirmwareUpgradeStateMachineLog) << "Setting up WaitingForReplug";
    emit statusMessage(tr("Board found. Please unplug and replug the board to enter bootloader mode."));
    // Replug detection happens via handleBoardFound() callback
}

void FirmwareUpgradeStateMachine::_setupConnectingBootloader(AsyncFunctionState* state)
{
    Q_UNUSED(state)
    qCDebug(FirmwareUpgradeStateMachineLog) << "Setting up ConnectingBootloader";
    emit statusMessage(tr("Connecting to bootloader..."));
    // Bootloader connection happens via handleBootloaderInfo() callback
}

void FirmwareUpgradeStateMachine::_setupDownloadingFirmware(AsyncFunctionState* state)
{
    Q_UNUSED(state)
    qCDebug(FirmwareUpgradeStateMachineLog) << "Setting up DownloadingFirmware";
    emit statusMessage(tr("Downloading firmware..."));
    _startDownload();
}

void FirmwareUpgradeStateMachine::_setupValidatingImage(AsyncFunctionState* state)
{
    Q_UNUSED(state)
    qCDebug(FirmwareUpgradeStateMachineLog) << "Setting up ValidatingImage";
    emit statusMessage(tr("Validating firmware image..."));
    _validateAndFlash(_localFirmwareFile);
}

void FirmwareUpgradeStateMachine::_setupErasing(AsyncFunctionState* state)
{
    Q_UNUSED(state)
    qCDebug(FirmwareUpgradeStateMachineLog) << "Setting up Erasing";
    emit statusMessage(tr("Erasing flash memory..."));
    // Erase is triggered by the thread controller after validation
    // We just wait for handleEraseComplete()
}

void FirmwareUpgradeStateMachine::_setupFlashing(AsyncFunctionState* state)
{
    Q_UNUSED(state)
    qCDebug(FirmwareUpgradeStateMachineLog) << "Setting up Flashing";
    emit statusMessage(tr("Programming firmware..."));
    // Flash happens via thread controller, we wait for handleFlashComplete()
}

void FirmwareUpgradeStateMachine::_setupRebooting(AsyncFunctionState* state)
{
    Q_UNUSED(state)
    qCDebug(FirmwareUpgradeStateMachineLog) << "Setting up Rebooting";
    emit statusMessage(tr("Rebooting board..."));
    // Reboot detection happens via handleBoardGone() callback
}

void FirmwareUpgradeStateMachine::_onComplete()
{
    qCDebug(FirmwareUpgradeStateMachineLog) << "Flash complete!";
    emit statusMessage(tr("Firmware upgrade complete!"));
    emit flashComplete(true, tr("Firmware upgrade complete"));
    LinkManager::instance()->setConnectionsAllowed();
}

void FirmwareUpgradeStateMachine::_onError()
{
    qCDebug(FirmwareUpgradeStateMachineLog) << "Flash error:" << _errorMessage;
    emit statusMessage(tr("Error: %1").arg(_errorMessage));
    emit flashComplete(false, _errorMessage);
    LinkManager::instance()->setConnectionsAllowed();
}

// -----------------------------------------------------------------------------
// Public Methods
// -----------------------------------------------------------------------------

void FirmwareUpgradeStateMachine::startBoardSearch()
{
    qCDebug(FirmwareUpgradeStateMachineLog) << "Starting board search";

    _firmwareUrl.clear();
    _errorMessage.clear();
    _waitingForBootloader = false;
    resetProgress();

    if (!isRunning()) {
        start();
    }

    postEvent("start_search");
}

void FirmwareUpgradeStateMachine::startFlash(const QString& firmwareUrl)
{
    qCDebug(FirmwareUpgradeStateMachineLog) << "Starting flash with URL:" << firmwareUrl;

    _firmwareUrl = firmwareUrl;
    _waitingForBootloader = true;

    emit flashStarted();

    if (isStateActive(_connectingBootloaderState) || isStateActive(_idleState)) {
        // Already connected to bootloader or idle, start flash immediately
        postEvent("start_flash");
    }
    // Otherwise, flash will start when bootloader is connected
}

void FirmwareUpgradeStateMachine::cancel()
{
    qCDebug(FirmwareUpgradeStateMachineLog) << "Cancelling operation";

    _eraseProgressTimer.stop();
    postEvent("cancel");
}

bool FirmwareUpgradeStateMachine::isFlashing() const
{
    return isStateActive(_downloadingFirmwareState) ||
           isStateActive(_validatingImageState) ||
           isStateActive(_erasingState) ||
           isStateActive(_flashingState) ||
           isStateActive(_rebootingState);
}

double FirmwareUpgradeStateMachine::flashProgress() const
{
    return progress();
}

// -----------------------------------------------------------------------------
// Thread Worker Callbacks
// -----------------------------------------------------------------------------

AsyncFunctionState* FirmwareUpgradeStateMachine::_currentAsyncState() const
{
    if (isStateActive(_waitingForBoardState)) return _waitingForBoardState;
    if (isStateActive(_waitingForReplugState)) return _waitingForReplugState;
    if (isStateActive(_connectingBootloaderState)) return _connectingBootloaderState;
    if (isStateActive(_downloadingFirmwareState)) return _downloadingFirmwareState;
    if (isStateActive(_validatingImageState)) return _validatingImageState;
    if (isStateActive(_erasingState)) return _erasingState;
    if (isStateActive(_flashingState)) return _flashingState;
    if (isStateActive(_rebootingState)) return _rebootingState;
    return nullptr;
}

void FirmwareUpgradeStateMachine::_failCurrentState(const QString& error)
{
    _errorMessage = error;
    if (auto* state = _currentAsyncState()) {
        state->fail();
    }
}

void FirmwareUpgradeStateMachine::handleBoardFound(bool firstAttempt, const QSerialPortInfo& portInfo,
                                                    int boardType, const QString& boardName)
{
    Q_UNUSED(portInfo)
    Q_UNUSED(boardType)
    Q_UNUSED(boardName)

    qCDebug(FirmwareUpgradeStateMachineLog) << "Board found, firstAttempt:" << firstAttempt;

    if (isStateActive(_waitingForBoardState)) {
        if (firstAttempt) {
            // First detection - need to unplug and replug
            postEvent("board_found_first");
        } else {
            // Board replug detected - go directly to bootloader
            postEvent("board_found_replug");
        }
    } else if (isStateActive(_waitingForReplugState)) {
        // Board was replugged - complete the wait
        _waitingForReplugState->complete();
    }
}

void FirmwareUpgradeStateMachine::handleBoardGone()
{
    qCDebug(FirmwareUpgradeStateMachineLog) << "Board gone";

    if (isStateActive(_rebootingState)) {
        // Board disconnect during reboot means success
        _rebootingState->complete();
    }
}

void FirmwareUpgradeStateMachine::handleNoBoardFound()
{
    qCDebug(FirmwareUpgradeStateMachineLog) << "No board found";

    _errorMessage = tr("No compatible board found. Make sure the board is plugged in via USB.");
    if (isStateActive(_waitingForBoardState)) {
        _waitingForBoardState->fail();
    }
}

void FirmwareUpgradeStateMachine::handleBootloaderInfo(int bootloaderVersion, int boardId, int flashSize)
{
    Q_UNUSED(bootloaderVersion)
    Q_UNUSED(boardId)
    Q_UNUSED(flashSize)

    qCDebug(FirmwareUpgradeStateMachineLog) << "Bootloader info received";

    if (_waitingForBootloader && !_firmwareUrl.isEmpty()) {
        // We have a pending flash request
        postEvent("start_flash");
    } else {
        // No pending flash - show firmware selection dialog
        postEvent("show_firmware_select");
    }
}

void FirmwareUpgradeStateMachine::handleEraseStarted()
{
    qCDebug(FirmwareUpgradeStateMachineLog) << "Erase started";

    _eraseTickCount = 0;
    _eraseProgressTimer.start(_eraseTickMsec);
}

void FirmwareUpgradeStateMachine::handleEraseComplete()
{
    qCDebug(FirmwareUpgradeStateMachineLog) << "Erase complete";

    _eraseProgressTimer.stop();
    setSubProgress(1.0);

    if (isStateActive(_erasingState)) {
        _erasingState->complete();
    }
}

void FirmwareUpgradeStateMachine::handleFlashProgress(int current, int total)
{
    if (total > 0) {
        double subProgress = static_cast<double>(current) / static_cast<double>(total);
        setSubProgress(subProgress);
        emit progressChanged(progress());
    }
}

void FirmwareUpgradeStateMachine::handleFlashComplete()
{
    qCDebug(FirmwareUpgradeStateMachineLog) << "Flash complete";

    setSubProgress(1.0);

    if (isStateActive(_flashingState)) {
        _flashingState->complete();
    }
}

void FirmwareUpgradeStateMachine::handleError(const QString& errorString)
{
    qCDebug(FirmwareUpgradeStateMachineLog) << "Error:" << errorString;

    _errorMessage = errorString;
    _eraseProgressTimer.stop();

    // Fail the current async state
    if (auto* state = _currentAsyncState()) {
        state->fail();
    }
}

void FirmwareUpgradeStateMachine::handleStatus(const QString& statusString)
{
    emit statusMessage(statusString);
}

void FirmwareUpgradeStateMachine::handleDownloadProgress(qint64 current, qint64 total)
{
    if (total > 0) {
        double subProgress = static_cast<double>(current) / static_cast<double>(total);
        setSubProgress(subProgress);
        emit progressChanged(progress());
    }
}

void FirmwareUpgradeStateMachine::handleDownloadComplete(const QString& localFile, const QString& errorMsg)
{
    if (!errorMsg.isEmpty()) {
        qCDebug(FirmwareUpgradeStateMachineLog) << "Download failed:" << errorMsg;
        _errorMessage = errorMsg;
        if (isStateActive(_downloadingFirmwareState)) {
            _downloadingFirmwareState->fail();
        }
        return;
    }

    qCDebug(FirmwareUpgradeStateMachineLog) << "Download complete:" << localFile;
    _localFirmwareFile = localFile;
    setSubProgress(1.0);

    if (isStateActive(_downloadingFirmwareState)) {
        _downloadingFirmwareState->complete();
    }
}

// -----------------------------------------------------------------------------
// Internal Helpers
// -----------------------------------------------------------------------------

void FirmwareUpgradeStateMachine::_startDownload()
{
    if (_firmwareUrl.isEmpty()) {
        _errorMessage = tr("No firmware URL specified");
        _downloadingFirmwareState->fail();
        return;
    }

    emit statusMessage(tr("Downloading from: %1").arg(_firmwareUrl));

    auto* downloader = new QGCFileDownload(this);
    connect(downloader, &QGCFileDownload::downloadComplete, this,
            [this](QString /*remoteFile*/, QString localFile, QString errorMsg) {
                handleDownloadComplete(localFile, errorMsg);
            });
    connect(downloader, &QGCFileDownload::downloadProgress, this,
            [this](qint64 curr, qint64 total) {
                handleDownloadProgress(curr, total);
            });
    downloader->download(_firmwareUrl);
}

void FirmwareUpgradeStateMachine::_validateAndFlash(const QString& localFile)
{
    auto* image = new FirmwareImage(_controller);

    connect(image, &FirmwareImage::statusMessage, this, &FirmwareUpgradeStateMachine::handleStatus);
    connect(image, &FirmwareImage::errorMessage, this, [this](const QString& msg) {
        handleError(msg);
    });

    if (!image->load(localFile, _controller->_bootloaderBoardID)) {
        _errorMessage = tr("Image load failed");
        _validatingImageState->fail();
        delete image;
        return;
    }

    // Check flash size
    if (_controller->_bootloaderBoardFlashSize != 0 &&
        image->imageSize() > _controller->_bootloaderBoardFlashSize) {
        _errorMessage = tr("Image size of %1 is too large for board flash size %2")
                            .arg(image->imageSize())
                            .arg(_controller->_bootloaderBoardFlashSize);
        _validatingImageState->fail();
        delete image;
        return;
    }

    // Image is valid - start flash via thread controller
    _controller->_image = image;
    _controller->_threadController->flash(image);

    // Complete validation state - we'll wait in erasing state for erase to complete
    _validatingImageState->complete();
}
