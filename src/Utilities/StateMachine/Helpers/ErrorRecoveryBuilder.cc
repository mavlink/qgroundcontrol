#include "ErrorRecoveryBuilder.h"
#include "QGCStateMachine.h"
#include "QGCLoggingCategory.h"

// -----------------------------------------------------------------------------
// ErrorRecoveryBuilder
// -----------------------------------------------------------------------------

ErrorRecoveryBuilder::ErrorRecoveryBuilder(QGCStateMachine* machine, const QString& stateName)
    : _machine(machine)
    , _stateName(stateName)
{
}

ErrorRecoveryBuilder& ErrorRecoveryBuilder::withAction(Action action)
{
    _action = std::move(action);
    return *this;
}

ErrorRecoveryBuilder& ErrorRecoveryBuilder::retry(int maxRetries, int delayMsecs)
{
    _maxRetries = maxRetries;
    _retryDelayMsecs = delayMsecs;
    return *this;
}

ErrorRecoveryBuilder& ErrorRecoveryBuilder::withFallback(Action fallback)
{
    _fallback = std::move(fallback);
    return *this;
}

ErrorRecoveryBuilder& ErrorRecoveryBuilder::withRollback(VoidAction rollback)
{
    _rollback = std::move(rollback);
    return *this;
}

ErrorRecoveryBuilder& ErrorRecoveryBuilder::onExhausted(ExhaustedBehavior behavior)
{
    _exhaustedBehavior = behavior;
    return *this;
}

ErrorRecoveryBuilder& ErrorRecoveryBuilder::withTimeout(int timeoutMsecs)
{
    _timeoutMsecs = timeoutMsecs;
    return *this;
}

QGCState* ErrorRecoveryBuilder::build()
{
    auto* state = new ErrorRecoveryState(_stateName, _machine);

    state->setAction(_action);
    state->setFallback(_fallback);
    state->setRollback(_rollback);
    state->setRetry(_maxRetries, _retryDelayMsecs);
    state->setTimeout(_timeoutMsecs);
    state->setExhaustedBehavior(_exhaustedBehavior);

    _machine->registerState(state);
    return state;
}

// -----------------------------------------------------------------------------
// ErrorRecoveryState
// -----------------------------------------------------------------------------

ErrorRecoveryState::ErrorRecoveryState(const QString& stateName, QState* parent)
    : QGCState(stateName, parent)
{
    _retryTimer.setSingleShot(true);
    connect(&_retryTimer, &QTimer::timeout, this, &ErrorRecoveryState::_executeAction);

    _timeoutTimer.setSingleShot(true);
    connect(&_timeoutTimer, &QTimer::timeout, this, &ErrorRecoveryState::_onTimeout);
}

void ErrorRecoveryState::setRetry(int maxRetries, int delayMsecs)
{
    _maxRetries = maxRetries;
    _retryDelayMsecs = delayMsecs;
}

void ErrorRecoveryState::setTimeout(int timeoutMsecs)
{
    _timeoutMsecs = timeoutMsecs;
}

void ErrorRecoveryState::setExhaustedBehavior(ErrorRecoveryBuilder::ExhaustedBehavior behavior)
{
    _exhaustedBehavior = behavior;
}

void ErrorRecoveryState::onEnter()
{
    _currentAttempt = 0;
    _triedFallback = false;
    _successPhase.clear();

    if (_timeoutMsecs > 0) {
        _timeoutTimer.start(_timeoutMsecs);
    }

    _executeAction();
}

void ErrorRecoveryState::_executeAction()
{
    _currentAttempt++;
    int totalAttempts = _maxRetries + 1;

    qCDebug(QGCStateMachineLog) << stateName() << "attempt" << _currentAttempt << "of" << totalAttempts;

    bool success = false;
    if (_action) {
        success = _action();
    }

    if (success) {
        _timeoutTimer.stop();
        _successPhase = QStringLiteral("primary");
        qCDebug(QGCStateMachineLog) << stateName() << "primary action succeeded";
        emit succeeded();
        emit advance();
    } else if (_currentAttempt <= _maxRetries) {
        // Retry
        qCDebug(QGCStateMachineLog) << stateName() << "retrying in" << _retryDelayMsecs << "ms";
        emit retrying(_currentAttempt + 1, totalAttempts);
        _retryTimer.start(_retryDelayMsecs);
    } else {
        // Primary exhausted
        _handleFailure();
    }
}

void ErrorRecoveryState::_handleFailure()
{
    // Try fallback if available and not already tried
    if (_fallback && !_triedFallback) {
        _triedFallback = true;
        qCDebug(QGCStateMachineLog) << stateName() << "trying fallback";
        emit tryingFallback();

        bool success = _fallback();
        if (success) {
            _timeoutTimer.stop();
            _successPhase = QStringLiteral("fallback");
            qCDebug(QGCStateMachineLog) << stateName() << "fallback succeeded";
            emit succeeded();
            emit advance();
            return;
        }
    }

    // All options exhausted
    _handleExhausted();
}

void ErrorRecoveryState::_handleExhausted()
{
    _timeoutTimer.stop();

    // Execute rollback if available
    if (_rollback) {
        qCDebug(QGCStateMachineLog) << stateName() << "executing rollback";
        emit rollingBack();
        _rollback();
    }

    emit exhausted();

    switch (_exhaustedBehavior) {
    case ErrorRecoveryBuilder::EmitError:
        qCDebug(QGCStateMachineLog) << stateName() << "all options exhausted, emitting error";
        emit error();
        break;

    case ErrorRecoveryBuilder::EmitAdvance:
        qCDebug(QGCStateMachineLog) << stateName() << "all options exhausted, continuing anyway";
        emit advance();
        break;

    case ErrorRecoveryBuilder::LogAndError:
        qCWarning(QGCStateMachineLog) << stateName() << "all recovery options exhausted";
        emit error();
        break;

    case ErrorRecoveryBuilder::LogAndAdvance:
        qCWarning(QGCStateMachineLog) << stateName() << "all recovery options exhausted, continuing";
        emit advance();
        break;
    }
}

void ErrorRecoveryState::_onTimeout()
{
    qCDebug(QGCStateMachineLog) << stateName() << "operation timed out";
    _retryTimer.stop();
    _handleExhausted();
}
