#pragma once

#include "QGCStateMachine.h"
#include "MAVLinkLib.h"

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(InitialConnectStateMachineLog)

class Vehicle;
class SkippableAsyncState;
class AsyncFunctionState;
class RetryableRequestMessageState;
class RetryState;

/// State machine for initial vehicle connection sequence.
/// Handles requesting autopilot version, standard modes, component info,
/// parameters, missions, geofence, and rally points in sequence.
///
/// Uses QGCStateMachine's built-in weighted progress tracking where different
/// states contribute different amounts to the overall progress (e.g., parameter
/// loading takes longer than version request).
class InitialConnectStateMachine : public QGCStateMachine
{
    Q_OBJECT

public:
    explicit InitialConnectStateMachine(Vehicle* vehicle, QObject* parent = nullptr);
    ~InitialConnectStateMachine() override;

    void start();

private slots:
    void _onSubProgressUpdate(double progressValue);

private:
    // State creation and wiring
    void _createStates();
    void _wireTransitions();
    void _wireProgressTracking();
    void _wireTimeoutHandling();

    // State callbacks
    void _handleAutopilotVersionSuccess(const mavlink_message_t& message);
    void _handleAutopilotVersionFailure();
    void _requestStandardModes(AsyncFunctionState* state);
    void _requestCompInfo(AsyncFunctionState* state);
    void _requestParameters(AsyncFunctionState* state);
    void _onParametersReady(bool ready);
    void _requestMission(SkippableAsyncState* state);
    void _requestGeoFence(SkippableAsyncState* state);
    void _requestRallyPoints(SkippableAsyncState* state);
    void _signalComplete();

    // Skip predicates
    bool _shouldSkipAutopilotVersionRequest() const;
    bool _shouldSkipForLinkType() const;
    bool _hasPrimaryLink() const;

    // State pointers for wiring
    RetryableRequestMessageState* _stateAutopilotVersion = nullptr;
    AsyncFunctionState* _stateStandardModes = nullptr;
    AsyncFunctionState* _stateCompInfo = nullptr;
    AsyncFunctionState* _stateParameters = nullptr;
    SkippableAsyncState* _stateMission = nullptr;
    SkippableAsyncState* _stateGeoFence = nullptr;
    SkippableAsyncState* _stateRallyPoints = nullptr;
    RetryState* _stateComplete = nullptr;
    QGCFinalState* _stateFinal = nullptr;

    // Timeout handling with retry
    static constexpr int _maxRetries = 1;

    // Timeout values (ms)
    static constexpr int _timeoutAutopilotVersion = 5000;
    static constexpr int _timeoutStandardModes = 5000;
    static constexpr int _timeoutCompInfo = 30000;
    static constexpr int _timeoutParameters = 60000;
    static constexpr int _timeoutMission = 30000;
    static constexpr int _timeoutGeoFence = 15000;
    static constexpr int _timeoutRallyPoints = 15000;
};
