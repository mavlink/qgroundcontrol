#pragma once

#include "QGCStateMachine.h"

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(ESP8266StateMachineLog)

class ESP8266ComponentController;

/// State machine for ESP8266 WiFi module operations.
///
/// Handles two operations with retry logic:
/// - Reboot: Send reboot command, wait for ack
/// - Restore Defaults: Send restore command, wait for ack, refresh parameters
///
/// State Diagram:
///
/// ```
///     ┌────────────┐
///     │    Idle    │◄────────────────────────────┐
///     └─────┬──────┘                             │
///           │                                    │
///   ┌───────┴───────┐                            │
///   │               │                            │
///   ▼               ▼                            │
/// ┌─────────────┐ ┌─────────────┐                │
/// │   Rebooting │ │  Restoring  │                │
/// └──────┬──────┘ └──────┬──────┘                │
///        │               │                       │
///   (ack/timeout)   (ack/timeout)                │
///        │               │                       │
///        └───────┬───────┘                       │
///                │                               │
///                ▼                               │
///         ┌──────────┐                           │
///         │ Complete │───────────────────────────┘
///         └──────────┘
/// ```
class ESP8266StateMachine : public QGCStateMachine
{
    Q_OBJECT

public:
    explicit ESP8266StateMachine(ESP8266ComponentController* controller, QObject* parent = nullptr);
    ~ESP8266StateMachine() override = default;

    /// Start a reboot operation
    void startReboot();

    /// Start a restore defaults operation
    void startRestore();

    /// Handle MAVLink command result
    /// @param command The MAVLink command ID
    /// @param result The command result (MAV_RESULT_*)
    void handleCommandResult(int command, int result);

    /// @return true if an operation is in progress
    bool isBusy() const;

signals:
    /// Emitted when an operation completes
    /// @param success true if operation succeeded
    void operationComplete(bool success);

    /// Emitted when busy state changes
    void busyChanged();

private:
    void _buildStateMachine();
    void _setupTransitions();
    void _sendRebootCommand();
    void _sendRestoreCommand();

    ESP8266ComponentController* _controller;

    // States
    QGCState* _idleState = nullptr;
    QGCState* _rebootingState = nullptr;
    QGCState* _restoringState = nullptr;
    QGCFinalState* _completeState = nullptr;

    // Retry tracking
    int _retryCount = 0;
    static constexpr int _maxRetries = 5;
    static constexpr int _commandTimeoutMs = 5000;

    // Track which operation is in progress for completion handling
    enum class Operation {
        None,
        Reboot,
        Restore
    };
    Operation _currentOperation = Operation::None;
};
