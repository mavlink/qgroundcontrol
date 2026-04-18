#include "VideoStreamStateMachine.h"

#include <QtCore/QMetaEnum>
#include <QtStateMachine/QState>

#include "QGCAbstractState.h"
#include "QGCFinalState.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(VideoStreamStateMachineLog, "Video.StateMachine")

VideoStreamStateMachine::VideoStreamStateMachine(const QString& machineName, Policy policy, QObject* parent)
    : QGCStateMachine(machineName, /*vehicle=*/nullptr, parent), _policy(std::move(policy))
{
    _createStates();
    _wireTransitions();
    setInitialState(_sIdle);
}

VideoStreamStateMachine::~VideoStreamStateMachine() = default;

void VideoStreamStateMachine::_createStates()
{
    // Plain QStates — we do not need QGCState's callback machinery for the
    // skeleton. PR #4 may swap specific states (e.g. Reconnecting →
    // CircuitBreakerState) without changing the surrounding wiring.
    _sIdle = new QState(this);
    _sIdle->setObjectName(QStringLiteral("Idle"));
    _sStarting = new QState(this);
    _sStarting->setObjectName(QStringLiteral("Starting"));
    _sConnected = new QState(this);
    _sConnected->setObjectName(QStringLiteral("Connected"));
    _sStreaming = new QState(this);
    _sStreaming->setObjectName(QStringLiteral("Streaming"));
    _sPaused = new QState(this);
    _sPaused->setObjectName(QStringLiteral("Paused"));
    _sReconnecting = new QState(this);
    _sReconnecting->setObjectName(QStringLiteral("Reconnecting"));
    _sStopping = new QState(this);
    _sStopping->setObjectName(QStringLiteral("Stopping"));
    _sFailed = new QGCFinalState(QStringLiteral("Failed"), this);

    // stateChanged emissions: drive from Qt's entered() signal so the public
    // enum always matches the active QState. No manual bookkeeping at the
    // transition sites.
    struct Mapping
    {
        QAbstractState* state;
        State value;
    };
    const Mapping mappings[] = {
        {_sIdle, State::Idle},       {_sStarting, State::Starting},
        {_sConnected, State::Connected},   {_sStreaming, State::Streaming},
        {_sPaused, State::Paused},       {_sReconnecting, State::Reconnecting},
        {_sStopping, State::Stopping}, {_sFailed, State::Failed},
    };
    for (const auto& m : mappings) {
        connect(m.state, &QAbstractState::entered, this, [this, v = m.value]() { _setCurrentState(v); });
    }

    // onEntry side-effects — the FSM drives the receiver, not the other way around.

    // Starting → call receiver->start(). The receiver will emit receiverStarted
    // (→ Connected) or receiverError/timeout (→ Reconnecting/Failed).
    connect(_sStarting, &QState::entered, this, [this]() {
        if (_receiver)
            _receiver->start(static_cast<uint32_t>(_policy.startTimeoutS));
    });

    // Connected → pipeline is up; arm the receiver for frame delivery.
    connect(_sConnected, &QState::entered, this, [this]() {
        if (_receiver) {
            _receiver->setStarted(true);
            _receiver->startDecoding();
        }
        emit receiverFullyStarted();
    });

    // Stopping → call receiver->stop(). The receiver will emit receiverStopped
    // (→ Idle) or the watchdog timeout transitions us to Idle.
    connect(_sStopping, &QState::entered, this, [this]() {
        if (_receiver)
            _receiver->stop();
    });

    // Idle → receiver is quiescent; clear the started flag and notify VideoStream.
    // Only emit receiverFullyStopped if the FSM was previously past Starting —
    // on the initial machine entry `_sawStarting` is false so no spurious
    // reconnect fires (the initial Idle entry happens before the user calls
    // requestStart, yet `_receiver` is already bound).
    connect(_sIdle, &QState::entered, this, [this]() {
        if (!_sawStarting)
            return;
        _sawStarting = false;
        if (_receiver) {
            _receiver->setStarted(false);
            emit receiverFullyStopped();
        }
    });
    connect(_sStarting, &QState::entered, this, [this]() { _sawStarting = true; });

    // Reconnecting → let VideoStream schedule a retry via VideoRestartPolicy.
    connect(_sReconnecting, &QState::entered, this, [this]() {
        emit reconnectRequested();
    });
}

void VideoStreamStateMachine::_wireTransitions()
{
    // Idle → Starting ─ user asked the stream to start.
    _sIdle->addTransition(this, &VideoStreamStateMachine::_startRequested, _sStarting);

    // Starting transitions
    _sStarting->addTransition(this, &VideoStreamStateMachine::_receiverStartedProxy, _sConnected);
    _sStarting->addTransition(this, &VideoStreamStateMachine::_receiverSoftFailureProxy, _sReconnecting);
    _sStarting->addTransition(this, &VideoStreamStateMachine::_receiverHardFailureProxy, _sFailed);
    _sStarting->addTransition(this, &VideoStreamStateMachine::_stopRequested, _sStopping);
    (void)addTimeoutTransition(_sStarting, _policy.startingTimeoutMs, _sFailed);

    // Connected transitions
    _sConnected->addTransition(this, &VideoStreamStateMachine::_receiverFirstFrameProxy, _sStreaming);
    _sConnected->addTransition(this, &VideoStreamStateMachine::_receiverPausedProxy, _sPaused);
    _sConnected->addTransition(this, &VideoStreamStateMachine::_receiverSoftFailureProxy, _sReconnecting);
    _sConnected->addTransition(this, &VideoStreamStateMachine::_receiverHardFailureProxy, _sFailed);
    _sConnected->addTransition(this, &VideoStreamStateMachine::_stopRequested, _sStopping);
    _sConnected->addTransition(this, &VideoStreamStateMachine::_pauseRequested, _sPaused);

    // Streaming transitions
    _sStreaming->addTransition(this, &VideoStreamStateMachine::_receiverPausedProxy, _sPaused);
    _sStreaming->addTransition(this, &VideoStreamStateMachine::_receiverSoftFailureProxy, _sReconnecting);
    _sStreaming->addTransition(this, &VideoStreamStateMachine::_receiverHardFailureProxy, _sFailed);
    _sStreaming->addTransition(this, &VideoStreamStateMachine::_stopRequested, _sStopping);
    _sStreaming->addTransition(this, &VideoStreamStateMachine::_pauseRequested, _sPaused);

    // Paused transitions ─ resume always returns to Streaming. (If the
    // resume landed on a pipeline that hadn't delivered frames yet we'd still
    // prefer Streaming because pause only arrives from Connected/Streaming and
    // the visible distinction is the presence of frames — the FSM treats
    // resume as "frames will continue".)
    _sPaused->addTransition(this, &VideoStreamStateMachine::_receiverResumedProxy, _sStreaming);
    _sPaused->addTransition(this, &VideoStreamStateMachine::_resumeRequested, _sStreaming);
    _sPaused->addTransition(this, &VideoStreamStateMachine::_receiverSoftFailureProxy, _sReconnecting);
    _sPaused->addTransition(this, &VideoStreamStateMachine::_receiverHardFailureProxy, _sFailed);
    _sPaused->addTransition(this, &VideoStreamStateMachine::_stopRequested, _sStopping);

    // Reconnecting transitions ─ bounce back to Connected on the next
    // receiverStarted, fall through to Failed on hard failure, respect
    // stopRequested, and re-drive the receiver when VideoStream schedules
    // a retry via reconnectRequested → requestStart() (Reconnecting → Starting
    // re-enters the Starting onEntry which calls receiver->start()).
    _sReconnecting->addTransition(this, &VideoStreamStateMachine::_receiverStartedProxy, _sConnected);
    _sReconnecting->addTransition(this, &VideoStreamStateMachine::_receiverHardFailureProxy, _sFailed);
    _sReconnecting->addTransition(this, &VideoStreamStateMachine::_stopRequested, _sStopping);
    _sReconnecting->addTransition(this, &VideoStreamStateMachine::_startRequested, _sStarting);

    // Stopping transitions ─ either the receiver confirms, or the watchdog
    // timeout punches us back to Idle to prevent a stuck receiver from
    // pinning the stream.
    _sStopping->addTransition(this, &VideoStreamStateMachine::_receiverStoppedProxy, _sIdle);
    (void)addTimeoutTransition(_sStopping, _policy.stoppingTimeoutMs, _sIdle);
}

void VideoStreamStateMachine::_setCurrentState(State newState)
{
    if (newState == _currentState)
        return;
    const auto meta = QMetaEnum::fromType<State>();
    qCDebug(VideoStreamStateMachineLog) << machineName() << meta.valueToKey(static_cast<int>(_currentState)) << "->"
                                        << meta.valueToKey(static_cast<int>(newState));
    _currentState = newState;
    emit stateChanged(newState);
}

bool VideoStreamStateMachine::bind(VideoReceiver* receiver)
{
    if (_currentState != State::Idle) {
        qCWarning(VideoStreamStateMachineLog) << machineName() << "bind() rejected — not Idle (state="
                                              << static_cast<int>(_currentState) << ")";
        return false;
    }

    // Detach any prior receiver first.
    if (_receiver) {
        for (auto& c : _receiverConns)
            disconnect(c);
        _receiverConns.clear();
        _receiver.clear();
    }

    if (!receiver)
        return true;

    _receiver = receiver;
    _receiverConns.reserve(6);
    _receiverConns << connect(receiver, &VideoReceiver::receiverStarted, this,
                              &VideoStreamStateMachine::_receiverStartedProxy);
    _receiverConns << connect(receiver, &VideoReceiver::receiverStopped, this,
                              &VideoStreamStateMachine::_receiverStoppedProxy);
    _receiverConns << connect(receiver, &VideoReceiver::receiverPaused, this,
                              &VideoStreamStateMachine::_receiverPausedProxy);
    _receiverConns << connect(receiver, &VideoReceiver::receiverResumed, this,
                              &VideoStreamStateMachine::_receiverResumedProxy);
    _receiverConns << connect(receiver, &VideoReceiver::receiverFirstFrame, this,
                              &VideoStreamStateMachine::_receiverFirstFrameProxy);
    _receiverConns << connect(receiver, &VideoReceiver::receiverError, this,
                              &VideoStreamStateMachine::_forwardReceiverError);
    return true;
}

bool VideoStreamStateMachine::unbind()
{
    if (_currentState != State::Idle) {
        qCWarning(VideoStreamStateMachineLog) << machineName() << "unbind() rejected — not Idle";
        return false;
    }
    for (auto& c : _receiverConns)
        disconnect(c);
    _receiverConns.clear();
    _receiver.clear();
    return true;
}

// requestX() queues the proxy signal through the event loop so callers can
// invoke it immediately after `_fsm->start()`. QStateMachine's initial state
// entry is itself a posted event; using Qt::QueuedConnection here guarantees
// the signal fires *after* the FSM has entered Idle, so QSignalTransitions
// on the initial state capture it.
void VideoStreamStateMachine::requestStart()
{
    QMetaObject::invokeMethod(this, [this]() { emit _startRequested(); }, Qt::QueuedConnection);
}

void VideoStreamStateMachine::requestStop()
{
    QMetaObject::invokeMethod(this, [this]() { emit _stopRequested(); }, Qt::QueuedConnection);
}

void VideoStreamStateMachine::requestPause()
{
    QMetaObject::invokeMethod(this, [this]() { emit _pauseRequested(); }, Qt::QueuedConnection);
}

void VideoStreamStateMachine::requestResume()
{
    QMetaObject::invokeMethod(this, [this]() { emit _resumeRequested(); }, Qt::QueuedConnection);
}

void VideoStreamStateMachine::_forwardReceiverError(VideoReceiver::ErrorCategory category, const QString& message)
{
    Q_UNUSED(message)
    // Only Fatal counts as a failure for FSM purposes — Transient / MissingPlugin /
    // HardwareFallback are informational and do not tear down the stream.
    if (category != VideoReceiver::ErrorCategory::Fatal)
        return;

    if (_policy.allowSoftReconnect)
        emit _receiverSoftFailureProxy();
    else
        emit _receiverHardFailureProxy();
}
