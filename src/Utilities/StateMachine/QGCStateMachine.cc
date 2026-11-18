#include "QGCStateMachine.h"
#include "Vehicle.h"

QGCStateMachine::QGCStateMachine(const QString& machineName, Vehicle *vehicle, QObject* parent)
    : QStateMachine (parent)
    , _vehicle      (vehicle)
{
    setObjectName(machineName);

    if (_vehicle) {
        // States check vehicle() for null, so a machine that outlives its vehicle must not keep the pointer
        connect(_vehicle, &QObject::destroyed, this, [this]() { _vehicle = nullptr; });
    }

    connect(this, &QGCStateMachine::started, this, [this] () {
        qCDebug(QGCStateMachineLog) << "State machine started:" << objectName();
        if (!_inProgress) {
            qCCritical(QGCStateMachineLog)
                << objectName()
                << "started without QGCStateMachine::start() - QStateMachine::start() or setRunning() was used";
            _setInProgress(true);
        }
    });
    // Cleared before anything else reacts to stopped/finished so handlers, including the final progress update,
    // already see the machine as no longer in progress
    connect(this, &QGCStateMachine::stopped, this, [this]() {
        _setInProgress(false);
        qCDebug(QGCStateMachineLog) << "State machine stopped:" << objectName();
    });
    connect(this, &QGCStateMachine::finished, this, [this]() {
        _setInProgress(false);
        qCDebug(QGCStateMachineLog) << "State machine finished:" << objectName();
        // Not when a listener of inProgressChanged has restarted the machine: the progress is that of the new run
        if (!_inProgress && _progressTotalWeight > 0 && _progressLastEmitted < 1.0f) {
            _progressLastEmitted = 1.0f;
            emit progressUpdate(1.0f);
        }
    });
}

void QGCStateMachine::start()
{
    if (_inProgress) {
        qCCritical(QGCStateMachineLog) << objectName()
                                       << "start() called but already in progress - check signal connections";
        return;
    }
    if ((childMode() == QState::ExclusiveStates) && !initialState()) {
        qCCritical(QGCStateMachineLog) << objectName() << "start() called without an initial state";
        return;
    }
    _inProgress = true;
    QStateMachine::start();
    // Emitted after Qt's start so a stop() from a listener is honored
    emit inProgressChanged();
}

void QGCStateMachine::onEntry(QEvent* event)
{
    // Qt calls this for a machine that is a state of another machine. QStateMachine::onEntry() starts the machine
    // with QStateMachine::start(), which is not virtual, so this does what it does through start()
    start();
    QState::onEntry(event);
}

void QGCStateMachine::_setInProgress(bool inProgress)
{
    if (_inProgress != inProgress) {
        _inProgress = inProgress;
        emit inProgressChanged();
    }
}

void QGCStateMachine::setGlobalErrorState(QAbstractState* errorState)
{
    _globalErrorState = errorState;
}

void QGCStateMachine::registerState(QGCState* state)
{
    if (_globalErrorState && !state->localErrorState()) {
        state->addTransition(state, &QGCState::error, _globalErrorState);
    }
}

void QGCStateMachine::registerState(QGCAbstractState* state)
{
    // QGCAbstractState cannot have local error-state helpers, so wire to global.
    if (_globalErrorState) {
        state->addTransition(state, &QGCAbstractState::error, _globalErrorState);
    }
}

FunctionState* QGCStateMachine::addFunctionState(const QString& stateName, std::function<void()> function)
{
    auto* state = new FunctionState(stateName, this, std::move(function));
    registerState(state);
    return state;
}

AsyncFunctionState* QGCStateMachine::addAsyncFunctionState(const QString& stateName,
                                                            AsyncFunctionState::SetupFunction setupFunction,
                                                            int timeoutMsecs)
{
    auto* state = new AsyncFunctionState(stateName, this, std::move(setupFunction), timeoutMsecs);
    registerState(state);
    return state;
}

DelayState* QGCStateMachine::addDelayState(int delayMsecs)
{
    auto* state = new DelayState(this, delayMsecs);
    registerState(state);
    return state;
}

void QGCStateMachine::postEvent(const QString& eventName, const QVariant& data, EventPriority priority)
{
    qCDebug(QGCStateMachineLog) << objectName() << "posting event:" << eventName
                                 << (priority == HighPriority ? "(high priority)" : "");
    QStateMachine::postEvent(new QGCStateMachineEvent(eventName, data), priority);
}

int QGCStateMachine::postDelayedEvent(const QString& eventName, int delayMsecs, const QVariant& data)
{
    qCDebug(QGCStateMachineLog) << objectName() << "posting delayed event:" << eventName << "delay:" << delayMsecs << "ms";
    return QStateMachine::postDelayedEvent(new QGCStateMachineEvent(eventName, data), delayMsecs);
}

bool QGCStateMachine::cancelDelayedEvent(int eventId)
{
    qCDebug(QGCStateMachineLog) << objectName() << "cancelling delayed event id:" << eventId;
    return QStateMachine::cancelDelayedEvent(eventId);
}

// -----------------------------------------------------------------------------
// State Configuration
// -----------------------------------------------------------------------------

void QGCStateMachine::setInitialState(QAbstractState* state, bool autoStart)
{
    QStateMachine::setInitialState(state);
    if (autoStart) {
        start();
    }
}

QGCFinalState* QGCStateMachine::addFinalState(const QString& stateName)
{
    auto* state = new QGCFinalState(stateName.isEmpty() ? QStringLiteral("Final") : stateName, this);
    return state;
}

// -----------------------------------------------------------------------------
// Lifecycle Helpers
// -----------------------------------------------------------------------------

void QGCStateMachine::restart()
{
    // The progress is reset right before the start, so that a machine that is still stopping cannot raise it again.
    // resetProgress() emits nothing, so a machine that had progressed says here that it is back at 0.
    const auto startOver = [this]() {
        _restartPending = false;
        const bool hadProgress = progress() > 0.0f;
        resetProgress();
        if (hadProgress) {
            emit progressUpdate(0.0f);
        }
        start();
    };

    if (inProgress()) {
        // stop() is asynchronous, so connect to stopped signal to start again. One connection serves every
        // request made until the machine has stopped: a second start() would be refused.
        if (!_restartPending) {
            _restartPending = true;
            connect(
                this, &QStateMachine::stopped, this,
                [this, startOver]() {
                    // Not when a listener of stopped that came first has restarted the machine already
                    if (_restartPending) {
                        startOver();
                    }
                },
                Qt::SingleShotConnection);
        }
        stop();
    } else {
        startOver();
    }
}

// -----------------------------------------------------------------------------
// Transition Helpers
// -----------------------------------------------------------------------------

QSignalTransition* QGCStateMachine::addTransition(QState* from, const char* signal, QAbstractState* to)
{
    return from->QState::addTransition(from, signal, to);
}

MachineEventTransition* QGCStateMachine::addEventTransition(QState* from, const QString& eventName, QAbstractState* to)
{
    auto* transition = new MachineEventTransition(eventName, to);
    from->addTransition(transition);
    return transition;
}

// -----------------------------------------------------------------------------
// Progress Tracking
// -----------------------------------------------------------------------------

void QGCStateMachine::setProgressWeights(const QList<QPair<QAbstractState*, int>>& stateWeights)
{
    // Disconnect old connections before re-wiring
    for (auto* state : _progressStates) {
        disconnect(state, &QAbstractState::entered, this, &QGCStateMachine::_onStateEntered);
    }

    _progressStates.clear();
    _progressWeights.clear();
    _progressTotalWeight = 0;

    for (const auto& pair : stateWeights) {
        _progressStates.append(pair.first);
        _progressWeights.append(pair.second);
        _progressTotalWeight += pair.second;

        // Connect to state's entered signal to auto-update progress index
        connect(pair.first, &QAbstractState::entered, this, &QGCStateMachine::_onStateEntered);
    }

    qCDebug(QGCStateMachineLog) << objectName() << "progress tracking enabled for"
                                 << _progressStates.size() << "states, total weight:" << _progressTotalWeight;
}

void QGCStateMachine::_onStateEntered()
{
    auto* state = qobject_cast<QAbstractState*>(sender());
    if (!state) return;

    int index = _progressStates.indexOf(state);
    if (index >= 0 && index != _progressCurrentIndex) {
        _progressCurrentIndex = index;
        _progressSubProgress = 0.0f;

        float newProgress = _calculateProgress();
        if (newProgress > _progressLastEmitted) {
            _progressLastEmitted = newProgress;
            emit progressUpdate(newProgress);
        }
    }
}

void QGCStateMachine::setSubProgress(float subProgress)
{
    _progressSubProgress = qBound(0.0f, subProgress, 1.0f);

    float newProgress = _calculateProgress();
    if (newProgress > _progressLastEmitted) {
        _progressLastEmitted = newProgress;
        emit progressUpdate(newProgress);
    }
}

float QGCStateMachine::progress() const
{
    return _progressLastEmitted;
}

void QGCStateMachine::resetProgress()
{
    _progressCurrentIndex = -1;
    _progressSubProgress = 0.0f;
    _progressLastEmitted = 0.0f;
}

float QGCStateMachine::_calculateProgress() const
{
    if (_progressTotalWeight <= 0 || _progressCurrentIndex < 0) {
        return 0.0f;
    }

    int completedWeight = 0;
    for (int i = 0; i < _progressCurrentIndex && i < _progressWeights.size(); ++i) {
        completedWeight += _progressWeights[i];
    }

    int currentWeight = (_progressCurrentIndex < _progressWeights.size())
                            ? _progressWeights[_progressCurrentIndex]
                            : 1;

    return (completedWeight + currentWeight * _progressSubProgress) / static_cast<float>(_progressTotalWeight);
}

QGCStateMachine* qgcStateMachineOf(const QAbstractState* state)
{
    return qobject_cast<QGCStateMachine*>(state->machine());
}

QGCStateMachine* qgcStateMachineOf(const QAbstractTransition* transition)
{
    return qobject_cast<QGCStateMachine*>(transition->machine());
}

QString qualifiedStateName(const QAbstractState* state)
{
    const QGCStateMachine* const machine = qgcStateMachineOf(state);
    return machine ? QStringLiteral("%1:%2").arg(machine->objectName(), state->objectName()) : state->objectName();
}
