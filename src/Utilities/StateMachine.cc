/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "StateMachine.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(StateMachineLog, "qgc.utilities.statemachine");

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
