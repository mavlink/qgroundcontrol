#include "QGCSignalTransition.h"
#include "QGCStateMachine.h"

QGCSignalTransition::QGCSignalTransition(QState* sourceState)
    : QSignalTransition(sourceState)
{
}

QGCSignalTransition::QGCSignalTransition(const QObject* sender, const char* signal, QState* sourceState)
    : QSignalTransition(sender, signal, sourceState)
{
}

QGCStateMachine* QGCSignalTransition::machine() const
{
    return qobject_cast<QGCStateMachine*>(QSignalTransition::machine());
}

Vehicle* QGCSignalTransition::vehicle() const
{
    auto* m = machine();
    return m ? m->vehicle() : nullptr;
}
