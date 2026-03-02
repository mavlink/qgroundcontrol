#include "QGCAbstractState.h"
#include "QGCStateMachine.h"
#include "QGCLoggingCategory.h"

QGCAbstractState::QGCAbstractState(const QString& stateName, QState* parent)
    : QState(QState::ExclusiveStates, parent)
{
    setObjectName(stateName);
}

QGCStateMachine* QGCAbstractState::machine() const
{
    return qobject_cast<QGCStateMachine*>(QState::machine());
}

Vehicle* QGCAbstractState::vehicle() const
{
    return machine() ? machine()->vehicle() : nullptr;
}

StateContext* QGCAbstractState::context() const
{
    return machine() ? &machine()->context() : nullptr;
}

QString QGCAbstractState::stateName() const
{
    if (machine()) {
        return QStringLiteral("%1:%2").arg(machine()->machineName(), objectName());
    } else {
        return objectName();
    }
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
