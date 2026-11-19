#include "QGCAbstractState.h"
#include "QGCStateMachine.h"

QGCAbstractState::QGCAbstractState(const QString& stateName, QState* parent)
    : QState(QState::ExclusiveStates, parent)
{
    setObjectName(stateName);
}

QGCStateMachine* QGCAbstractState::machine() const
{
    return qgcStateMachineOf(this);
}

Vehicle* QGCAbstractState::vehicle() const
{
    return machine() ? machine()->vehicle() : nullptr;
}

QString QGCAbstractState::stateName() const
{
    return qualifiedStateName(this);
}

void QGCAbstractState::onEntry(QEvent* event)
{
    QState::onEntry(event);
    qCDebug(QGCStateMachineLog) << "Entered" << stateName();
}

void QGCAbstractState::onExit(QEvent* event)
{
    if (_exitCallback) {
        _exitCallback();
    }

    QState::onExit(event);
    qCDebug(QGCStateMachineLog) << "Exited" << stateName();
}
