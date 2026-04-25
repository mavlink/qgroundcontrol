#pragma once

#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QPointer>
#include <QtQmlIntegration/QtQmlIntegration>
#include <QtStateMachine/QState>

#include "QGCStateMachine.h"
#include "VideoReceiver.h"
#include "VideoStreamFsmState.h"

Q_DECLARE_LOGGING_CATEGORY(VideoStreamStateMachineLog)

/// Owning state machine for a single video stream.
///
/// Drives transitions from the primitive lifecycle signals emitted by every
/// VideoReceiver implementation:
///
///   receiverStarted / receiverStopped / receiverPaused / receiverResumed /
///   receiverFirstFrame / receiverError(ErrorCategory, QString)
///
/// Replaces the ad-hoc boolean soup of `_sessionState`, `_isLegalTransition`,
/// and `VideoRestartPolicy` that previously lived on VideoStream.
///
/// Rebinding contract: `bind(r)` requires the machine to be in Idle. Callers
/// flush through Stopping first via `requestStop()`. `rebind()` helpers at
/// the VideoStream level will sequence that drain.
class VideoStreamStateMachine : public QGCStateMachine
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    /// Public state vocabulary. `Connected` distinguishes "pipeline up, no
    /// frames yet" from `Streaming` ("frames flowing"); the split trips on the
    /// first `receiverFirstFrame()`. `Reconnecting` is the retry-loop anchor.
    ///
    /// The enum itself lives in `VideoStreamFsm::State` so headers that only
    /// need the type (VideoStream.h's Q_PROPERTY) don't transitively pull in
    /// QGCStateMachine.h.
    using State = VideoStreamFsm::State;

    /// Per-role tunables. VideoStream constructs a Policy for each stream role
    /// (dynamic streams override defaults to disallow soft reconnect and use a
    /// longer circuit-breaker reset window).
    struct Policy
    {
        int startingTimeoutMs = 10000;
        int stoppingTimeoutMs = 3000;
        int circuitFailureThreshold = 3;
        int circuitResetTimeoutMs = 8000;
        /// When false, Fatal receiver errors jump straight to Failed instead of
        /// routing through Reconnecting. Dynamic streams want this off so a
        /// caller-supplied bogus URI fails fast rather than looping.
        bool allowSoftReconnect = true;
        /// Seconds passed to VideoReceiver::start() from the Starting onEntry.
        int startTimeoutS = 3;
    };

    explicit VideoStreamStateMachine(const QString& machineName, Policy policy, QObject* parent = nullptr);
    ~VideoStreamStateMachine() override;

    [[nodiscard]] State state() const { return _currentState; }
    [[nodiscard]] const Policy& policy() const { return _policy; }
    [[nodiscard]] VideoReceiver* receiver() const { return _receiver.data(); }

    /// Override the start timeout. Called by VideoStream before requestStart()
    /// to set the per-URI timeout (RTSP = 8s, others = 3s).
    void setStartTimeoutS(int seconds) { _policy.startTimeoutS = seconds; }

    /// Attach a receiver. Must be called from Idle (machine may be running or
    /// stopped). Returns false if the machine is not Idle; callers flush via
    /// `requestStop()` first.
    bool bind(VideoReceiver* receiver);

    /// Detach the current receiver. Idempotent. Returns false if the machine
    /// is not Idle.
    bool unbind();

    /// Route a receiver-equivalent error into the FSM. This is used both for
    /// concrete VideoReceiver errors and sidecar ingest errors that affect the
    /// same stream lifecycle.
    void handleReceiverError(VideoReceiver::ErrorCategory category, const QString& message);

public slots:
    /// External entry points. These fan out to the FSM via proxy signals so
    /// that `QSignalTransition`s keep a stable sender (`this`) across
    /// receiver rebinds.
    void requestStart();
    void requestStop();
    void requestPause();
    void requestResume();

signals:
    /// Emitted on every entered-state transition.
    void stateChanged(VideoStreamStateMachine::State newState);

    /// Emitted when Connected is entered: receiver is up, setStarted(true) and
    /// startDecoding() have been called.
    void receiverFullyStarted();

    /// Emitted when Idle is entered after a stop cycle: setStarted(false) has
    /// been called and the receiver is quiescent.
    void receiverFullyStopped();

    /// Emitted when Reconnecting is entered so VideoStream can schedule a
    /// retry via VideoRestartPolicy.
    void reconnectRequested();

    // -- Internal proxy signals (avoid direct use; exposed for transition wiring) --
    void _startRequested();
    void _stopRequested();
    void _pauseRequested();
    void _resumeRequested();

    void _receiverStartedProxy();
    void _receiverStoppedProxy();
    void _receiverPausedProxy();
    void _receiverResumedProxy();
    void _receiverFirstFrameProxy();
    /// Fired when receiverError arrives with `ErrorCategory::Fatal` and
    /// `policy.allowSoftReconnect` is true — the FSM routes to Reconnecting.
    void _receiverSoftFailureProxy();
    /// Fired when receiverError arrives with Fatal and soft reconnect is
    /// disabled, or when the circuit breaker opens (PR #4). Routes to Failed.
    void _receiverHardFailureProxy();

private:
    void _createStates();
    void _wireTransitions();
    void _setCurrentState(State newState);
    Policy _policy;
    QPointer<VideoReceiver> _receiver;
    State _currentState = State::Idle;
    /// True once the FSM has entered Starting at least since the last Idle
    /// entry — gates Idle-onEntry side effects so the FSM's initial entry to
    /// Idle (which happens on QStateMachine::start() before any requestStart)
    /// does NOT emit receiverFullyStopped and trigger a spurious reconnect.
    bool _sawStarting = false;

    // State pointers (owned by QStateMachine).
    QState* _sIdle = nullptr;
    QState* _sStarting = nullptr;
    QState* _sConnected = nullptr;
    QState* _sStreaming = nullptr;
    QState* _sPaused = nullptr;
    QState* _sReconnecting = nullptr;
    QState* _sStopping = nullptr;
    QAbstractState* _sFailed = nullptr;  ///< QGCFinalState — abstract-typed so the member accepts it

    // Receiver-side connections — torn down on unbind().
    QList<QMetaObject::Connection> _receiverConns;
};
