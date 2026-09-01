#pragma once

#include "QGCStateMachine.h"
#include "MAVLinkMessageType.h"

class Vehicle;
class SkippableAsyncState;
class AsyncFunctionState;
class RetryableRequestMessageState;
class RetryState;

/// \brief State machine for initial vehicle connection sequence.
///
/// Handles requesting autopilot version, standard modes, component info,
/// parameters, missions, geofence, and rally points in sequence.
///
/// Uses QGCStateMachine's built-in weighted progress tracking where different
/// states contribute different amounts to the overall progress (e.g., parameter
/// loading takes longer than version request).
///
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

    // State callbacks
    void _handleAutopilotVersionSuccess(const mavlink_message_t& message);
    void _handleAutopilotVersionFailure();
    void _requestStandardModes(AsyncFunctionState* state);
    void _requestCompInfo(AsyncFunctionState* state);
    void _requestParameters(SkippableAsyncState* state);
    void _onParametersReady(bool ready);
    void _requestMission(SkippableAsyncState* state);
    void _requestGeoFence(SkippableAsyncState* state);
    void _requestRallyPoints(SkippableAsyncState* state);
    void _signalComplete();

    // Skip predicates
    bool _shouldSkipAutopilotVersionRequest() const;
    bool _shouldSkipForFlying() const;
    bool _shouldSkipForLinkType() const;
    bool _hasPrimaryLink() const;
    bool _shouldSkipForPlanLoad();

    QString _lastSkipReason;

    // State pointers for wiring
    RetryableRequestMessageState* _stateAutopilotVersion = nullptr;
    AsyncFunctionState* _stateStandardModes = nullptr;
    AsyncFunctionState* _stateCompInfo = nullptr;
    SkippableAsyncState* _stateParameters = nullptr;
    SkippableAsyncState* _stateMission = nullptr;
    SkippableAsyncState* _stateGeoFence = nullptr;
    SkippableAsyncState* _stateRallyPoints = nullptr;
    RetryState* _stateComplete = nullptr;
    QGCFinalState* _stateFinal = nullptr;

    static constexpr int _autopilotVersionMaxRetries = 1;
};
