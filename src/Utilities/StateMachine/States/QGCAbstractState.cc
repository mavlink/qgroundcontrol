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

void QGCAbstractState::setCallbacks(EntryCallback onEntry, ExitCallback onExit)
{
    _entryCallback = std::move(onEntry);
    _exitCallback = std::move(onExit);
}

void QGCAbstractState::onEntry(QEvent* event)
{
    QState::onEntry(event);
    qCDebug(QGCStateMachineLog) << "Entered" << stateName();

    if (_entryCallback) {
        _entryCallback();
    }

    onEnter();
}

void QGCAbstractState::onExit(QEvent* event)
{
    onLeave();

    if (_exitCallback) {
        _exitCallback();
    }

    QState::onExit(event);
    qCDebug(QGCStateMachineLog) << "Exited" << stateName();
}

bool QGCAbstractState::event(QEvent* event)
{
    if (_eventHandler && _eventHandler(event)) {
        return true;
    }

    return QState::event(event);
}
