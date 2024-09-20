/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ActuatorActions.h"
#include "QGCApplication.h"

using namespace ActuatorActions;

QString Config::typeToLabel() const
{
    switch (type) {
        case Type::beep: return QCoreApplication::translate("ActuatorAction", "Beep");
        case Type::set3DModeOn: return QCoreApplication::translate("ActuatorAction", "3D mode: On");
        case Type::set3DModeOff: return QCoreApplication::translate("ActuatorAction", "3D mode: Off");
        case Type::setSpinDirection1: return QCoreApplication::translate("ActuatorAction", "Set Spin Direction 1");
        case Type::setSpinDirection2: return QCoreApplication::translate("ActuatorAction", "Set Spin Direction 2");
    }
    return "";
}

Action::Action([[maybe_unused]] QObject *parent,
               const Config &action,
               const QString &label,
               int outputFunction,
               Vehicle *vehicle)
    : _label(label), _outputFunction(outputFunction), _type(action.type), _vehicle(vehicle)
{
}

void Action::trigger()
{
    if (_commandInProgress) {
        return;
    }
    sendMavlinkRequest();
}

void Action::ackHandlerEntry(void* resultHandlerData, int /*compId*/, const mavlink_command_ack_t& ack, Vehicle::MavCmdResultFailureCode_t failureCode)
{
    Action* action = (Action*)resultHandlerData;
    action->ackHandler(static_cast<MAV_RESULT>(ack.result), failureCode);
}

void Action::ackHandler(MAV_RESULT commandResult, Vehicle::MavCmdResultFailureCode_t failureCode)
{
    _commandInProgress = false;
    if (failureCode != Vehicle::MavCmdResultFailureNoResponseToCommand && commandResult != MAV_RESULT_ACCEPTED) {
        qgcApp()->showAppMessage(tr("Actuator action command failed"));
    }
}

void Action::sendMavlinkRequest()
{
    qCDebug(ActuatorsConfigLog) << "Sending actuator action, function:" << _outputFunction << "type:" << (int)_type;

    Vehicle::MavCmdAckHandlerInfo_t handlerInfo = {};
    handlerInfo.resultHandler       = ackHandlerEntry;
    handlerInfo.resultHandlerData   = this;

    _vehicle->sendMavCommandWithHandler(
            &handlerInfo,
            MAV_COMP_ID_AUTOPILOT1,           // the ID of the autopilot
            MAV_CMD_CONFIGURE_ACTUATOR,       // the mavlink command
            (int)_type,                       // action type
            0,                                // unused parameter
            0,                                // unused parameter
            0,                                // unused parameter
            1000+_outputFunction,             // function
            0,                                // unused parameter
            0);
    _commandInProgress = true;
}

ActionGroup::ActionGroup([[maybe_unused]] QObject *parent, const QString &label, Config::Type type)
    : _label(label), _type(type)
{
}
