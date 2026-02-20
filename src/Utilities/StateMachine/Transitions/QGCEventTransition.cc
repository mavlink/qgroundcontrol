#include "QGCEventTransition.h"
#include "QGCState.h"

QGCEventTransition::QGCEventTransition(QObject* object, QEvent::Type eventType, QAbstractState* target)
    : QEventTransition(object, eventType)
{
    setTargetState(target);
}

QGCEventTransition::QGCEventTransition(QObject* object, QEvent::Type eventType, QAbstractState* target, Guard guard)
    : QEventTransition(object, eventType)
    , _guard(std::move(guard))
{
    setTargetState(target);
}

bool QGCEventTransition::eventTest(QEvent* event)
{
    if (!QEventTransition::eventTest(event)) {
        return false;
    }

    if (_guard && !_guard(event)) {
        qCDebug(QGCStateMachineLog) << "QGCEventTransition blocked by guard";
        return false;
    }

    return true;
}
