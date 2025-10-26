/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "WaitForHashCheckParamValueState.h"
#include "Vehicle.h"
#include "ParamHashCheckStateMachine.h"

WaitForHashCheckParamValueState::WaitForHashCheckParamValueState(int timeoutMsecs, QState *parentState)
    : QGCState(QStringLiteral("WaitForHashCheckParamValueState"), parentState)
    , _timeoutMsecs(timeoutMsecs > 0 ? timeoutMsecs : 0)
{
    connect(this, &QState::entered, this, &WaitForHashCheckParamValueState::_onEntered);
    connect(this, &QState::exited, this, &WaitForHashCheckParamValueState::_onExited);
    connect(&_timeoutTimer, &QTimer::timeout, this, &WaitForHashCheckParamValueState::_onTimeout);

    _timeoutTimer.setSingleShot(true);
}

void WaitForHashCheckParamValueState::_onEntered()
{
    connect(vehicle(), &Vehicle::mavlinkMessageReceived, this, &WaitForHashCheckParamValueState::_messageReceived);

    if (_timeoutMsecs > 0) {
        _timeoutTimer.start(_timeoutMsecs);
    }
}

void WaitForHashCheckParamValueState::_onExited()
{
    disconnect(vehicle(), &Vehicle::mavlinkMessageReceived, this, &WaitForHashCheckParamValueState::_messageReceived);

    _timeoutTimer.stop();
}

void WaitForHashCheckParamValueState::_messageReceived(const mavlink_message_t &message)
{
    if (message.msgid != MAVLINK_MSG_ID_PARAM_VALUE) {
        return;
    }

    mavlink_param_value_t param_value{};
    mavlink_msg_param_value_decode(&message, &param_value);

    // This will null terminate the name string
    char parameterNameWithNull[MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1] = {};
    (void) strncpy(parameterNameWithNull, param_value.param_id, MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN);
    const QString parameterName(parameterNameWithNull);

    if (parameterName == QStringLiteral("_HASH_CHECK")) {
        mavlink_param_union_t paramUnion{};
        paramUnion.param_float = param_value.param_value;
        paramUnion.type = param_value.param_type;

        QVariant parameterValue;
        if (!QGCMAVLink::mavlinkParamUnionToVariant(paramUnion, parameterValue)) {
            emit notFound();
            return;
        }
        uint32_t hashCRC = parameterValue.toUInt();

        qCDebug(ParamHashCheckStateMachineLog) << "Received _HASH_CHECK PARAM_VALUE message. Hash:" << hashCRC << stateName();

        emit found(hashCRC);
        emit advance();
    } else {
        qCDebug(ParamHashCheckStateMachineLog) << "First PARAM_VALUE was not _HASH_CHECK" << parameterName << stateName();
        emit notFound();
    }
}

void WaitForHashCheckParamValueState::_onTimeout()
{
    qCDebug(ParamHashCheckStateMachineLog) << "Timeout waiting for _HASH_CHECK" << stateName();
    emit notFound();
}
