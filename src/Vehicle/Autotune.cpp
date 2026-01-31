#include "Autotune.h"
#include "AutotuneStateMachine.h"

Autotune::Autotune(Vehicle *vehicle)
    : QObject(vehicle)
    , _vehicle(vehicle)
    , _stateMachine(new AutotuneStateMachine(this, vehicle, this))
{
    connect(_stateMachine, &AutotuneStateMachine::autotuneChanged, this, &Autotune::autotuneChanged);
}

void Autotune::autotuneRequest()
{
    _stateMachine->startAutotune();
}

void Autotune::ackHandler(void* resultHandlerData, int compId, const mavlink_command_ack_t& ack, Vehicle::MavCmdResultFailureCode_t failureCode)
{
    Q_UNUSED(compId);

    auto* autotune = static_cast<Autotune*>(resultHandlerData);
    if (!autotune || !autotune->_stateMachine) {
        return;
    }

    if (!autotune->_stateMachine->isInProgress()) {
        qWarning() << "Ack received but autotune not in progress";
        return;
    }

    if (failureCode == Vehicle::MavCmdResultCommandResultOnly) {
        if ((ack.result == MAV_RESULT_IN_PROGRESS) || (ack.result == MAV_RESULT_ACCEPTED)) {
            autotune->_stateMachine->handleProgress(ack.progress);
        } else if (ack.result == MAV_RESULT_FAILED) {
            autotune->_stateMachine->handleFailure();
        } else {
            autotune->_stateMachine->handleError(ack.result);
        }
    } else {
        autotune->_stateMachine->handleFailure();
    }
}

void Autotune::progressHandler(void* progressHandlerData, int compId, const mavlink_command_ack_t& ack)
{
    Q_UNUSED(compId);

    auto* autotune = static_cast<Autotune*>(progressHandlerData);
    if (!autotune || !autotune->_stateMachine) {
        return;
    }

    if (!autotune->_stateMachine->isInProgress()) {
        qWarning() << "Progress received but autotune not in progress";
        return;
    }

    autotune->_stateMachine->handleProgress(ack.progress);
}

bool Autotune::autotuneInProgress()
{
    return _stateMachine->isInProgress();
}

float Autotune::autotuneProgress()
{
    return _stateMachine->progress();
}

QString Autotune::autotuneStatus()
{
    return _stateMachine->statusString();
}
