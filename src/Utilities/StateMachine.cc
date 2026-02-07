#include "StateMachine.h"
#include <QtCore/QLoggingCategory>

Q_STATIC_LOGGING_CATEGORY(StateMachineLog, "Utilities.StateMachine");

StateMachine::StateMachine(QObject *parent)
    : QObject(parent)
{
    // qCDebug(StateMachineLog) << Q_FUNC_INFO << this;
}

StateMachine::~StateMachine()
{
    // qCDebug(StateMachineLog) << Q_FUNC_INFO << this;
}

void StateMachine::start()
{
    _active = true;
    advance();
}

void StateMachine::advance()
{
    if (!_active) {
        return;
    }

    _stateIndex++;
    if (_stateIndex < stateCount()) {
        (*rgStates()[_stateIndex])(this);
    } else {
        _active = false;
        statesCompleted();
    }
}

void StateMachine::move(StateFn stateFn)
{
    if (!_active) {
        return;
    }

    for (int i = 0; i < stateCount(); i++) {
        if (rgStates()[i] == stateFn) {
            _stateIndex = i;
            (*rgStates()[_stateIndex])(this);
            break;
        }
    }
}

StateMachine::StateFn StateMachine::currentState() const
{
    return (_active ? rgStates()[_stateIndex] : nullptr);
}
