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
    _updateErrorTransition();
    if (errorState) {
        qCDebug(QGCStateMachineLog) << stateName() << "set local error state";
    }
}

void QGCState::_setGlobalErrorState(QAbstractState* errorState)
{
    _globalErrorState = errorState;
    _updateErrorTransition();
}

void QGCState::_updateErrorTransition()
{
    // One transition, retargeted: of two transitions on error() Qt takes the one that was added first, whichever
    // error state it leads to
    QAbstractState* const target = _localErrorState ? _localErrorState : _globalErrorState;
    if (!target) {
        if (_errorTransition) {
            removeTransition(_errorTransition);
            delete _errorTransition;
            _errorTransition = nullptr;
        }
    } else if (_errorTransition) {
        _errorTransition->setTargetState(target);
    } else {
        _errorTransition = addTransition(this, &QGCState::error, target);
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
