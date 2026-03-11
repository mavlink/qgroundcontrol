#pragma once

#include <QtStateMachine/QEventTransition>
#include <functional>

/// Transition that fires when a specific QEvent is received by a watched object
/// Useful for reacting to timer events, mouse events, focus changes, etc.
class QGCEventTransition : public QEventTransition
{
    Q_OBJECT
    Q_DISABLE_COPY(QGCEventTransition)

public:
    using Guard = std::function<bool(QEvent*)>;

    /// Create an event transition
    /// @param object The object to watch for events
    /// @param eventType The event type to listen for
    /// @param target Target state for the transition
    QGCEventTransition(QObject* object, QEvent::Type eventType, QAbstractState* target);

    /// Create an event transition with a guard
    /// @param object The object to watch for events
    /// @param eventType The event type to listen for
    /// @param target Target state for the transition
    /// @param guard Predicate that must return true for transition to fire
    QGCEventTransition(QObject* object, QEvent::Type eventType, QAbstractState* target, Guard guard);

protected:
    bool eventTest(QEvent* event) override;

private:
    Guard _guard;
};
