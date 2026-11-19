#include "WaitForParamResponseState.h"

#include <utility>

#include "MAVLinkLib.h"

WaitForParamResponseState::WaitForParamResponseState(QState* parent, int timeoutMsecs, Predicate paramValuePredicate,
                                                     Predicate paramErrorPredicate)
    : VehicleMessageWaitState(QStringLiteral("WaitForParamResponseState"), parent, timeoutMsecs)
    , _paramValuePredicate(std::move(paramValuePredicate))
    , _paramErrorPredicate(std::move(paramErrorPredicate))
{
}

void WaitForParamResponseState::onWaitEntered()
{
    // A wait that ends without a PARAM_ERROR must not show the error of an earlier one
    _lastParamError = MAV_PARAM_ERROR_NO_ERROR;
    _lastParamErrorString.clear();

    VehicleMessageWaitState::onWaitEntered();
}

void WaitForParamResponseState::handleMessage(const mavlink_message_t& message)
{
    if (message.msgid == MAVLINK_MSG_ID_PARAM_VALUE) {
        if (_paramValuePredicate && !_paramValuePredicate(message)) {
            return;
        }

        qCDebug(QGCStateMachineLog) << "Received PARAM_VALUE" << stateName();
        waitComplete();
        return;
    }

    if (message.msgid == MAVLINK_MSG_ID_PARAM_ERROR) {
        if (_paramErrorPredicate && !_paramErrorPredicate(message)) {
            return;
        }

        mavlink_param_error_t paramError{};
        mavlink_msg_param_error_decode(&message, &paramError);

        _lastParamError = paramError.error;
        _lastParamErrorString = _paramErrorToString(paramError.error);

        char paramId[MAVLINK_MSG_PARAM_ERROR_FIELD_PARAM_ID_LEN + 1] = {};
        (void) strncpy(paramId, paramError.param_id, MAVLINK_MSG_PARAM_ERROR_FIELD_PARAM_ID_LEN);

        qCDebug(QGCStateMachineLog) << "Received PARAM_ERROR for" << paramId
                                    << "error:" << _lastParamErrorString << stateName();
        waitFailed();
        return;
    }
}

QString WaitForParamResponseState::_paramErrorToString(uint8_t errorCode)
{
    switch (errorCode) {
    case MAV_PARAM_ERROR_NO_ERROR:
        return QStringLiteral("No error");
    case MAV_PARAM_ERROR_DOES_NOT_EXIST:
        return QStringLiteral("Parameter does not exist");
    case MAV_PARAM_ERROR_VALUE_OUT_OF_RANGE:
        return QStringLiteral("Value out of range");
    case MAV_PARAM_ERROR_PERMISSION_DENIED:
        return QStringLiteral("Permission denied");
    case MAV_PARAM_ERROR_COMPONENT_NOT_FOUND:
        return QStringLiteral("Component not found");
    case MAV_PARAM_ERROR_READ_ONLY:
        return QStringLiteral("Parameter is read-only");
    case MAV_PARAM_ERROR_TYPE_UNSUPPORTED:
        return QStringLiteral("Parameter type unsupported");
    case MAV_PARAM_ERROR_TYPE_MISMATCH:
        return QStringLiteral("Parameter type mismatch");
    case MAV_PARAM_ERROR_READ_FAIL:
        return QStringLiteral("Parameter read failed");
    default:
        return QStringLiteral("Unknown error (%1)").arg(errorCode);
    }
}
