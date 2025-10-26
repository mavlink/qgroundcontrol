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

class ParamRequestListStateMachine : public QGCStateMachine
{
    Q_OBJECT

public:
    ParamRequestListStateMachine(Vehicle* vehicle, uint8_t componentId, QObject* parent);

private:
    void _setupStateGraph();

    uint8_t _componentID;                                   ///< Component id for which we are requesting parameters
    QMap<int, int> _paramCountMap;                          ///< Key: Component id, Value: Total number ofparameters in this component
    QMap<int, QMap<int, int>> _waitingReadParamIndexMap;    ///< Key: Component id, Value: Map { Key: parameter index still waiting for, Value: retry count }
};