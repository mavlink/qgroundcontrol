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


/// Phase 1 of parameter request list state machine sends the PARAM_REQUEST_LIST and waits for
/// all the initial PARAM_VALUE messages to come through
class ParamRequestListPhase1StateMachine : public QGCStateMachine
{
    Q_OBJECT

public:
    ParamRequestListPhase1StateMachine(Vehicle* vehicle, uint8_t componentId, QObject* parent);

private:
    void _setStateGraph();
    QGCState* _createParallelState();

    uint8_t _componentId;
    QMap<int, int> _paramCountMap;                      ///< Key: Component id, Value: count of parameters in this component
    QMap<int, QList<int>> _waitingReadParamIndexMap;    ///< Key: Component id, Value: List of parameter indices still waiting for
    QMap<int, QList<int>> _failedReadParamIndexMap;     ///< Key: Component id, Value: failed parameter index

    static constexpr int kMaxParamRequestListRetries = 2;
    static constexpr int kWaitForParamValueMs = 1000;
};