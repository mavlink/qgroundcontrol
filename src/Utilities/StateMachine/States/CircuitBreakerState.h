#pragma once

#include "QGCState.h"

#include <QtCore/QElapsedTimer>

#include <functional>

/// A state that implements the circuit breaker pattern.
///
/// After a threshold of failures, the circuit "trips" and immediately
/// fails without attempting the action for a reset timeout period.
/// This prevents cascading failures when an external service is down.
///
/// States:
/// - Closed: Normal operation, action is attempted
/// - Open: Tripped, immediately fails without trying
/// - Half-Open: After reset timeout, allows one attempt to test recovery
///
/// Example usage:
/// @code
/// auto* state = new CircuitBreakerState("ExternalAPI", &machine,
///     []() { return callExternalApi(); },
///     5,      // Failure threshold
///     30000   // Reset timeout (ms)
/// );
///
/// state->addTransition(state, &CircuitBreakerState::advance, successState);
/// state->addTransition(state, &CircuitBreakerState::error, errorState);
/// @endcode
class CircuitBreakerState : public QGCState
{
    Q_OBJECT
    Q_DISABLE_COPY(CircuitBreakerState)

public:
    using Action = std::function<bool()>;

    enum class State {
        Closed,     ///< Normal operation
        Open,       ///< Tripped, failing fast
        HalfOpen    ///< Testing if service recovered
    };
    Q_ENUM(State)

    /// @param stateName Name for logging
    /// @param parent Parent state
    /// @param action Action to execute (returns true on success)
    /// @param failureThreshold Number of failures before tripping
    /// @param resetTimeoutMsecs Time before attempting recovery
    CircuitBreakerState(const QString& stateName, QState* parent,
                        Action action, int failureThreshold = 5,
                        int resetTimeoutMsecs = 30000);

    /// Get the current circuit state
    State circuitState() const { return _circuitState; }

    /// Get the current failure count
    int failureCount() const { return _failureCount; }

    /// Get the failure threshold
    int failureThreshold() const { return _failureThreshold; }

    /// Manually reset the circuit breaker
    void reset();

    /// Check if the circuit is currently tripped (open)
    bool isTripped() const { return _circuitState == State::Open; }

signals:
    /// Emitted when the circuit trips (too many failures)
    void tripped();

    /// Emitted when the circuit resets (after successful half-open test)
    void circuitReset();

    /// Emitted when action succeeds
    void succeeded();

    /// Emitted when action fails (or circuit is open)
    void failed();

protected:
    void onEnter() override;

private:
    Action _action;
    int _failureThreshold;
    int _resetTimeoutMsecs;

    State _circuitState = State::Closed;
    int _failureCount = 0;
    QElapsedTimer _tripTimer;
};
