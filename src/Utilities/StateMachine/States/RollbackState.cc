#include "RollbackState.h"
#include "QGCLoggingCategory.h"

RollbackState::RollbackState(const QString& stateName, QState* parent,
                             ForwardAction forwardAction, RollbackAction rollbackAction)
    : QGCState(stateName, parent)
    , _forwardAction(std::move(forwardAction))
    , _rollbackAction(std::move(rollbackAction))
{
}

void RollbackState::onEnter()
{
    _wasRolledBack = false;
    _rollbackSucceeded = false;

    qCDebug(QGCStateMachineLog) << stateName() << "executing forward action";

    bool success = false;
    if (_forwardAction) {
        success = _forwardAction();
    }

    if (success) {
        qCDebug(QGCStateMachineLog) << stateName() << "forward action succeeded";
        emit forwardSucceeded();
        emit advance();
    } else {
        qCDebug(QGCStateMachineLog) << stateName() << "forward action failed, rolling back";
        emit forwardFailed();

        _wasRolledBack = true;
        emit rollingBack();

        if (_rollbackAction) {
            _rollbackAction();
            _rollbackSucceeded = true;
            qCDebug(QGCStateMachineLog) << stateName() << "rollback completed";
        }

        emit rollbackComplete();
        emit error();
    }
}
