/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ParamRequestReadStateMachine.h"
#include "ParamHashCheckStateMachine.h"
#include "WatchForMavlinkMessageState.h"
#include "Vehicle.h"
#include "MAVLinkProtocol.h"
#include "ParameterManager.h"

ParamRequestReadStateMachine::ParamRequestReadStateMachine(Vehicle* vehicle, uint8_t componentId, const QString &paramName, int paramIndex, bool notifyFailure, QGCState* parentState)
    : QGCStateMachine(QStringLiteral("ParamRequestList"), vehicle, nullptr /* auto delete */)
    , _componentId(componentId)
    , _paramName(paramName)
    , _paramIndex(paramIndex)
    , _notifyFailure(notifyFailure)
{
    _setupStateGraph();
}

void ParamRequestReadStateMachine::_setupStateGraph()
{
    // State Machine:
    //  Send PARAM_REQUEST_READ - 2 retries after initial attempt
    //  Wait for PARAM_VALUE ack
    //
    //  timeout:
    //      Back up to PARAM_REQUEST_READ for retries
    //
    //  error:
    //      Notify user of failure

    auto paramRequestReadEncoder = [this](SendMavlinkMessageState *state, uint8_t systemId, uint8_t channel, mavlink_message_t *message) -> void {
        char param_id[MAVLINK_MSG_PARAM_REQUEST_READ_FIELD_PARAM_ID_LEN + 1] = {};
        (void) strncpy(param_id, _paramName.toLocal8Bit().constData(), MAVLINK_MSG_PARAM_REQUEST_READ_FIELD_PARAM_ID_LEN);

        (void) mavlink_msg_param_request_read_pack_chan(MAVLinkProtocol::instance()->getSystemId(),   // QGC system id
                                                        MAVLinkProtocol::getComponentId(),            // QGC component id
                                                        channel,
                                                        message,
                                                        static_cast<uint8_t>(vehicle()->id()),
                                                        static_cast<uint8_t>(_componentId),
                                                        param_id,
                                                        static_cast<int16_t>(_paramIndex));
    };

    auto checkForCorrectParamValue = [this](WaitForMavlinkMessageState *state, const mavlink_message_t &message) -> bool {
        if (message.compid != _componentId) {
            return false;
        }

        mavlink_param_value_t param_value{};
        mavlink_msg_param_value_decode(&message, &param_value);

        // This will null terminate the name string
        char parameterNameWithNull[MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN + 1] = {};
        (void) strncpy(parameterNameWithNull, param_value.param_id, MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN);
        const QString msgParamName(parameterNameWithNull);

        // Check that this is for the parameter we requested
        if (_paramIndex != -1) {
            // Index based request
            if (param_value.param_index != _paramIndex) {
                return false;
            }
        } else {
            // Name based request
            if (msgParamName != _paramName) {
                return false;
            }
        }

        return true;
    };

    // Create states
    auto stateMachine = new QGCStateMachine(QStringLiteral("PARAM_REQUEST_READ"), vehicle(), nullptr /* auto delete */);
    auto sendParamRequestReadState = new SendMavlinkMessageState(paramRequestReadEncoder, kParamRequestReadRetryCount, stateMachine);
    auto waitAckState = new WaitForMavlinkMessageState(MAVLINK_MSG_ID_PARAM_VALUE, kWaitForParamValueAckMs, checkForCorrectParamValue, stateMachine);
    auto userNotifyState = new ShowAppMessageState(QStringLiteral("Parameter read failed: param: %1 %2").arg(_paramName).arg(_vehicleIdAndComponentString(_componentId)), stateMachine);
    auto logSuccessState = new FunctionState(QStringLiteral("Log success"), [this](FunctionState */*state*/) {
        qCDebug(ParameterManagerLog) << "PARAM_REQUEST_READ succeeded: name:" << _paramName << "index" << _paramIndex << _vehicleIdAndComponentString(_componentId);
        emit _paramRequestReadSuccess(_componentId, _paramName, _paramIndex);
    }, stateMachine);
    auto logFailureState = new FunctionState(QStringLiteral("Log failure"), [this](FunctionState */*state*/) {
        qCDebug(ParameterManagerLog) << "PARAM_REQUEST_READ failed: param:" << _paramName << "index" << _paramIndex << _vehicleIdAndComponentString(_componentId);
        emit _paramRequestReadFailure(_componentId, _paramName, _paramIndex);
    }, stateMachine);
    auto finalState = new QGCFinalState(stateMachine);

    // Successful state machine transitions
    stateMachine->setInitialState(sendParamRequestReadState);
    sendParamRequestReadState->addThisTransition(&QGCState::advance, waitAckState);
    waitAckState->addThisTransition             (&QGCState::advance, logSuccessState);
    logSuccessState->addThisTransition          (&QGCState::advance, finalState);

    // Retry transitions
    waitAckState->addTransition(waitAckState, &WaitForMavlinkMessageState::timeout, sendParamRequestReadState); // Retry on timeout

    // Error transitions
    sendParamRequestReadState->addThisTransition(&QGCState::error, logFailureState); // Error is signaled after retries exhausted or internal error

    // Error state branching transitions
    if (_notifyFailure) {
        logFailureState->addThisTransition  (&QGCState::advance, userNotifyState);
    } else {
        logFailureState->addThisTransition  (&QGCState::advance, finalState);
    }
    userNotifyState->addThisTransition  (&QGCState::advance, finalState);
}

QString ParamRequestReadStateMachine::_vehicleIdAndComponentString(int componentId) const
{ 
    return vehicle()->parameterManager()->vehicleIdAndComponentString(componentId); 
}
