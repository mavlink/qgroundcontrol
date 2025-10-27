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

class ParamRequestReadStateMachine : public QGCStateMachine
{
    Q_OBJECT

public:
    using MissingParamIndicesMap = QMap<int, QList<int>>;   ///< Key: Component id, Value: List of missing parameter indices

    ParamRequestReadStateMachine(Vehicle* vehicle, uint8_t componentId, const QString &paramName, int paramIndex, bool notifyFailure, QGCState* parentState);

signals:
    // Used by unit tests
    void _paramRequestReadSuccess(int componentId, const QString &paramName, int paramIndex);
    void _paramRequestReadFailure(int componentId, const QString &paramName, int paramIndex);

private:
    void _setupStateGraph();
    QString _vehicleIdAndComponentString(int componentId) const;

    uint8_t _componentId;
    QString _paramName;
    int _paramIndex;
    bool _notifyFailure;

    static constexpr int kParamRequestReadRetryCount = 2;
    static constexpr int kWaitForParamValueAckMs = 1000;
};