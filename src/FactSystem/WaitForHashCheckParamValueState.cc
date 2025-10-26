/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "WaitForHashCheckParamValue.h"

#include "MultiVehicleManager.h"
#include "Vehicle.h"

#include "QGCLoggingCategory.h"

#include <QString>
#include <utility>
WaitForHashCheckParamValue::WaitForHashCheckParamValue(QState *parent, int timeoutMsecs)
    : QGCState(QStringLiteral("WaitForHashCheckParamValue"), parent)
    , _timeoutMsecs(timeoutMsecs > 0 ? timeoutMsecs : 0)
{
    connect(this, &QState::entered, this, &WaitForHashCheckParamValue::_onEntered);
    connect(this, &QState::exited, this, &WaitForHashCheckParamValue::_onExited);
    connect(&_timeoutTimer, &QTimer::timeout, this, &WaitForHashCheckParamValue::_onTimeout);

    _timeoutTimer.setSingleShot(true);
}

void WaitForHashCheckParamValue::_onEntered()
{
    connect(vehicle(), &Vehicle::mavlinkMessageReceived, this, &WaitForHashCheckParamValue::_messageReceived);

    if (_timeoutMsecs > 0) {
        _timeoutTimer.start(_timeoutMsecs);
    }
}

void WaitForHashCheckParamValue::_onExited()
{
    disconnect(vehicle(), &Vehicle::mavlinkMessageReceived, this, &WaitForHashCheckParamValue::_messageReceived);

    _timeoutTimer.stop();
}

void WaitForHashCheckParamValue::_messageReceived(const mavlink_message_t &message)
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
        qCDebug(QGCStateMachineLog) << "Received _HASH_CHECK PARAM_VALUE message" << stateName();
        emit advance();
    } else {
        qCDebug(QGCStateMachineLog) << "First PARAM_VALUE was not _HASH_CHECK" << parameterName << stateName();
        emit notFound();
    }
}

void WaitForHashCheckParamValue::_onTimeout()
{
    qCDebug(QGCStateMachineLog) << "Timeout waiting for _HASH_CHECK" << stateName();
    emit notFound();
}
