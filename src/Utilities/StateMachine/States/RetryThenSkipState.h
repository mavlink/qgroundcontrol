#pragma once

#include "QGCState.h"

#include <QtCore/QTimer>

#include <functional>

/// A state that retries an action but continues (skips) on failure.
///
/// Similar to RetryThenFailState, but instead of emitting error() when
/// retries are exhausted, it emits advance() with a skipped flag.
/// Useful for optional operations that shouldn't block the workflow.
///
/// Example usage:
/// @code
/// auto* state = new RetryThenSkipState("OptionalSync", &machine,
///     []() { return syncData(); },
///     2,      // Max retries
///     500     // Delay between retries
/// );
/// // Always advances, check wasSkipped() to know if it failed
/// state->addTransition(state, &RetryThenSkipState::advance, nextState);
/// @endcode
class RetryThenSkipState : public QGCState
{
    Q_OBJECT
    Q_DISABLE_COPY(RetryThenSkipState)

public:
    using Action = std::function<bool()>;

    /// @param stateName Name for logging
    /// @param parent Parent state
    /// @param action Action to execute (returns true on success)
    /// @param maxRetries Maximum number of retry attempts
    /// @param retryDelayMsecs Delay between retries in milliseconds
    RetryThenSkipState(const QString& stateName, QState* parent,
                       Action action, int maxRetries = 2, int retryDelayMsecs = 500);

    /// Check if the action was skipped (all retries failed)
    bool wasSkipped() const { return _wasSkipped; }

    /// Get the current attempt number
    int currentAttempt() const { return _currentAttempt; }

    /// Get the maximum number of retries
    int maxRetries() const { return _maxRetries; }

signals:
    /// Emitted before each retry attempt
    void retrying(int attempt, int maxAttempts);

    /// Emitted when the action succeeds
    void succeeded();

    /// Emitted when all retries exhausted and operation is skipped
    void skipped();

protected:
    void onEnter() override;

private slots:
    void _executeAction();

private:
    Action _action;
    int _maxRetries;
    int _retryDelayMsecs;
    int _currentAttempt = 0;
    bool _wasSkipped = false;
    QTimer _retryTimer;
};
