#include "CircuitBreakerState.h"
#include "QGCLoggingCategory.h"

CircuitBreakerState::CircuitBreakerState(const QString& stateName, QState* parent,
                                         Action action, int failureThreshold,
                                         int resetTimeoutMsecs)
    : QGCState(stateName, parent)
    , _action(std::move(action))
    , _failureThreshold(failureThreshold)
    , _resetTimeoutMsecs(resetTimeoutMsecs)
{
}

void CircuitBreakerState::reset()
{
    _circuitState = State::Closed;
    _failureCount = 0;
    qCDebug(QGCStateMachineLog) << stateName() << "circuit breaker reset";
    emit circuitReset();
}

void CircuitBreakerState::onEnter()
{
    // Check if we're in Open state and if reset timeout has passed
    if (_circuitState == State::Open) {
        if (_tripTimer.elapsed() >= _resetTimeoutMsecs) {
            // Move to half-open to test recovery
            _circuitState = State::HalfOpen;
            qCDebug(QGCStateMachineLog) << stateName() << "circuit half-open, testing recovery";
        } else {
            // Still in cooldown, fail immediately
            qCDebug(QGCStateMachineLog) << stateName() << "circuit open, failing fast ("
                                         << (_resetTimeoutMsecs - _tripTimer.elapsed()) << "ms until retry)";
            emit failed();
            emit error();
            return;
        }
    }

    // Execute the action
    qCDebug(QGCStateMachineLog) << stateName() << "executing action (state:"
                                 << (_circuitState == State::HalfOpen ? "half-open" : "closed") << ")";

    bool success = false;
    if (_action) {
        success = _action();
    }

    if (success) {
        // Success - reset circuit if we were testing
        if (_circuitState == State::HalfOpen) {
            qCDebug(QGCStateMachineLog) << stateName() << "recovery successful, resetting circuit";
            reset();
        } else {
            _failureCount = 0;  // Reset failure count on success
        }
        emit succeeded();
        emit advance();
    } else {
        // Failure
        _failureCount++;
        qCDebug(QGCStateMachineLog) << stateName() << "action failed, count:"
                                     << _failureCount << "/" << _failureThreshold;

        if (_circuitState == State::HalfOpen) {
            // Failed during half-open test, go back to open
            _circuitState = State::Open;
            _tripTimer.restart();
            qCDebug(QGCStateMachineLog) << stateName() << "half-open test failed, circuit re-opened";
        } else if (_failureCount >= _failureThreshold) {
            // Trip the circuit
            _circuitState = State::Open;
            _tripTimer.start();
            qCDebug(QGCStateMachineLog) << stateName() << "failure threshold reached, circuit tripped";
            emit tripped();
        }

        emit failed();
        emit error();
    }
}
