#include "NamedEventTransition.h"
#include "QGCState.h"

NamedEventTransition::NamedEventTransition(const QString& eventName, QAbstractState* target, QState* sourceState)
    : QGCAbstractTransition(target, sourceState)
    , _eventName(eventName)
{
}

NamedEventTransition::NamedEventTransition(const QString& eventName, QAbstractState* target, NamedGuard guard, QState* sourceState)
    : QGCAbstractTransition(target, sourceState)
    , _eventName(eventName)
    , _namedGuard(std::move(guard))
{
}

bool NamedEventTransition::eventTest(QEvent* event)
{
    // Only handle QGCStateMachineEvent events
    if (event->type() != QGCStateMachineEvent::EventType) {
        return false;
    }

    auto* smEvent = static_cast<QGCStateMachineEvent*>(event);

    if (smEvent->name() != _eventName) {
        return false;
    }

    if (_namedGuard && !_namedGuard(smEvent)) {
        qCDebug(QGCStateMachineLog) << "NamedEventTransition" << _eventName << "blocked by guard";
        return false;
    }

    qCDebug(QGCStateMachineLog) << "NamedEventTransition matched event:" << _eventName;
    return true;
}
