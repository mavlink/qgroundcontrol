/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SendMavlinkCommandState.h"
#include "MultiVehicleManager.h"
#include "MissionCommandTree.h"
#include "Vehicle.h"

SendMavlinkCommandState::SendMavlinkCommandState(QState* parentState, MAV_CMD command, double param1, double param2, double param3, double param4, double param5, double param6, double param7)
    : QGCState("SendMavlinkCommandState", parentState)
{
    setup(command, param1, param2, param3, param4, param5, param6, param7);
}

SendMavlinkCommandState::SendMavlinkCommandState(QState* parentState)
    : QGCState("SendMavlinkCommandState", parentState)
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

    connect(this, &QState::entered, this, [this] () 
        { 
            qCDebug(QGCStateMachineLog) << QStringLiteral("Sending %1 command").arg(MissionCommandTree::instance()->friendlyName(_command)) << " - " << Q_FUNC_INFO;
            _sendMavlinkCommand();
        });

    connect(this, &QState::exited, this, &SendMavlinkCommandState::_disconnectAll);
}

void SendMavlinkCommandState::_sendMavlinkCommand()
{
    connect(vehicle(), &Vehicle::mavCommandResult, this, &SendMavlinkCommandState::_mavCommandResult);
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
        qWarning() << "Vehicle dynamic cast on sender() failed!" << " - " << Q_FUNC_INFO;
        return;
    }
    if (senderVehicle != vehicle()) {
        qCWarning(QGCStateMachineLog) << "Received mavCommandResult from unexpected vehicle" << " - " << Q_FUNC_INFO;
        return;
    }
    if (command != _command) {
        qCWarning(QGCStateMachineLog) << "Received mavCommandResult for unexpected command - expected:actual" << _command << command << " - " << Q_FUNC_INFO;
        return;
    }

    _disconnectAll();

    QString commandName = MissionCommandTree::instance()->friendlyName(_command);

    if (failureCode == Vehicle::MavCmdResultFailureNoResponseToCommand) {
        qCDebug(QGCStateMachineLog) << QStringLiteral("%1 Command - No response from vehicle").arg(commandName);
        emit error();
    } else if (failureCode == Vehicle::MavCmdResultFailureDuplicateCommand) {
        qCWarning(QGCStateMachineLog) << QStringLiteral("%1 Command - Duplicate command pending").arg(commandName);
        emit error();
    } else if (ackResult != MAV_RESULT_ACCEPTED) {
        qCWarning(QGCStateMachineLog) << QStringLiteral("%1 Command failed = ack.result: %2").arg(commandName).arg(ackResult);
        emit error();
    } else {
        // MAV_RESULT_ACCEPTED
        qCDebug(QGCStateMachineLog) << QStringLiteral("%1 Command succeeded").arg(commandName);
        emit advance();
    }
}

void SendMavlinkCommandState::_disconnectAll()
{
    disconnect(vehicle(), &Vehicle::mavCommandResult, this, &SendMavlinkCommandState::_mavCommandResult);
}