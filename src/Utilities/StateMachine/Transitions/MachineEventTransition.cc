#include "MachineEventTransition.h"

#include "QGCStateMachine.h"

MachineEventTransition::MachineEventTransition(const QString& eventName, QAbstractState* target)
    : MachineEventTransition(eventName, target, Guard())
{
}

MachineEventTransition::MachineEventTransition(const QString& eventName, QAbstractState* target, Guard guard)
    : _eventName(eventName)
    , _guard(std::move(guard))
{
    setTargetState(target);
}

bool MachineEventTransition::eventTest(QEvent* event)
{
    if (event->type() != QGCStateMachineEvent::EventType) {
        return false;
    }

    auto* smEvent = static_cast<QGCStateMachineEvent*>(event);

    if (smEvent->name() != _eventName) {
        return false;
    }

    if (_guard && !_guard(smEvent)) {
        qCDebug(QGCStateMachineLog) << "MachineEventTransition" << _eventName << "blocked by guard";
        return false;
    }

    // Store data for access after transition
    _lastEventData = smEvent->data();

    qCDebug(QGCStateMachineLog) << "MachineEventTransition matched event:" << _eventName;
    return true;
}
