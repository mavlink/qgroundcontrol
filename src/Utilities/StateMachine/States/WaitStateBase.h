#pragma once

#include "QGCState.h"

#include <QtCore/QTimer>

/// \brief Base class for states that wait for something with optional timeout
///
/// Provides common timeout handling infrastructure
///
/// The wait ends when the state is exited, and when its machine stops: Qt exits no state of a machine that stops,
/// so the timeout and the wait signal would otherwise outlive the run.
///
class WaitStateBase : public QGCState
{
    Q_OBJECT
    Q_DISABLE_COPY(WaitStateBase)

public:
    /// @param stateName Name for this state (for logging)
    /// @param parent Parent state
    /// @param timeoutMsecs Timeout in milliseconds, 0 for no timeout
    WaitStateBase(const QString& stateName, QState* parent, int timeoutMsecs = 0);

    /// Wait again after a timeout, with the wait signal connected and the full timeout. For retries that stay in
    /// the state, as RetryTransition makes them.
    void restartWait();

signals:
    /// Emitted when the timeout expires before the wait condition is met. A wait that is satisfied emits advance(),
    /// one that fails emits error().
    void timedOut();

protected:
    /// Called when the state is entered - subclasses should call base implementation
    virtual void onWaitEntered() {}

    /// Called when the state is exited, or when its machine stops while the state is active - subclasses should
    /// call base implementation
    virtual void onWaitExited() {}

    /// Called when the timeout expires - default emits timedOut(), subclasses can override
    virtual void onWaitTimeout();

    /// Called by restartWait() once the wait signal is connected again, for what else the wait needs to begin anew
    virtual void onWaitRestarted() {}

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

    /// @return true once waitComplete() or waitFailed() has ended the wait that began when the state was entered
    bool waitEnded() const { return _completed; }

private slots:
    void _onEntered();
    void _stopWaiting();
    void _onTimeout();

private:
    void _startTimeoutTimer();

    int _timeoutMsecs = 0;
    QTimer _timeoutTimer;
    bool _completed = false;
    bool _waiting = false;
};
