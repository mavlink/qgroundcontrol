#pragma once

#include "QGCState.h"

#include <QtCore/QTimer>

#include <functional>

/// A state that retries an action multiple times before failing.
///
/// The action is executed on entry. If it returns false (failure),
/// the state waits for the retry delay and tries again. After maxRetries
/// failures, the error() signal is emitted.
///
/// Example usage:
/// @code
/// auto* state = new RetryThenFailState("LoadConfig", &machine,
///     []() { return tryLoadConfig(); },  // Returns true on success
///     3,      // Max retries
///     1000    // Delay between retries (ms)
/// );
/// state->addTransition(state, &RetryThenFailState::advance, successState);
/// state->addTransition(state, &RetryThenFailState::error, errorState);
/// @endcode
class RetryThenFailState : public QGCState
{
    Q_OBJECT
    Q_DISABLE_COPY(RetryThenFailState)

public:
    /// Action that returns true on success, false on failure
    using Action = std::function<bool()>;

    /// @param stateName Name for logging
    /// @param parent Parent state
    /// @param action Action to execute (returns true on success)
    /// @param maxRetries Maximum number of retry attempts (0 = no retries, just one attempt)
    /// @param retryDelayMsecs Delay between retries in milliseconds
    RetryThenFailState(const QString& stateName, QState* parent,
                       Action action, int maxRetries = 3, int retryDelayMsecs = 1000);

    /// Get the current attempt number (1-based)
    int currentAttempt() const { return _currentAttempt; }

    /// Get the maximum number of retries
    int maxRetries() const { return _maxRetries; }

    /// Get the total number of attempts (maxRetries + 1)
    int totalAttempts() const { return _maxRetries + 1; }

signals:
    /// Emitted before each retry attempt
    void retrying(int attempt, int maxAttempts);

    /// Emitted when the action succeeds
    void succeeded();

    /// Emitted when all retries are exhausted
    void exhausted();

protected:
    void onEnter() override;

private slots:
    void _executeAction();

private:
    Action _action;
    int _maxRetries;
    int _retryDelayMsecs;
    int _currentAttempt = 0;
    QTimer _retryTimer;
};
