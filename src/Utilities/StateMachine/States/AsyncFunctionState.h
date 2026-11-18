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
class AsyncFunctionState : public CompletionWaitState
{
    Q_OBJECT
    Q_DISABLE_COPY(AsyncFunctionState)

public:
    using SetupFunction = std::function<void(AsyncFunctionState* state)>;

    /// @param stateName Name for this state (for logging)
    /// @param parent Parent state
    /// @param setupFunction Function called when state is entered. Should start the async operation.
    ///                      The function receives a pointer to this state so it can call complete() when done.
    /// @param timeoutMsecs Timeout in milliseconds, 0 for no timeout
    AsyncFunctionState(const QString& stateName, QState* parent, SetupFunction setupFunction, int timeoutMsecs = 0);

protected:
    void onWaitEntered() override;

private:
    SetupFunction _setupFunction;
};
