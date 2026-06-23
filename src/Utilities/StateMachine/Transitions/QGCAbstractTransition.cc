#include "QGCAbstractTransition.h"
#include "QGCStateMachine.h"

QGCAbstractTransition::QGCAbstractTransition(QState* sourceState)
    : QAbstractTransition(sourceState)
{
}

QGCAbstractTransition::QGCAbstractTransition(QAbstractState* target, QState* sourceState)
    : QAbstractTransition(sourceState)
{
    setTargetState(target);
}

QGCStateMachine* QGCAbstractTransition::machine() const
{
    return qobject_cast<QGCStateMachine*>(QAbstractTransition::machine());
}

Vehicle* QGCAbstractTransition::vehicle() const
{
    auto* m = machine();
    return m ? m->vehicle() : nullptr;
}

void QGCAbstractTransition::onTransition(QEvent* event)
{
    Q_UNUSED(event);
    // Default implementation does nothing
}
