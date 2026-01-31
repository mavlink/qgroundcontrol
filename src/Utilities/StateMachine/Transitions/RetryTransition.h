#pragma once

#include "QGCSignalTransition.h"

#include <QtCore/QLoggingCategory>

#include <functional>

/// Transition that retries an action N times before advancing to target state.
/// Useful for timeout handling where you want to retry before giving up.
///
/// On each trigger (e.g., timeout signal):
/// - If retry count < max: increment count, invoke retryAction, block transition
/// - If retry count >= max: reset count, allow transition to target
///
/// Example usage:
/// @code
/// auto* retry = new RetryTransition(
///     state, &WaitStateBase::timeout,
///     nextState,
///     [this]() { _requestAutopilotVersion(_stateAutopilotVersion); },
///     1  // max retries
/// );
/// state->addTransition(retry);
/// @endcode
class RetryTransition : public QGCSignalTransition
{
    Q_OBJECT
    Q_DISABLE_COPY(RetryTransition)

public:
    using RetryAction = std::function<void()>;

    /// Create a retry transition
    /// @param sender Object emitting the trigger signal (usually the state itself)
    /// @param signal Signal that triggers retry/advance (usually timeout)
    /// @param target State to transition to after max retries exhausted
    /// @param retryAction Action to invoke on each retry attempt
    /// @param maxRetries Maximum retry attempts before transitioning (default: 1)
    template<typename Func>
    RetryTransition(const typename QtPrivate::FunctionPointer<Func>::Object* sender,
                    Func signal,
                    QAbstractState* target,
                    RetryAction retryAction,
                    int maxRetries = 1)
        : QGCSignalTransition(sender, signal)
        , _retryAction(std::move(retryAction))
        , _maxRetries(maxRetries)
    {
        setTargetState(target);
    }

    /// Reset retry count (call when re-entering the source state)
    void reset() { _retryCount = 0; }

    /// Get current retry count
    int retryCount() const { return _retryCount; }

    /// Get max retries setting
    int maxRetries() const { return _maxRetries; }

protected:
    /// Override to implement retry logic
    /// Returns false during retries (blocking transition), true after max retries
    bool eventTest(QEvent* event) override;

    /// Called when transition fires (after max retries)
    void onTransition(QEvent* event) override;

private:
    RetryAction _retryAction;
    int _maxRetries = 1;
    int _retryCount = 0;
};
