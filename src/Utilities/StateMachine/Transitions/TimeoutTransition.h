#pragma once

#include "QGCSignalTransition.h"

#include <QtCore/QTimer>

/// Transition that fires automatically after a specified delay.
/// Useful for adding timeout behavior to any state without requiring WaitStateBase.
///
/// The timer starts when the source state is entered and stops when exited.
/// If the timer expires while in the source state, the transition is taken.
///
/// Example usage:
/// @code
/// auto* timeoutTransition = new TimeoutTransition(5000, errorState);
/// normalState->addTransition(timeoutTransition);
/// @endcode
class TimeoutTransition : public QGCSignalTransition
{
    Q_OBJECT
    Q_DISABLE_COPY(TimeoutTransition)

public:
    /// Create a timeout transition
    /// @param timeoutMsecs Time in milliseconds before transition fires
    /// @param target Target state to transition to on timeout
    explicit TimeoutTransition(int timeoutMsecs, QAbstractState* target = nullptr);

    /// Get the timeout duration
    int timeoutMsecs() const { return _timeoutMsecs; }

    /// Set the timeout duration (only effective before state is entered)
    void setTimeoutMsecs(int msecs) { _timeoutMsecs = msecs; }

    /// Check if the timer is currently running
    bool isTimerActive() const { return _timer.isActive(); }

signals:
    /// Emitted when the timeout expires (internal use - triggers transition)
    void timeout();

protected:
    bool event(QEvent* e) override;
    void onTransition(QEvent* event) override;

private slots:
    void _onSourceStateEntered();
    void _onSourceStateExited();

private:
    void _onAddedToState();

    int _timeoutMsecs;
    QTimer _timer;
};
