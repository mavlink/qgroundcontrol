#include "QGCStateMachine.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QGCStateMachineLog, "Utilities.QGCStateMachine")

QGCState::QGCState(const QString& stateName, QState* parentState)
    : QGCAbstractState(stateName, parentState)
{
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
