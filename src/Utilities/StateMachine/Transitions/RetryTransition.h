#pragma once

#include <functional>

#include <QtCore/QMetaObject>
#include <QtCore/QPointer>
#include <QtStateMachine/QAbstractState>
#include <QtStateMachine/QSignalTransition>

#include "SignalSender.h"

/// \brief Transition that retries an action N times before advancing to target state.
///
/// Useful for timeout handling where you want to retry before giving up.
///
/// On each trigger (e.g., timeout signal):
/// - If retry count < max: increment count, let the source state wait again, invoke retryAction, stay in the state
/// - If retry count >= max: reset count, go to the target
///
/// A retry is a transition that is taken: while retries are left the transition has no target, which is how Qt
/// stays in a state, and targetState() is null. Another transition for the same signal is therefore not taken
/// during the retries: not one of a parent state, and one of the state itself only if it was added before this one.
///
/// A source state that is a WaitStateBase waits again through restartWait(). An AsyncFunctionState or a
/// SkippableAsyncState runs its setup function again there, so the retry action is for what that function does
/// not do; a state that sends when it is entered, and not in a setup function, needs the action to send again.
///
/// The count starts again each time the source state is entered, so a state that was left through another
/// transition after some retries has all of them the next time.
///
/// Example usage:
/// @code
/// auto* retry = new RetryTransition(
///     state, &WaitStateBase::timedOut,
///     nextState,
///     [this]() { _retriesMade++; },
///     1  // max retries
/// );
/// state->addTransition(retry);
/// @endcode
///
class RetryTransition : public QSignalTransition
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
    template <typename Func>
    RetryTransition(const SignalSender<Func>* sender, Func signalFn, QAbstractState* target, RetryAction retryAction,
                    int maxRetries = 1)
        : QSignalTransition(sender, signalFn)
        , _retryAction(std::move(retryAction))
        , _maxRetries(maxRetries)
        , _target(target)
    {
        _updateTargetState();
    }

    /// Start the retries over, as is done when the source state is entered
    void reset();

    /// Get current retry count
    int retryCount() const { return _retryCount; }

    /// Get max retries setting
    int maxRetries() const { return _maxRetries; }

protected:
    bool eventTest(QEvent* event) override;

    /// Makes a retry, or goes to the target once the retries are used up
    void onTransition(QEvent* event) override;

private:
    /// No target while retries are left, the target of the constructor after the last one
    void _updateTargetState();

    RetryAction _retryAction;
    int _maxRetries = 1;
    int _retryCount = 0;
    QPointer<QAbstractState> _target;
    QMetaObject::Connection _resetOnEntry;
};
