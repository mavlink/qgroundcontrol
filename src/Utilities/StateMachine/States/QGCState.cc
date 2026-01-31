#include "QGCStateMachine.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QGCStateMachineLog, "Utilities.QGCStateMachine")

QGCState::QGCState(const QString& stateName, QState* parentState)
    : QState(QState::ExclusiveStates, parentState)
{
    setObjectName(stateName);
}

QGCStateMachine* QGCState::machine() const
{
    return qobject_cast<QGCStateMachine*>(QState::machine());
}

Vehicle* QGCState::vehicle() const
{
    return machine() ? machine()->vehicle() : nullptr;
}

StateContext* QGCState::context() const
{
    return machine() ? &machine()->context() : nullptr;
}

QString QGCState::stateName() const
{
    if (machine()) {
        return QStringLiteral("%1:%2").arg(objectName(), machine()->machineName());
    } else {
        return objectName();
    }
}

void QGCState::setCallbacks(EntryCallback onEntry, ExitCallback onExit)
{
    _entryCallback = std::move(onEntry);
    _exitCallback = std::move(onExit);
}

void QGCState::setLocalErrorState(QAbstractState* errorState)
{
    _localErrorState = errorState;
    if (errorState) {
        addTransition(this, &QGCState::error, errorState);
        qCDebug(QGCStateMachineLog) << stateName() << "set local error state";
    }
}

void QGCState::setProperty(QObject* object, const char* name, const QVariant& value)
{
    assignProperty(object, name, value);
}

void QGCState::setEnabled(QObject* object, bool enabled)
{
    assignProperty(object, "enabled", enabled);
}

void QGCState::setVisible(QObject* object, bool visible)
{
    assignProperty(object, "visible", visible);
}

void QGCState::onEntry(QEvent* event)
{
    QState::onEntry(event);  // Must be first - sets active property
    qCDebug(QGCStateMachineLog) << "Entered" << stateName();

    if (_entryCallback) {
        _entryCallback();
    }

    onEnter();
}

void QGCState::onExit(QEvent* event)
{
    onLeave();

    if (_exitCallback) {
        _exitCallback();
    }

    QState::onExit(event);

    qCDebug(QGCStateMachineLog) << "Exited" << stateName();
}

bool QGCState::event(QEvent* event)
{
    if (_eventHandler && _eventHandler(event)) {
        return true;
    }

    return QState::event(event);
}
