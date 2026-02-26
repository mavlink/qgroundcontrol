#include "MachineEventTransition.h"
#include "QGCState.h"

MachineEventTransition::MachineEventTransition(const QString& eventName, QAbstractState* target)
    : QGCAbstractTransition(target)
    , _eventName(eventName)
{
}

MachineEventTransition::MachineEventTransition(const QString& eventName, QAbstractState* target, Guard guard)
    : QGCAbstractTransition(target)
    , _eventName(eventName)
    , _guard(std::move(guard))
{
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

void MachineEventTransition::onTransition(QEvent* event)
{
    Q_UNUSED(event);
}
