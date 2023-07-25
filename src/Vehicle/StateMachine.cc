/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "StateMachine.h"

StateMachine::StateMachine(void)
{

}

void StateMachine::start(void)
{
    _active = true;
    advance();
}

void StateMachine::advance(void)
{
    if (_active) {
        _stateIndex++;
        if (_stateIndex < stateCount()) {
            (*rgStates()[_stateIndex])(this);
        } else {
            _active = false;
            statesCompleted();
        }
    }
}

void StateMachine::move(StateFn stateFn)
{
    if (_active) {
        for (int i=0; i<stateCount(); i++) {
            if (rgStates()[i] == stateFn) {
                _stateIndex = i;
                (*rgStates()[_stateIndex])(this);
                break;
            }
        }
    }
}

void StateMachine::statesCompleted(void) const
{

}

StateMachine::StateFn StateMachine::currentState(void) const
{
    if (_active) {
        return rgStates()[_stateIndex];
    } else {
        return nullptr;
    }
}
