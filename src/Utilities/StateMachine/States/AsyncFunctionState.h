#pragma once

#include <functional>

#include "CompletionWaitState.h"

/// \brief Calls a function when entered and waits for an external trigger to advance
///
/// Unlike FunctionState (which advances immediately), this state waits for:
/// - A signal from an external object, OR
/// - A call to complete() from within the function's callbacks
///
/// Useful for async operations like loading from vehicle where you call a function
/// and wait for a completion signal.
///
/// A state that only a later call to complete() or fail() ends needs a timeout: without one, and without a
/// completion connection, nothing ends it if that call never comes, and entering it is reported as a coding error.
///
/// A timeout ends the completion connections. A retry that stays in the state (RetryTransition) therefore calls
/// the function again, which starts the operation and connects its completion anew.
///
/// A step that is not needed every time has a skip predicate (setSkipPredicate()). When it returns true as the
/// state is entered, the function is not called and the state emits advance() like one whose step is done.
///
class AsyncFunctionState : public CompletionWaitState
{
    Q_OBJECT
    Q_DISABLE_COPY(AsyncFunctionState)

public:
    using SetupFunction = std::function<void(AsyncFunctionState* state)>;
    using SkipPredicate = std::function<bool()>;
    using SkipAction = std::function<void()>;

    /// @param stateName Name for this state (for logging)
    /// @param parent Parent state
    /// @param setupFunction Function called when state is entered. Should start the async operation.
    ///                      The function receives a pointer to this state so it can call complete() when done.
    /// @param timeoutMsecs Timeout in milliseconds, 0 for no timeout
    AsyncFunctionState(const QString& stateName, QState* parent, SetupFunction setupFunction, int timeoutMsecs = 0);

    /// @param skipPredicate Asked each time the state is entered: true skips the step. A retry does not ask again.
    /// @param skipAction Called when the step is skipped, for what the skipped step would have left behind
    void setSkipPredicate(SkipPredicate skipPredicate, SkipAction skipAction = nullptr);

protected:
    void onWaitEntered() override;
    void onWaitRestarted() override;

private:
    SetupFunction _setupFunction;
    SkipPredicate _skipPredicate;
    SkipAction _skipAction;
};
