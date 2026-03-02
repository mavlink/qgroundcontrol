#pragma once

#include "QGCState.h"

#include <QtCore/QTimer>

/// Base class for states that wait for something with optional timeout
/// Provides common timeout handling infrastructure
class WaitStateBase : public QGCState
{
    Q_OBJECT
    Q_DISABLE_COPY(WaitStateBase)

public:
    /// @param stateName Name for this state (for logging)
    /// @param parent Parent state
    /// @param timeoutMsecs Timeout in milliseconds, 0 for no timeout
    WaitStateBase(const QString& stateName, QState* parent, int timeoutMsecs = 0);

    /// Rearm wait signal connections and timeout after a handled timeout event.
    /// Intended for retry loops that stay in the same state.
    void restartWait();

signals:
    /// Emitted when the wait condition is satisfied (alias for advance())
    /// Prefer using completed() over advance() for clarity in wait states
    void completed();

    /// Emitted when the timeout expires before the wait condition is met
    /// @note For transition wiring, timedOut() is preferred over timeout() for clarity
    void timeout();

    /// Emitted when the timeout expires (alias for timeout())
    /// Prefer using timedOut() over timeout() for consistency with other signals
    void timedOut();

protected:
    /// Called when the state is entered - subclasses should call base implementation
    virtual void onWaitEntered();

    /// Called when the state is exited - subclasses should call base implementation
    virtual void onWaitExited();

    /// Called when the timeout expires - default emits timeout(), subclasses can override
    virtual void onWaitTimeout();

    /// Call this when the wait condition is satisfied
    /// Stops the timer, calls cleanup, and emits advance()
    void waitComplete();

    /// Call this when the wait fails
    /// Stops the timer, calls cleanup, and emits error()
    void waitFailed();

    /// Subclasses override to set up their signal connections
    virtual void connectWaitSignal() = 0;

    /// Subclasses override to tear down their signal connections
    virtual void disconnectWaitSignal() = 0;

    int timeoutMsecs() const { return _timeoutMsecs; }

private slots:
    void _onEntered();
    void _onExited();
    void _onTimeout();

private:
    int _timeoutMsecs = 0;
    QTimer _timeoutTimer;
    bool _completed = false;
};
