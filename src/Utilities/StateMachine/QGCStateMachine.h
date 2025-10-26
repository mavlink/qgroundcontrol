/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "DelayState.h"
#include "FunctionState.h"
#include "SendMavlinkCommandState.h"
#include "SendMavlinkMessageState.h"
#include "WaitForMavlinkMessageState.h"
#include "ShowAppMessageState.h"
#include "QGCFinalState.h"

#include <QStateMachine>
#include <QFinalState>
#include <QString>

#include <functional>

class Vehicle;

/// QGroundControl specific state machine
class QGCStateMachine : public QStateMachine
{
    Q_OBJECT
public:
    /// @param machineName Name of the state machine, for logging
    /// @param vehicle Vehicle associated with this state machine, can be nullptr
    /// @param parentState Parent state for the state machine, if nullptr the object will be automatically deleted when finished
    QGCStateMachine(const QString& machineName, Vehicle* vehicle, QState* parentState);

    Vehicle *vehicle() const { return _vehicle; }
    QString machineName() const { return objectName(); }

signals:
    void error();

private:
    Vehicle *_vehicle = nullptr;
};

