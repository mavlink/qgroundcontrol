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

Vehicle* QGCState::vehicleOrWarn(const QLoggingCategory& (*category)()) const
{
    Vehicle* const stateVehicle = vehicle();
    if (!stateVehicle) {
        qCWarning(category) << "No vehicle available" << stateName();
    }
    return stateVehicle;
}
