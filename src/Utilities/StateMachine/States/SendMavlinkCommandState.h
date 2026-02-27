#pragma once

#include "WaitStateBase.h"
#include "QGCMAVLink.h"

class Vehicle;

/// Sends the specified MAVLink command to the vehicle and waits for the result
class SendMavlinkCommandState : public WaitStateBase
{
    Q_OBJECT
    Q_DISABLE_COPY(SendMavlinkCommandState)

public:
    SendMavlinkCommandState(QState* parent, MAV_CMD command, double param1 = 0.0, double param2 = 0.0, double param3 = 0.0, double param4 = 0.0, double param5 = 0.0, double param6 = 0.0, double param7 = 0.0);
    SendMavlinkCommandState(QState* parent);

    void setup(MAV_CMD command, double param1 = 0.0, double param2 = 0.0, double param3 = 0.0, double param4 = 0.0, double param5 = 0.0, double param6 = 0.0, double param7 = 0.0);

signals:
    void success();

protected:
    void connectWaitSignal() override;
    void disconnectWaitSignal() override;
    void onWaitEntered() override;

private slots:
    void _mavCommandResult(int vehicleId, int targetComponent, int command, int ackResult, int failureCode);

private:
    MAV_CMD     _command = MAV_CMD(0);
    double      _param1 = 0.0;
    double      _param2 = 0.0;
    double      _param3 = 0.0;
    double      _param4 = 0.0;
    double      _param5 = 0.0;
    double      _param6 = 0.0;
    double      _param7 = 0.0;
    bool        _configured = false;
};
