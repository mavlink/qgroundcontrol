#include "QGCLoggingCategory.h"
#include "QGCStateMachine.h"
#include "RuntimeScheduler.h"

QGC_LOGGING_CATEGORY(QGCStateMachineLog, "Utilities.StateMachine.QGCStateMachine")

QGCState::QGCState(const QString& stateName, QState* parentState)
    : QState(QState::ExclusiveStates, parentState)
{
    setObjectName(stateName);
}

QGCStateMachine* QGCState::machine() const
{
    return qgcStateMachineOf(this);
}

Vehicle* QGCState::vehicle() const
{
    return machine() ? machine()->vehicle() : nullptr;
}

QString QGCState::stateName() const
{
    return qualifiedStateName(this);
}

Vehicle* QGCState::vehicleOrWarn(const QLoggingCategory& (*category)()) const
{
    Vehicle* const stateVehicle = vehicle();
    if (!stateVehicle) {
        qCWarning(category) << "No vehicle available" << stateName();
    }
    return stateVehicle;
}

RuntimeScheduler* QGCState::scheduler()
{
    if (const QGCStateMachine* const stateMachine = machine()) {
        return stateMachine->scheduler();
    }

    _ownScheduler = RuntimeScheduler::orDefault(_ownScheduler, this);
    return _ownScheduler;
}

void QGCState::onEntry(QEvent* event)
{
    QState::onEntry(event);
    qCDebug(QGCStateMachineLog) << "Entered" << stateName();
}

void QGCState::onExit(QEvent* event)
{
    if (_exitCallback) {
        _exitCallback();
    }

    QState::onExit(event);
    qCDebug(QGCStateMachineLog) << "Exited" << stateName();
}
