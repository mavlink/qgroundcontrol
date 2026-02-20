#include "QGCStateMachine.h"
#include "StateHistoryRecorder.h"
#include "StateMachineLogger.h"
#include "StateMachineProfiler.h"
#include "QGCApplication.h"
#include "AudioOutput.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"

#include <QtCore/QTimer>
#include <QtStateMachine/QFinalState>
#include <QtStateMachine/QSignalTransition>

#include <memory>

QGCStateMachine::QGCStateMachine(const QString& machineName, Vehicle *vehicle, QObject* parent)
    : QStateMachine (parent)
    , _vehicle      (vehicle)
{
    setObjectName(machineName);

    connect(this, &QGCStateMachine::started, this, [this] () {
        qCDebug(QGCStateMachineLog) << "State machine started:" << objectName();
        emit runningChanged();
    });
    connect(this, &QGCStateMachine::stopped, this, [this] () {
        qCDebug(QGCStateMachineLog) << "State machine stopped:" << objectName();
        emit runningChanged();
        if (_deleteOnStop) {
            deleteLater();
        }
    });
    connect(this, &QGCStateMachine::finished, this, [this] () {
        qCDebug(QGCStateMachineLog) << "State machine finished:" << objectName();
        if (_progressTotalWeight > 0 && _progressLastEmitted < 1.0f) {
            _progressLastEmitted = 1.0f;
            emit progressUpdate(1.0f);
        }
    });

    if (qEnvironmentVariableIsSet("QGC_STATEMACHINE_HISTORY")) {
        setHistoryRecordingEnabled(true);
    }
    if (qEnvironmentVariableIsSet("QGC_STATEMACHINE_PROFILE")) {
        setProfilingEnabled(true);
    }
    if (qEnvironmentVariableIsSet("QGC_STATEMACHINE_LOG")) {
        setStructuredLoggingEnabled(true);
    }
}

QString QGCStateMachine::currentStateName() const
{
    const auto activeStates = configuration();
    if (activeStates.isEmpty()) {
        return QString();
    }
    // Return the name of the first (deepest) active state
    QAbstractState* active = *activeStates.begin();
    return active ? active->objectName() : QString();
}

void QGCStateMachine::start()
{
    if (isRunning()) {
        qCWarning(QGCStateMachineLog) << objectName() << "start() called but already running - check signal connections";
    }
    QStateMachine::start();
}

void QGCStateMachine::setGlobalErrorState(QAbstractState* errorState)
{
    _globalErrorState = errorState;
}

void QGCStateMachine::enablePropertyRestore()
{
    setGlobalRestorePolicy(QState::RestoreProperties);
    qCDebug(QGCStateMachineLog) << objectName() << "property restore enabled";
}

bool QGCStateMachine::isPropertyRestoreEnabled() const
{
    return globalRestorePolicy() == QState::RestoreProperties;
}

void QGCStateMachine::registerState(QGCState* state)
{
    // Only add global error transition if no local error state is set
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

QGCState* QGCStateMachine::addErrorRecoveryState(const QString& stateName,
                                                 ErrorRecoveryBuilder::Action action,
                                                 int maxRetries,
                                                 int retryDelayMsecs,
                                                 ErrorRecoveryBuilder::ExhaustedBehavior exhaustedBehavior,
                                                 ErrorRecoveryBuilder::Action fallback,
                                                 ErrorRecoveryBuilder::VoidAction rollback,
                                                 int timeoutMsecs)
{
    ErrorRecoveryBuilder builder(this, stateName);
    builder.withAction(std::move(action))
           .retry(maxRetries, retryDelayMsecs)
           .onExhausted(exhaustedBehavior);

    if (fallback) {
        builder.withFallback(std::move(fallback));
    }
    if (rollback) {
        builder.withRollback(std::move(rollback));
    }
    if (timeoutMsecs > 0) {
        builder.withTimeout(timeoutMsecs);
    }

    return builder.build();
}

DelayState* QGCStateMachine::addDelayState(int delayMsecs)
{
    auto* state = new DelayState(this, delayMsecs);
    registerState(state);
    return state;
}

ParallelState* QGCStateMachine::addParallelState(const QString& stateName)
{
    auto* state = new ParallelState(stateName, this);
    registerState(state);
    return state;
}

void QGCStateMachine::postEvent(const QString& eventName, const QVariant& data, EventPriority priority)
{
    qCDebug(QGCStateMachineLog) << objectName() << "posting event:" << eventName
                                 << (priority == HighPriority ? "(high priority)" : "");
    emit machineEvent(eventName);
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

ConditionalState* QGCStateMachine::addConditionalState(const QString& stateName,
                                                        ConditionalState::Predicate predicate,
                                                        ConditionalState::Action action)
{
    auto* state = new ConditionalState(stateName, this, std::move(predicate), std::move(action));
    registerState(state);
    return state;
}

SubMachineState* QGCStateMachine::addSubMachineState(const QString& stateName, SubMachineState::MachineFactory factory)
{
    auto* state = new SubMachineState(stateName, this, std::move(factory));
    registerState(state);
    return state;
}

EventQueuedState* QGCStateMachine::addEventQueuedState(const QString& stateName,
                                                        const QString& eventName,
                                                        int timeoutMsecs)
{
    auto* state = new EventQueuedState(stateName, this, eventName, timeoutMsecs);
    registerState(state);
    return state;
}

QState* QGCStateMachine::createTimedActionState(const QString& stateName,
                                                 int durationMsecs,
                                                 std::function<void()> onEntry,
                                                 std::function<void()> onExit)
{
    // Create the composite parent state
    auto* parentState = new QState(this);
    parentState->setObjectName(stateName);

    // Create a timer owned by the parent state (auto-cleanup)
    auto* timer = new QTimer(parentState);
    timer->setInterval(durationMsecs);
    timer->setSingleShot(true);

    // Create internal timing state (active while timer runs)
    auto* timingState = new QState(parentState);
    timingState->setObjectName(stateName + QStringLiteral("_Timing"));

    // Create internal final state (signals completion)
    auto* doneState = new QFinalState(parentState);

    // Wire up the timing state
    if (onEntry) {
        connect(timingState, &QState::entered, this, onEntry);
    }
    connect(timingState, &QState::entered, timer, qOverload<>(&QTimer::start));

    if (onExit) {
        connect(timingState, &QState::exited, this, onExit);
    }

    // Timer timeout transitions to done state
    timingState->addTransition(timer, &QTimer::timeout, doneState);

    // Set initial state
    parentState->setInitialState(timingState);

    qCDebug(QGCStateMachineLog) << "Created timed action state:" << stateName
                                 << "duration:" << durationMsecs << "ms";

    return parentState;
}

// -----------------------------------------------------------------------------
// State Queries
// -----------------------------------------------------------------------------

bool QGCStateMachine::isStateActive(QAbstractState* state) const
{
    return configuration().contains(state);
}

QAbstractState* QGCStateMachine::findState(const QString& stateName) const
{
    // Use findChild with name parameter - more efficient than findChildren + loop
    return findChild<QAbstractState*>(stateName);
}

// -----------------------------------------------------------------------------
// Error Handling
// -----------------------------------------------------------------------------

bool QGCStateMachine::isInErrorState() const
{
    if (_globalErrorState) {
        return configuration().contains(_globalErrorState);
    }
    return false;
}

FunctionState* QGCStateMachine::addLogAndContinueErrorState(const QString& stateName,
                                                            QAbstractState* nextState,
                                                            const QString& message)
{
    auto* state = ErrorHandlers::logAndContinue(this, stateName, nextState, message);
    registerState(state);
    return state;
}

FunctionState* QGCStateMachine::addLogAndStopErrorState(const QString& stateName,
                                                        const QString& message)
{
    auto* state = ErrorHandlers::logAndStop(this, stateName, message);
    registerState(state);
    return state;
}

void QGCStateMachine::clearError(bool restart)
{
    if (restart) {
        stop();
        start();
    }
}

bool QGCStateMachine::resetToState(QAbstractState* state)
{
    if (!state) {
        qCWarning(QGCStateMachineLog) << objectName() << "resetToState: null state";
        return false;
    }

    // Verify the state belongs to this machine
    if (state->parentState() != this && state->parent() != this) {
        qCWarning(QGCStateMachineLog) << objectName() << "resetToState: state does not belong to this machine";
        return false;
    }

    qCDebug(QGCStateMachineLog) << objectName() << "resetting to state:" << state->objectName();

    // Stop the machine, change initial state, and restart
    if (isRunning()) {
        auto conn = std::make_shared<QMetaObject::Connection>();
        *conn = connect(this, &QStateMachine::stopped, this, [this, state, conn]() {
            disconnect(*conn);
            setInitialState(state);
            resetProgress();
            start();
        });
        stop();
    } else {
        setInitialState(state);
        resetProgress();
        start();
    }

    return true;
}

bool QGCStateMachine::recoverFromError()
{
    if (!isInErrorState()) {
        qCDebug(QGCStateMachineLog) << objectName() << "recoverFromError: not in error state";
        return false;
    }

    qCDebug(QGCStateMachineLog) << objectName() << "recovering from error, restarting";
    clearError(true);
    return true;
}

bool QGCStateMachine::attemptRecovery()
{
    if (!_recoveryState) {
        qCDebug(QGCStateMachineLog) << objectName() << "attemptRecovery: no recovery state set";
        return false;
    }

    qCDebug(QGCStateMachineLog) << objectName() << "attempting recovery to:" << _recoveryState->objectName();
    return resetToState(_recoveryState);
}

// -----------------------------------------------------------------------------
// Lifecycle Helpers
// -----------------------------------------------------------------------------

void QGCStateMachine::ensureRunning()
{
    if (!isRunning()) {
        start();
    }
}

void QGCStateMachine::stopMachine(bool deleteOnStop)
{
    _deleteOnStop = deleteOnStop;
    stop();
}

void QGCStateMachine::restart()
{
    if (isRunning()) {
        // stop() is asynchronous, so connect to stopped signal to start again
        auto conn = std::make_shared<QMetaObject::Connection>();
        *conn = connect(this, &QStateMachine::stopped, this, [this, conn]() {
            disconnect(*conn);
            start();
        });
        stop();
    } else {
        start();
    }
}

// -----------------------------------------------------------------------------
// Transition Helpers
// -----------------------------------------------------------------------------

MachineEventTransition* QGCStateMachine::addEventTransition(QState* from, const QString& eventName, QAbstractState* to,
                                                            QAbstractAnimation* animation)
{
    auto* transition = new MachineEventTransition(eventName, to);
    if (animation) {
        transition->addAnimation(animation);
    }
    from->addTransition(transition);
    return transition;
}

TimeoutTransition* QGCStateMachine::addTimeoutTransition(QState* from, int timeoutMsecs, QAbstractState* to,
                                                         QAbstractAnimation* animation)
{
    auto* transition = new TimeoutTransition(timeoutMsecs, to);
    transition->attachToSourceState(from);
    if (animation) {
        transition->addAnimation(animation);
    }
    from->addTransition(transition);
    return transition;
}

// -----------------------------------------------------------------------------
// State Introspection
// -----------------------------------------------------------------------------

QList<QAbstractTransition*> QGCStateMachine::transitionsFrom(QAbstractState* state) const
{
    QList<QAbstractTransition*> result;
    if (auto* qstate = qobject_cast<QState*>(state)) {
        result = qstate->transitions();
    }
    return result;
}

QList<QAbstractTransition*> QGCStateMachine::transitionsTo(QAbstractState* state) const
{
    QList<QAbstractTransition*> result;
    const auto allTransitions = findChildren<QAbstractTransition*>();
    for (QAbstractTransition* transition : allTransitions) {
        const auto targets = transition->targetStates();
        if (targets.contains(state)) {
            result.append(transition);
        }
    }
    return result;
}

QList<QAbstractState*> QGCStateMachine::reachableFrom(QAbstractState* state) const
{
    QList<QAbstractState*> result;
    for (QAbstractTransition* transition : transitionsFrom(state)) {
        for (QAbstractState* target : transition->targetStates()) {
            if (!result.contains(target)) {
                result.append(target);
            }
        }
    }
    return result;
}

QList<QAbstractState*> QGCStateMachine::predecessorsOf(QAbstractState* state) const
{
    QList<QAbstractState*> result;
    for (QAbstractTransition* transition : transitionsTo(state)) {
        if (auto* source = transition->sourceState()) {
            if (!result.contains(source)) {
                result.append(source);
            }
        }
    }
    return result;
}

// -----------------------------------------------------------------------------
// Debugging & Visualization
// -----------------------------------------------------------------------------

QString QGCStateMachine::dumpCurrentState() const
{
    QString result = objectName() + QStringLiteral(": ");

    if (!isRunning()) {
        result += QStringLiteral("(not running)");
        return result;
    }

    const auto active = configuration();
    if (active.isEmpty()) {
        result += QStringLiteral("(no active state)");
    } else {
        QStringList stateNames;
        for (QAbstractState* state : active) {
            stateNames.append(state->objectName().isEmpty()
                              ? QStringLiteral("<unnamed>")
                              : state->objectName());
        }
        result += stateNames.join(QStringLiteral(", "));
        result += QStringLiteral(" (running)");
    }

    return result;
}

QString QGCStateMachine::dumpConfiguration() const
{
    QString result;
    result += QStringLiteral("State Machine: %1\n").arg(objectName());
    result += QStringLiteral("================\n");

    // Get all states
    const auto allStates = findChildren<QAbstractState*>();
    result += QStringLiteral("States (%1):\n").arg(allStates.size());

    for (QAbstractState* state : allStates) {
        QString stateName = state->objectName().isEmpty()
                            ? QStringLiteral("<unnamed>")
                            : state->objectName();

        // Mark special states
        QString marker;
        if (state == initialState()) {
            marker = QStringLiteral(" [initial]");
        }
        if (qobject_cast<QFinalState*>(state)) {
            marker += QStringLiteral(" [final]");
        }
        if (state == _globalErrorState) {
            marker += QStringLiteral(" [error]");
        }
        if (configuration().contains(state)) {
            marker += QStringLiteral(" [active]");
        }

        result += QStringLiteral("  - %1%2\n").arg(stateName, marker);

        // Show transitions from this state
        auto transitions = transitionsFrom(state);
        for (QAbstractTransition* transition : transitions) {
            const auto targets = transition->targetStates();
            for (QAbstractState* target : targets) {
                QString targetName = target->objectName().isEmpty()
                                     ? QStringLiteral("<unnamed>")
                                     : target->objectName();
                result += QStringLiteral("      -> %1\n").arg(targetName);
            }
        }
    }

    return result;
}

void QGCStateMachine::logCurrentState() const
{
    qCDebug(QGCStateMachineLog) << dumpCurrentState();
}

void QGCStateMachine::logConfiguration() const
{
    const auto lines = dumpConfiguration().split('\n');
    for (const QString& line : lines) {
        if (!line.isEmpty()) {
            qCDebug(QGCStateMachineLog) << line;
        }
    }
}

void QGCStateMachine::setHistoryRecordingEnabled(bool enabled, int maxEntries)
{
    if (!_historyRecorder) {
        _historyRecorder = new StateHistoryRecorder(this, maxEntries);
    }
    _historyRecorder->setMaxEntries(maxEntries);
    _historyRecorder->setEnabled(enabled);
}

bool QGCStateMachine::historyRecordingEnabled() const
{
    return _historyRecorder && _historyRecorder->isEnabled();
}

QString QGCStateMachine::dumpRecordedHistory() const
{
    return _historyRecorder ? _historyRecorder->dumpHistory() : QString();
}

QJsonArray QGCStateMachine::recordedHistoryJson() const
{
    return _historyRecorder ? _historyRecorder->toJson() : QJsonArray();
}

void QGCStateMachine::setProfilingEnabled(bool enabled)
{
    if (!_profiler) {
        _profiler = new StateMachineProfiler(this);
    }
    _profiler->setEnabled(enabled);
}

bool QGCStateMachine::profilingEnabled() const
{
    return _profiler && _profiler->isEnabled();
}

QString QGCStateMachine::profilingSummary() const
{
    return _profiler ? _profiler->summary() : QString();
}

void QGCStateMachine::setStructuredLoggingEnabled(bool enabled)
{
    if (!_logger) {
        _logger = new StateMachineLogger(this, this);
    }
    _logger->setEnabled(enabled);
}

bool QGCStateMachine::structuredLoggingEnabled() const
{
    return _logger && _logger->isEnabled();
}

QString QGCStateMachine::exportAsDot() const
{
    QString dot;
    dot += QStringLiteral("digraph \"%1\" {\n").arg(objectName());
    dot += QStringLiteral("    rankdir=TB;\n");
    dot += QStringLiteral("    node [shape=box, style=rounded];\n");
    dot += QStringLiteral("\n");

    const auto allStates = findChildren<QAbstractState*>();

    // Define nodes with special styling
    for (QAbstractState* state : allStates) {
        QString name = state->objectName().isEmpty()
                       ? QStringLiteral("state_%1").arg(reinterpret_cast<quintptr>(state), 0, 16)
                       : state->objectName();

        QStringList attrs;
        if (state == initialState()) {
            attrs << QStringLiteral("style=\"rounded,bold\"");
            attrs << QStringLiteral("peripheries=2");
        }
        if (qobject_cast<QFinalState*>(state)) {
            attrs << QStringLiteral("shape=doublecircle");
        }
        if (state == _globalErrorState) {
            attrs << QStringLiteral("color=red");
        }

        if (attrs.isEmpty()) {
            dot += QStringLiteral("    \"%1\";\n").arg(name);
        } else {
            dot += QStringLiteral("    \"%1\" [%2];\n").arg(name, attrs.join(", "));
        }
    }

    dot += QStringLiteral("\n");

    // Define edges
    for (QAbstractState* state : allStates) {
        QString fromName = state->objectName().isEmpty()
                           ? QStringLiteral("state_%1").arg(reinterpret_cast<quintptr>(state), 0, 16)
                           : state->objectName();

        auto transitions = transitionsFrom(state);
        for (QAbstractTransition* transition : transitions) {
            const auto targets = transition->targetStates();
            for (QAbstractState* target : targets) {
                QString toName = target->objectName().isEmpty()
                                 ? QStringLiteral("state_%1").arg(reinterpret_cast<quintptr>(target), 0, 16)
                                 : target->objectName();

                // Try to get transition label from object name
                QString label = transition->objectName();
                if (label.isEmpty()) {
                    dot += QStringLiteral("    \"%1\" -> \"%2\";\n").arg(fromName, toName);
                } else {
                    dot += QStringLiteral("    \"%1\" -> \"%2\" [label=\"%3\"];\n").arg(fromName, toName, label);
                }
            }
        }
    }

    dot += QStringLiteral("}\n");
    return dot;
}

QList<QAbstractState*> QGCStateMachine::unreachableStates() const
{
    QList<QAbstractState*> unreachable;

    if (!initialState()) {
        return unreachable;
    }

    // BFS from initial state
    QSet<QAbstractState*> visited;
    QList<QAbstractState*> queue;
    queue.append(initialState());
    visited.insert(initialState());

    while (!queue.isEmpty()) {
        QAbstractState* current = queue.takeFirst();
        auto reachable = reachableFrom(current);
        for (QAbstractState* next : reachable) {
            if (!visited.contains(next)) {
                visited.insert(next);
                queue.append(next);
            }
        }
    }

    // Find states not visited
    const auto allStates = findChildren<QAbstractState*>();
    for (QAbstractState* state : allStates) {
        if (!visited.contains(state)) {
            unreachable.append(state);
        }
    }

    return unreachable;
}

int QGCStateMachine::maxPathLength() const
{
    if (!initialState()) {
        return -1;
    }

    // BFS with depth tracking
    QHash<QAbstractState*, int> depth;
    QList<QAbstractState*> queue;

    queue.append(initialState());
    depth[initialState()] = 0;
    int maxDepth = 0;

    const int stateCount = findChildren<QAbstractState*>().size();
    const int maxSearchDepth = qMax(stateCount * 2, 100);

    while (!queue.isEmpty()) {
        QAbstractState* current = queue.takeFirst();
        int currentDepth = depth[current];

        if (currentDepth >= maxSearchDepth) {
            continue;
        }

        auto reachable = reachableFrom(current);
        for (QAbstractState* next : reachable) {
            int newDepth = currentDepth + 1;
            // Only update if we found a longer path
            if (!depth.contains(next) || depth[next] < newDepth) {
                depth[next] = newDepth;
                maxDepth = qMax(maxDepth, newDepth);
                queue.append(next);
            }
        }
    }

    return maxDepth;
}

QList<QAbstractState*> QGCStateMachine::deadEndStates() const
{
    QList<QAbstractState*> deadEnds;

    const auto allStates = findChildren<QAbstractState*>();
    for (QAbstractState* state : allStates) {
        // Skip final states - they're supposed to be dead ends
        if (qobject_cast<QFinalState*>(state)) {
            continue;
        }

        // Check if state has any outgoing transitions
        auto transitions = transitionsFrom(state);
        if (transitions.isEmpty()) {
            deadEnds.append(state);
        }
    }

    return deadEnds;
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

    // Update QML state history
    QString stateName = state->objectName();
    if (!stateName.isEmpty()) {
        _stateHistory.append(stateName);
        while (_stateHistory.size() > _stateHistoryLimit) {
            _stateHistory.removeFirst();
        }
        emit stateHistoryChanged();
        emit currentStateNameChanged();
    }

    // Update progress tracking
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

// -----------------------------------------------------------------------------
// Timeout Configuration
// -----------------------------------------------------------------------------

void QGCStateMachine::setTimeoutOverride(const QString& stateName, int timeoutMsecs)
{
    _timeoutOverrides[stateName] = timeoutMsecs;
    qCDebug(QGCStateMachineLog) << objectName() << "timeout override set for" << stateName << ":" << timeoutMsecs << "ms";
}

void QGCStateMachine::removeTimeoutOverride(const QString& stateName)
{
    _timeoutOverrides.remove(stateName);
}

int QGCStateMachine::timeoutOverride(const QString& stateName) const
{
    return _timeoutOverrides.value(stateName, -1);
}

void QGCStateMachine::recordTimeout(const QString& stateName)
{
    _timeoutStats[stateName]++;
    qCDebug(QGCStateMachineLog) << objectName() << "timeout recorded for" << stateName
                                 << "total:" << _timeoutStats[stateName];
}

// -----------------------------------------------------------------------------
// Entry/Exit Callbacks
// -----------------------------------------------------------------------------

void QGCStateMachine::setCallbacks(EntryCallback onEntry, ExitCallback onExit)
{
    _entryCallback = std::move(onEntry);
    _exitCallback = std::move(onExit);
}

// -----------------------------------------------------------------------------
// QStateMachine Overrides
// -----------------------------------------------------------------------------

void QGCStateMachine::onEntry(QEvent* event)
{
    QStateMachine::onEntry(event);

    if (_entryCallback) {
        _entryCallback();
    }

    onEnter();
}

void QGCStateMachine::onExit(QEvent* event)
{
    onLeave();

    if (_exitCallback) {
        _exitCallback();
    }

    QStateMachine::onExit(event);
}

bool QGCStateMachine::event(QEvent* event)
{
    // Only allow handler to intercept custom events, not internal state machine events
    if (_eventHandler && event->type() >= QEvent::User) {
        if (_eventHandler(event)) {
            return true;
        }
    }

    return QStateMachine::event(event);
}
