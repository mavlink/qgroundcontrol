#pragma once

#include "WaitStateBase.h"

#include <functional>

/// Calls a function when entered and waits for an external trigger to advance
/// Unlike FunctionState (which advances immediately), this state waits for:
/// - A signal from an external object, OR
/// - A call to complete() from within the function's callbacks
///
/// Useful for async operations like loading from vehicle where you call a function
/// and wait for a completion signal.
class AsyncFunctionState : public WaitStateBase
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

    /// Call this to signal that the async operation has completed successfully
    void complete() { waitComplete(); }

    /// Call this to signal that the async operation has failed
    void fail() { waitFailed(); }

    /// Connect to an external signal that will trigger completion
    /// @param sender The QObject that will emit the completion signal
    /// @param signal The signal that indicates completion
    template<typename Func>
    void connectToCompletion(typename QtPrivate::FunctionPointer<Func>::Object* sender, Func signal)
    {
        _completionConnection = connect(sender, signal, this, [this]() {
            complete();
        });
    }

    /// Connect to an external signal that will trigger completion, with a slot
    /// Useful when you need to process the signal data before completing
    template<typename Func, typename Slot>
    void connectToCompletion(typename QtPrivate::FunctionPointer<Func>::Object* sender, Func signal, Slot slot)
    {
        _completionConnection = connect(sender, signal, this, slot);
    }

protected:
    void connectWaitSignal() override;
    void disconnectWaitSignal() override;
    void onWaitEntered() override;

private:
    SetupFunction _setupFunction;
    QMetaObject::Connection _completionConnection;
};
