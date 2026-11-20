#pragma once

#include "MAVLinkEnums.h"
#include "WaitStateBase.h"

/// \brief Sends the specified MAVLink command to the vehicle and waits for the result
///
/// Emits advance() when the vehicle accepts the command, error() for any other result. A result for another
/// component, and the refusal of the same command that something else sends meanwhile, are not its own.
///
class SendMavlinkCommandState : public WaitStateBase
{
    Q_OBJECT
    Q_DISABLE_COPY(SendMavlinkCommandState)

public:
    SendMavlinkCommandState(QState* parent, MAV_CMD command, double param1 = 0.0, double param2 = 0.0, double param3 = 0.0, double param4 = 0.0, double param5 = 0.0, double param6 = 0.0, double param7 = 0.0);
    SendMavlinkCommandState(QState* parent);

    void setup(MAV_CMD command, double param1 = 0.0, double param2 = 0.0, double param3 = 0.0, double param4 = 0.0, double param5 = 0.0, double param6 = 0.0, double param7 = 0.0);

protected:
    void connectWaitSignal() override;
    void disconnectWaitSignal() override;
    void onWaitEntered() override;

private slots:
    void _mavCommandResult(quint32 vehicleId, int targetComponent, int command, int ackResult, int failureCode);

private:
    static constexpr int _targetComponent = MAV_COMP_ID_AUTOPILOT1;

    MAV_CMD     _command = MAV_CMD(0);
    double      _param1 = 0.0;
    double      _param2 = 0.0;
    double      _param3 = 0.0;
    double      _param4 = 0.0;
    double      _param5 = 0.0;
    double      _param6 = 0.0;
    double      _param7 = 0.0;
    bool        _configured = false;
    bool _sendingCommand = false;
};
