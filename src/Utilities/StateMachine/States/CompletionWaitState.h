#pragma once

#include <QtCore/QMetaObject>

#include "SignalSender.h"
#include "WaitStateBase.h"

/// \brief Base class for states that wait until a signal they connected to is emitted, or until complete() or
/// fail() is called
///
/// The connection made with connectToCompletion() is the wait signal of the state: it is removed when the state
/// completes, fails, times out or is exited.
///
class CompletionWaitState : public WaitStateBase
{
    Q_OBJECT
    Q_DISABLE_COPY(CompletionWaitState)

public:
    /// Call this to signal that the async operation has completed successfully
    void complete() { waitComplete(); }

    /// Call this to signal that the async operation has failed
    void fail() { waitFailed(); }

    /// Connect to an external signal that will trigger completion
    /// @param sender The QObject that will emit the completion signal
    /// @param signal The signal that indicates completion
    template <typename Func>
    void connectToCompletion(SignalSender<Func>* sender, Func signal)
    {
        _completionConnection = connect(sender, signal, this, [this]() { complete(); });
    }

    /// Connect to an external signal that will trigger completion, with a slot
    /// Useful when you need to process the signal data before completing
    template <typename Func, typename Slot>
    void connectToCompletion(SignalSender<Func>* sender, Func signal, Slot slot)
    {
        _completionConnection = connect(sender, signal, this, slot);
    }

protected:
    CompletionWaitState(const QString& stateName, QState* parent, int timeoutMsecs);

    /// Nothing to connect: the connection is made with connectToCompletion() once the state is entered
    void connectWaitSignal() override {}

    void disconnectWaitSignal() override;

    bool hasCompletionConnection() const { return static_cast<bool>(_completionConnection); }

private:
    QMetaObject::Connection _completionConnection;
};
