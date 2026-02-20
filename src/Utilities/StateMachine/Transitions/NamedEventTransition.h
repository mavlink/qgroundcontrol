#pragma once

#include "QGCAbstractTransition.h"
#include "QGCStateMachineEvent.h"

#include <QtCore/QString>
#include <functional>

/// Transition that fires when a named QGCStateMachineEvent is posted to the state machine
/// Uses QAbstractTransition (via QGCAbstractTransition) to intercept events posted via
/// QStateMachine::postEvent() / QGCStateMachine::postEvent().
class NamedEventTransition : public QGCAbstractTransition
{
    Q_OBJECT
    Q_DISABLE_COPY(NamedEventTransition)

public:
    using NamedGuard = std::function<bool(const QGCStateMachineEvent*)>;

    /// Create transition for named events
    /// @param eventName The event name to match
    /// @param target Target state for the transition
    /// @param sourceState Optional source state (defaults to nullptr)
    NamedEventTransition(const QString& eventName, QAbstractState* target, QState* sourceState = nullptr);

    /// Create transition for named events with a guard
    /// @param eventName The event name to match
    /// @param target Target state for the transition
    /// @param guard Predicate that must return true for transition to fire
    /// @param sourceState Optional source state (defaults to nullptr)
    NamedEventTransition(const QString& eventName, QAbstractState* target, NamedGuard guard, QState* sourceState = nullptr);

    QString eventName() const { return _eventName; }

protected:
    bool eventTest(QEvent* event) override;

private:
    QString _eventName;
    NamedGuard _namedGuard;
};
