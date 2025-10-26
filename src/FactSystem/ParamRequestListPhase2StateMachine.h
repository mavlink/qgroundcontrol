/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCStateMachine.h"

/// Phase 2 of parameter request list state machine handles filling in the gaps of missing parameters
/// based on missing parameter indices.
class ParamRequestListPhase2StateMachine : public QGCStateMachine
{
    Q_OBJECT

public:
    ParamRequestListPhase2StateMachine(Vehicle* vehicle, uint8_t componentId, QObject* parent);
};