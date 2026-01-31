#pragma once

#include "QGCStateMachine.h"

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(AutotuneStateMachineLog)

class Autotune;
class Vehicle;

/// State machine for vehicle autotune operation.
///
/// Uses FunctionState for terminal states, QGCState for progress monitoring.
/// Progress states poll at regular intervals while waiting for external progress updates.
///
/// Manages the autotune workflow which progresses through:
/// - Initializing (0-20%)
/// - Roll tuning (20-40%)
/// - Pitch tuning (40-60%)
/// - Yaw tuning (60-80%)
/// - Wait for disarm (80-100%)
/// - Success/Failed
class AutotuneStateMachine : public QGCStateMachine
{
    Q_OBJECT

public:
    explicit AutotuneStateMachine(Autotune* autotune, Vehicle* vehicle, QObject* parent = nullptr);
    ~AutotuneStateMachine() override;

    /// Start the autotune operation
    void startAutotune();

    /// Handle progress update from MAVLink ack
    /// @param progress Progress value 0-100
    void handleProgress(uint8_t progress);

    /// Handle autotune failure
    void handleFailure();

    /// Handle autotune error with specific code
    void handleError(uint8_t errorCode);

    /// @return true if autotune is currently in progress
    bool isInProgress() const { return _inProgress; }

    /// @return current progress (0.0 to 1.0)
    float tuneProgress() const { return _progress; }

    /// @return current status string
    QString statusString() const { return _statusString; }

signals:
    /// Emitted when autotune state changes
    void autotuneChanged();

    /// Emitted when autotune completes (success or failure)
    void autotuneComplete(bool success);

    /// Emitted when user should be notified to disarm
    void disarmRequired();

private slots:
    void _sendAutotuneCommand();

private:
    void _buildStateMachine();
    void _wireTransitions();
    void _updateStatusForProgress(uint8_t progress);

    // State setup functions
    void _onInitializingEntered();
    void _onRollEntered();
    void _onPitchEntered();
    void _onYawEntered();
    void _onWaitForDisarmEntered();
    void _onSuccess();
    void _onFailed();

    Autotune* _autotune;
    Vehicle* _vehicle;

    // Progress monitoring states (poll and wait for external events)
    QGCState* _idleState = nullptr;
    QGCState* _initializingState = nullptr;
    QGCState* _rollState = nullptr;
    QGCState* _pitchState = nullptr;
    QGCState* _yawState = nullptr;
    QGCState* _waitForDisarmState = nullptr;

    // Terminal states using semantic types
    FunctionState* _successState = nullptr;
    FunctionState* _failedState = nullptr;
    QGCFinalState* _finalState = nullptr;

    // State tracking
    bool _inProgress = false;
    float _progress = 0.0f;
    QString _statusString;
    bool _disarmMessageDisplayed = false;

    static constexpr int _pollIntervalMs = 1000;
};
