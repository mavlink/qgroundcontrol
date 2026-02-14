#pragma once

#include "WaitStateBase.h"

#include <QtCore/QSet>
#include <QtCore/QString>

/// A state that waits for one or more named events before advancing.
///
/// This state integrates with QGCStateMachine's postEvent() and postDelayedEvent()
/// methods to provide event-based state coordination.
///
/// Example usage:
/// @code
/// // Create state that waits for "dataReady" event
/// auto* waitForData = new EventQueuedState("WaitForData", &machine, "dataReady", 5000);
///
/// // Wire transitions
/// waitForData->addTransition(waitForData, &EventQueuedState::completed, nextState);
/// waitForData->addTransition(waitForData, &EventQueuedState::timedOut, errorState);
///
/// // Later, from another state or external code:
/// machine.postEvent("dataReady");  // Triggers advancement
/// @endcode
///
/// Multiple events can be configured using addExpectedEvent().
/// The state advances when ANY of the expected events is received (OR logic).
/// For AND logic (wait for all events), chain multiple EventQueuedState instances.
class EventQueuedState : public WaitStateBase
{
    Q_OBJECT
    Q_DISABLE_COPY(EventQueuedState)

public:
    /// Create an EventQueuedState waiting for a single event
    /// @param stateName Name for logging
    /// @param parent Parent state
    /// @param eventName The event name to wait for
    /// @param timeoutMsecs Timeout in milliseconds (0 = no timeout)
    EventQueuedState(const QString& stateName, QState* parent,
                     const QString& eventName, int timeoutMsecs = 0);

    /// Create an EventQueuedState waiting for any of multiple events
    /// @param stateName Name for logging
    /// @param parent Parent state
    /// @param eventNames Set of event names to wait for (OR logic)
    /// @param timeoutMsecs Timeout in milliseconds (0 = no timeout)
    EventQueuedState(const QString& stateName, QState* parent,
                     const QSet<QString>& eventNames, int timeoutMsecs = 0);

    /// Add an additional event to wait for
    /// @param eventName Event name to add
    void addExpectedEvent(const QString& eventName);

    /// Remove an event from the expected set
    /// @param eventName Event name to remove
    void removeExpectedEvent(const QString& eventName);

    /// Get the set of expected event names
    QSet<QString> expectedEvents() const { return _expectedEvents; }

    /// Get the event that triggered completion (valid after completed() signal)
    QString receivedEvent() const { return _receivedEvent; }

signals:
    /// Emitted when an expected event is received, before completed()
    /// @param eventName The name of the received event
    void eventReceived(const QString& eventName);

protected:
    void connectWaitSignal() override;
    void disconnectWaitSignal() override;
    void onWaitEntered() override;

private slots:
    void _onMachineEvent(const QString& eventName);

private:
    QSet<QString> _expectedEvents;
    QString _receivedEvent;
    QMetaObject::Connection _eventConnection;
};
