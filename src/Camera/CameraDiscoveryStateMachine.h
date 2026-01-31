#pragma once

#include "QGCStateMachine.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(CameraDiscoveryStateMachineLog)

class QGCCameraManager;
class Vehicle;

/// State machine for discovering a single camera via MAVLink.
///
/// Uses semantic state types for camera info request protocol with retry and exponential backoff:
/// - AsyncFunctionState for request operations
/// - DelayState for backoff waits
/// - FunctionState for completion handlers
///
/// State Diagram:
///
/// ```
///     ┌────────────┐
///     │    Idle    │
///     └─────┬──────┘
///           │ (start)
///           ▼
///     ┌─────────────────┐
///     │ RequestingInfo  │◄───────────────┐
///     └────────┬────────┘                │
///              │                         │
///       (completed)              (completed after delay)
///       (failed/timeout)                 │
///              │                         │
///     ┌────────┴────────┐                │
///     │                 │                │
///     ▼                 ▼                │
/// ┌────────┐    ┌───────────────┐        │
/// │Complete│    │WaitingBackoff │────────┘
/// └────────┘    └───────┬───────┘
///                       │ (max_retries)
///                       ▼
///                 ┌──────────┐
///                 │  Failed  │
///                 └──────────┘
/// ```
class CameraDiscoveryStateMachine : public QGCStateMachine
{
    Q_OBJECT

public:
    explicit CameraDiscoveryStateMachine(QGCCameraManager* manager, uint8_t compId, Vehicle* vehicle, QObject* parent = nullptr);
    ~CameraDiscoveryStateMachine() override;

    /// Start the discovery process
    void startDiscovery();

    /// Called when camera info is received
    void handleInfoReceived();

    /// Called when a request fails
    void handleRequestFailed();

    /// Called when a heartbeat is received from this camera
    void handleHeartbeat();

    /// @return true if info has been received for this camera
    bool isInfoReceived() const { return _infoReceived; }

    /// @return Component ID of this camera
    uint8_t compId() const { return _compId; }

    /// @return Time since last heartbeat in milliseconds
    qint64 timeSinceLastHeartbeat() const { return _lastHeartbeat.elapsed(); }

    /// @return true if camera has been silent too long
    bool isTimedOut() const;

signals:
    /// Emitted when camera info is successfully received
    void discoveryComplete(uint8_t compId);

    /// Emitted when discovery fails after max retries
    void discoveryFailed(uint8_t compId);

private:
    void _buildStateMachine();
    void _wireTransitions();

    // State setup functions
    void _setupRequestingInfo(AsyncFunctionState* state);
    void _onComplete();
    void _onFailed();

    void _sendInfoRequest();
    int _calculateBackoffMs() const;

    QGCCameraManager* _manager;
    Vehicle* _vehicle;
    uint8_t _compId;

    // States using semantic types
    QGCState* _idleState = nullptr;
    AsyncFunctionState* _requestingInfoState = nullptr;
    QGCState* _waitingBackoffState = nullptr;  // Uses dynamic timeout for exponential backoff
    FunctionState* _completeState = nullptr;
    FunctionState* _failedState = nullptr;
    QGCFinalState* _finalState = nullptr;

    QTimer _backoffTimer;

    // State tracking
    bool _infoReceived = false;
    int _retryCount = 0;
    QElapsedTimer _lastHeartbeat;

    static constexpr int _maxRetryCount = 10;
    static constexpr int _silentTimeoutMs = 5000;
    static constexpr int _requestTimeoutMs = 3000;
    static constexpr int _baseBackoffMs = 1000;
};
