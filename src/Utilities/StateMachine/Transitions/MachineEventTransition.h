#pragma once

#include "QGCAbstractTransition.h"
#include "QGCStateMachineEvent.h"

#include <QtCore/QString>
#include <functional>

/// Transition that fires when a named event is posted to the state machine
/// Use this for events posted via QGCStateMachine::postEvent/postDelayedEvent.
/// For events on watched objects, use NamedEventTransition instead.
class MachineEventTransition : public QGCAbstractTransition
{
    Q_OBJECT
    Q_DISABLE_COPY(MachineEventTransition)

public:
    using Guard = std::function<bool(const QGCStateMachineEvent*)>;

    /// Create transition for a named machine event
    /// @param eventName The event name to match
    /// @param target Target state for the transition
    MachineEventTransition(const QString& eventName, QAbstractState* target);

    /// Create transition for a named machine event with guard
    /// @param eventName The event name to match
    /// @param target Target state for the transition
    /// @param guard Predicate that must return true for transition to fire
    MachineEventTransition(const QString& eventName, QAbstractState* target, Guard guard);

    QString eventName() const { return _eventName; }

    /// Access the event data from the last matched event
    QVariant eventData() const { return _lastEventData; }

protected:
    bool eventTest(QEvent* event) override;
    void onTransition(QEvent* event) override;

private:
    QString _eventName;
    Guard _guard;
    QVariant _lastEventData;
};
