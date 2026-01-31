#include "SendMavlinkCommandState.h"
#include "MultiVehicleManager.h"
#include "MissionCommandTree.h"
#include "Vehicle.h"

SendMavlinkCommandState::SendMavlinkCommandState(QState* parent, MAV_CMD command, double param1, double param2, double param3, double param4, double param5, double param6, double param7)
    : WaitStateBase(QStringLiteral("SendMavlinkCommandState"), parent, 0)
{
    setup(command, param1, param2, param3, param4, param5, param6, param7);
}

SendMavlinkCommandState::SendMavlinkCommandState(QState* parent)
    : WaitStateBase(QStringLiteral("SendMavlinkCommandState"), parent, 0)
{
}

void SendMavlinkCommandState::setup(MAV_CMD command, double param1, double param2, double param3, double param4, double param5, double param6, double param7)
{
    _command = command;
    _param1 = param1;
    _param2 = param2;
    _param3 = param3;
    _param4 = param4;
    _param5 = param5;
    _param6 = param6;
    _param7 = param7;
    _configured = true;
}

void SendMavlinkCommandState::connectWaitSignal()
{
    connect(vehicle(), &Vehicle::mavCommandResult, this, &SendMavlinkCommandState::_mavCommandResult);
}

void SendMavlinkCommandState::disconnectWaitSignal()
{
    if (vehicle()) {
        disconnect(vehicle(), &Vehicle::mavCommandResult, this, &SendMavlinkCommandState::_mavCommandResult);
    }
}

void SendMavlinkCommandState::onWaitEntered()
{
    if (!_configured) {
        qCWarning(QGCStateMachineLog) << "SendMavlinkCommandState not configured";
        waitFailed();
        return;
    }

    qCDebug(QGCStateMachineLog) << QStringLiteral("Sending %1 command").arg(MissionCommandTree::instance()->friendlyName(_command));

    vehicle()->sendMavCommand(MAV_COMP_ID_AUTOPILOT1,
                              _command,
                              false /* showError */,
                              static_cast<float>(_param1),
                              static_cast<float>(_param2),
                              static_cast<float>(_param3),
                              static_cast<float>(_param4),
                              static_cast<float>(_param5),
                              static_cast<float>(_param6),
                              static_cast<float>(_param7));
}

void SendMavlinkCommandState::_mavCommandResult(int vehicleId, int targetComponent, int command, int ackResult, int failureCode)
{
    Q_UNUSED(vehicleId);
    Q_UNUSED(targetComponent);

    Vehicle* senderVehicle = dynamic_cast<Vehicle*>(sender());
    if (!senderVehicle) {
        qWarning() << "Vehicle dynamic cast on sender() failed!";
        return;
    }
    if (senderVehicle != vehicle()) {
        return; // Not for us
    }
    if (command != _command) {
        return; // Not our command
    }

    QString commandName = MissionCommandTree::instance()->friendlyName(_command);

    if (failureCode == Vehicle::MavCmdResultFailureNoResponseToCommand) {
        qCDebug(QGCStateMachineLog) << QStringLiteral("%1 Command - No response from vehicle").arg(commandName);
        waitFailed();
    } else if (failureCode == Vehicle::MavCmdResultFailureDuplicateCommand) {
        qCWarning(QGCStateMachineLog) << QStringLiteral("%1 Command - Duplicate command pending").arg(commandName);
        waitFailed();
    } else if (ackResult != MAV_RESULT_ACCEPTED) {
        qCWarning(QGCStateMachineLog) << QStringLiteral("%1 Command failed = ack.result: %2").arg(commandName).arg(ackResult);
        waitFailed();
    } else {
        // MAV_RESULT_ACCEPTED
        qCDebug(QGCStateMachineLog) << QStringLiteral("%1 Command succeeded").arg(commandName);
        emit success();
        waitComplete();
    }
}
