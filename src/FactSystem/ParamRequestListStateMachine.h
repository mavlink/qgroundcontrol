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

#include <QMap>
#include <QList>

class ParamRequestListStateMachine : public QGCStateMachine
{
    Q_OBJECT

public:
    using MissingParamIndicesMap = QMap<int, QList<int>>;   ///< Key: Component id, Value: List of missing parameter indices

    ParamRequestListStateMachine(Vehicle* vehicle, uint8_t componentId);

signals:
    void allParamsReceived();
    void parametersMissing(MissingParamIndicesMap missingParams);

private:
    void _setupStateGraph();
    QGCState* _createParallelState();
    QGCState* _createWatchForAllParamsState(QState* parentState);

    uint8_t _componentID;                           ///< Component id for which we are requesting parameters
    QMap<int, int> _paramCountMap;                  ///< Key: Component id, Value: Total number ofparameters in this component
    MissingParamIndicesMap _missingParamIndicesMap;

    static constexpr int kMaxParamRequestListRetries = 2;
    static constexpr int kWaitForParamValueMs = 1000;
};