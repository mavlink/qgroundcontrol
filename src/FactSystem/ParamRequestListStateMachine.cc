/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ParamRequestListStateMachine.h"
#include "ParamRequestListPhase1StateMachine.h"
#include "ParamRequestListPhase2StateMachine.h"

ParamRequestListStateMachine::ParamRequestListStateMachine(Vehicle* vehicle, uint8_t componentID, QObject* parent)
    : QGCStateMachine(QStringLiteral("ParamRequestList"), vehicle, parent)
    , _componentID(componentID)
{
    _setupStateGraph();
}

void ParamRequestListStateMachine::_setupStateGraph()
{
    // State Graph:
    //  ParamRequestListPhase1StateMachine
    //  ParamRequestListPhase2StateMachine

    auto phase1State = new ParamRequestListPhase1StateMachine(vehicle(), _componentID, this);
    auto phase2State = new ParamRequestListPhase2StateMachine(vehicle(), _componentID, this);
    auto finalState = new QGCFinalState(this);

    setInitialState(phase1State);

    phase1State->addTransition(phase1State, &QGCStateMachine::finished, phase2State);
    phase2State->addTransition(phase2State, &QGCStateMachine::finished, finalState);
}