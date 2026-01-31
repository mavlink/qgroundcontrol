#pragma once

#include "QGCStateMachine.h"

class FirmwareUpgradeController;
class QSerialPortInfo;
class FirmwareImage;

/// State machine for firmware upgrade workflow using semantic state types.
///
/// States:
/// - Idle: Not flashing (QGCState)
/// - WaitingForBoard: User plugging in board (AsyncFunctionState)
/// - WaitingForReplug: Board found, asking user to unplug/replug (AsyncFunctionState)
/// - ConnectingBootloader: Finding bootloader after replug (AsyncFunctionState)
/// - DownloadingFirmware: Downloading firmware file (AsyncFunctionState)
/// - ValidatingImage: Loading and validating firmware image (AsyncFunctionState)
/// - Erasing: Erasing flash memory (AsyncFunctionState)
/// - Flashing: Programming flash (AsyncFunctionState)
/// - Rebooting: Rebooting into application (AsyncFunctionState)
/// - Complete: Flash successful (FunctionState -> FinalState)
/// - Error: Flash failed (FunctionState -> FinalState)
class FirmwareUpgradeStateMachine : public QGCStateMachine
{
    Q_OBJECT

public:
    explicit FirmwareUpgradeStateMachine(FirmwareUpgradeController* controller, QObject* parent = nullptr);
    ~FirmwareUpgradeStateMachine() override = default;

    /// Start searching for a board
    void startBoardSearch();

    /// Start the flash process with a firmware URL
    /// @param firmwareUrl URL or path to firmware file
    void startFlash(const QString& firmwareUrl);

    /// Cancel the current operation
    void cancel();

    /// @return true if a flash operation is in progress
    bool isFlashing() const;

    /// @return current flash progress (0.0 to 1.0)
    double flashProgress() const;

    // Thread worker callbacks (called from main thread via queued connections)

    /// Handle board found event from worker thread
    /// @param firstAttempt true if this is the first detection (need replug)
    /// @param portInfo Serial port info for the board
    /// @param boardType Type of board detected
    /// @param boardName Name of the board
    void handleBoardFound(bool firstAttempt, const QSerialPortInfo& portInfo,
                          int boardType, const QString& boardName);

    /// Handle board gone event (disconnected)
    void handleBoardGone();

    /// Handle no board found event
    void handleNoBoardFound();

    /// Handle bootloader info received
    /// @param bootloaderVersion Bootloader version
    /// @param boardId Board ID
    /// @param flashSize Flash size in bytes
    void handleBootloaderInfo(int bootloaderVersion, int boardId, int flashSize);

    /// Handle erase started event
    void handleEraseStarted();

    /// Handle erase complete event
    void handleEraseComplete();

    /// Handle flash progress update
    /// @param current Current bytes written
    /// @param total Total bytes to write
    void handleFlashProgress(int current, int total);

    /// Handle flash complete event
    void handleFlashComplete();

    /// Handle error from worker thread
    /// @param errorString Error message
    void handleError(const QString& errorString);

    /// Handle status message from worker thread
    /// @param statusString Status message
    void handleStatus(const QString& statusString);

    /// Handle firmware download progress
    /// @param current Current bytes downloaded
    /// @param total Total bytes to download
    void handleDownloadProgress(qint64 current, qint64 total);

    /// Handle firmware download complete
    /// @param localFile Path to downloaded file
    /// @param errorMsg Error message (empty on success)
    void handleDownloadComplete(const QString& localFile, const QString& errorMsg);

signals:
    void flashStarted();
    void flashComplete(bool success, const QString& message);
    void statusMessage(const QString& message);
    void progressChanged(double progress);

private:
    void _buildStateMachine();
    void _wireTransitions();
    void _wireProgressTracking();

    // State setup functions (called when state is entered)
    void _setupWaitingForBoard(AsyncFunctionState* state);
    void _setupWaitingForReplug(AsyncFunctionState* state);
    void _setupConnectingBootloader(AsyncFunctionState* state);
    void _setupDownloadingFirmware(AsyncFunctionState* state);
    void _setupValidatingImage(AsyncFunctionState* state);
    void _setupErasing(AsyncFunctionState* state);
    void _setupFlashing(AsyncFunctionState* state);
    void _setupRebooting(AsyncFunctionState* state);
    void _onComplete();
    void _onError();

    // Internal helpers
    void _startDownload();
    void _validateAndFlash(const QString& localFile);
    AsyncFunctionState* _currentAsyncState() const;
    void _failCurrentState(const QString& error);

    FirmwareUpgradeController* _controller;

    // Flash operation state
    QString _firmwareUrl;
    QString _errorMessage;
    QString _localFirmwareFile;
    bool _waitingForBootloader = false;

    // States using semantic types
    QGCState* _idleState = nullptr;
    AsyncFunctionState* _waitingForBoardState = nullptr;
    AsyncFunctionState* _waitingForReplugState = nullptr;
    AsyncFunctionState* _connectingBootloaderState = nullptr;
    AsyncFunctionState* _downloadingFirmwareState = nullptr;
    AsyncFunctionState* _validatingImageState = nullptr;
    AsyncFunctionState* _erasingState = nullptr;
    AsyncFunctionState* _flashingState = nullptr;
    AsyncFunctionState* _rebootingState = nullptr;
    FunctionState* _completeState = nullptr;
    FunctionState* _errorState = nullptr;
    QGCFinalState* _finalState = nullptr;

    // Timers for erase progress simulation
    QTimer _eraseProgressTimer;
    int _eraseTickCount = 0;

    // Constants
    static constexpr int _eraseTickMsec = 500;
    static constexpr int _eraseTotalMsec = 15000;
    static constexpr int _boardSearchTimeoutMsec = 30000;
    static constexpr int _bootloaderTimeoutMsec = 5000;
    static constexpr int _downloadTimeoutMsec = 300000;  // 5 minutes for large firmware
    static constexpr int _validateTimeoutMsec = 30000;
    static constexpr int _eraseTimeoutMsec = 60000;
    static constexpr int _flashTimeoutMsec = 300000;     // 5 minutes for large firmware
    static constexpr int _rebootTimeoutMsec = 10000;
};
