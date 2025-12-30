/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCState.h"
#include "QGCMAVLink.h"

class Vehicle;

/// Sends the specified MAVLink command to the vehicle
class SendMavlinkCommandState : public QGCState
{
    Q_OBJECT

public:
    SendMavlinkCommandState(QState* parent, MAV_CMD command, double param1 = 0.0, double param2 = 0.0, double param3 = 0.0, double param4 = 0.0, double param5 = 0.0, double param6 = 0.0, double param7 = 0.0);
    SendMavlinkCommandState(QState* parent);

    void setup(MAV_CMD command, double param1 = 0.0, double param2 = 0.0, double param3 = 0.0, double param4 = 0.0, double param5 = 0.0, double param6 = 0.0, double param7 = 0.0);
    
signals:
    void success();

private slots:
    void _mavCommandResult(int vehicleId, int targetComponent, int command, int ackResult, int failureCode);
    void _disconnectAll();

private:
    void _sendMavlinkCommand();

    MAV_CMD     _command;
    double      _param1 = 0.0;
    double      _param2 = 0.0;
    double      _param3 = 0.0;
    double      _param4 = 0.0;
    double      _param5 = 0.0;
    double      _param6 = 0.0;
    double      _param7 = 0.0;
};
