#include "QGCAbstractState.h"
#include "QGCStateMachine.h"
#include "QGCLoggingCategory.h"

QGCAbstractState::QGCAbstractState(const QString& stateName, QState* parent)
    : QAbstractState(parent)
{
    setObjectName(stateName);
}

QGCStateMachine* QGCAbstractState::machine() const
{
    return qobject_cast<QGCStateMachine*>(QAbstractState::machine());
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
        return QStringLiteral("%1:%2").arg(objectName(), machine()->machineName());
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
    Q_UNUSED(event);
    // Note: QAbstractState::onEntry is pure virtual - no base implementation to call
    // The 'active' property is managed by QStateMachine when it enters this state
    qCDebug(QGCStateMachineLog) << "Entered" << stateName();

    if (_entryCallback) {
        _entryCallback();
    }

    onEnter();
}

void QGCAbstractState::onExit(QEvent* event)
{
    Q_UNUSED(event);
    onLeave();

    if (_exitCallback) {
        _exitCallback();
    }

    qCDebug(QGCStateMachineLog) << "Exited" << stateName();
    // Note: QAbstractState::onExit is pure virtual - no base implementation to call
}

bool QGCAbstractState::event(QEvent* event)
{
    if (_eventHandler && _eventHandler(event)) {
        return true;
    }

    return QAbstractState::event(event);
}
