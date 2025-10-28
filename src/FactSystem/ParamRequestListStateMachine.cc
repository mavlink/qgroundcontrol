/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ParamRequestListStateMachine.h"
#include "ParamHashCheckStateMachine.h"
#include "QGCParallelState.h"
#include "WatchForMavlinkMessageState.h"
#include "Vehicle.h"
#include "ParameterManager.h"

ParamRequestListStateMachine::ParamRequestListStateMachine(Vehicle* vehicle, uint8_t componentID)
    : QGCStateMachine(QStringLiteral("ParamRequestList"), vehicle, nullptr /* auto delete */)
    , _componentID(componentID)
{
    _setupStateGraph();
}

void ParamRequestListStateMachine::_setupStateGraph()
{
    // States:
    //  Send PARAM_REQUEST_LIST - 2 retries after initial attempt
    //      Signal: advance - on successful send
    //      Signal: error - on retries exhausted, or internal error
    //  Watch for PARAM_VALUE, marking seen param indices
    //      Signal: allParamsReceived - all parameters received from all components
    //      Signal: receivedParamValue - each time a PARAM_VALUE is received
    //      Signal: timeoutBeforeFirstMessageReceived - timeout waiting for first PARAM_VALUE messages
    //      Signal: timeoutAfterFirstMessageReceived - no PARAM_VALUE received within timeout period after at least one message has been received
    //  Wait for _HASH_CHECK PARAM_VALUE - If _HASH_CHECK is the first PARAM_VALUE try cache loading
    //      Signals: timeout - no PARAM_VALUE received within timeout period
    //  Request missing parameter indices
    //      Signals: advance - all missing params have been requested
    
    // 1: Send PARAM_REQUEST_LIST
    // 2: Parallel composite state
    //      2.1: Composite state - Index tracking
    //          2.1.1: Watch for PARAM_VALUE
    //              Transition on timeoutAfterFirstMessageReceived: Next
    //          2.1.2: Watch for PARAM_VALUE
    //              Transition on timeoutAfterFirstMessageReceived: Next
    //          2.1.3: Final state
    //      2.2: Compsite State - Main request list handling:
    //          2.2.1: Wait for _HASH_CHECK PARAM_VALUE
    //              Transition on 2.1.1.timeoutAfterFirstMessageReceived: Next
    //          2.2.2: Request missing parameter indices
    //      Transition on 2.1.1.allParamsReceived: 3
    //      Transition on 2.1.1.timeoutBeforeFirstMessageReceived: 1
    //      Transition on 2.1.2.allParamsReceived: 3
    //      Transition on 2.2.2.advance: 3
    // 3: Param request list complete
    // 4: Final state

    _paramCountMap.clear();
    _missingParamIndicesMap.clear();

    auto paramRequestListEncoder = [this](SendMavlinkMessageState */*state*/, uint8_t systemId, uint8_t componentId, mavlink_message_t* message) {
        mavlink_msg_param_request_list_pack(systemId,
                                           componentId,
                                           message,
                                           vehicle()->id(),
                                           _componentID);
    };

    // Create states
    auto sendParamRequestListState = new SendMavlinkMessageState(paramRequestListEncoder, kMaxParamRequestListRetries, this);
    auto parallelState = new QGCParallelState(QStringLiteral("Parallel container"), this);
    auto indexTrackingCompositeState = new QGCState(QStringLiteral("IndexTracking composite state"), parallelState);
    auto watchForAllParamsState1 = _createWatchForAllParamsState(indexTrackingCompositeState);
    auto watchForAllParamsState2 = _createWatchForAllParamsState(indexTrackingCompositeState);
    auto finalIndexTrackingState = new QGCFinalState(indexTrackingCompositeState);
    auto mainRequestListCompositeState = new QGCState(QStringLiteral("MainRequestList composite state"), parallelState);
    auto waitForHashCheckState = new ParamHashCheckStateMachine(vehicle(), mainRequestListCompositeState);
    auto requestMissingParamsState = _createRequestMissingParamsState(mainRequestListCompositeState);
    auto requestCompleteState = new FunctionState(QStringLiteral("Request Complete"), std::bind(&ParamRequestListStateMachine::_paramRequestListComplete, this, std::placeholders::_1), this);
    auto finalMainRequestListState = new QGCFinalState(mainRequestListCompositeState);
    auto finalState = new QGCFinalState(this);

    // Transitions

    setInitialState(sendParamRequestListState);
    sendParamRequestListState->addThisTransition(&SendMavlinkMessageState::advance, parallelState);
    parallelState->addThisTransition(&QState::finished, requestCompleteState);
    parallelState->addTransition(this, &ParamRequestListStateMachine::allParamsReceived, requestCompleteState);
    parallelState->addTransition(watchForAllParamsState1, &WatchForMavlinkMessageState::timeoutBeforeFirstMessageReceived, sendParamRequestListState);
    requestCompleteState->addThisTransition(&QGCState::advance, finalState);

    parallelState->setInitialState(indexTrackingCompositeState);

    indexTrackingCompositeState->setInitialState(watchForAllParamsState1);
    watchForAllParamsState1->addThisTransition(&WatchForMavlinkMessageState::timeoutAfterFirstMessageReceived, watchForAllParamsState2);
    watchForAllParamsState2->addThisTransition(&WatchForMavlinkMessageState::timeoutBeforeFirstMessageReceived, finalIndexTrackingState);
    watchForAllParamsState2->addThisTransition(&WatchForMavlinkMessageState::timeoutAfterFirstMessageReceived, finalIndexTrackingState);

    mainRequestListCompositeState->setInitialState(waitForHashCheckState);
    waitForHashCheckState->addTransition(watchForAllParamsState1, &WatchForMavlinkMessageState::timeoutAfterFirstMessageReceived, requestMissingParamsState);
    requestMissingParamsState->addThisTransition(&QGCState::advance, finalMainRequestListState);
}

void ParamRequestListStateMachine::_paramRequestListComplete(FunctionState */* state */)
{
    qCDebug(ParameterManagerLog) << "Parameter request list complete";
    _parametersReady = true;
    _vehicle->autopilotPlugin()->parametersReadyPreChecks();
    emit parametersReadyChanged(true);
    emit missingParametersChanged(_missingParameters);
}

QGCState* ParamRequestListStateMachine::_createWatchForAllParamsState(QState* parentState)
{
    auto compositeParentState = new QGCState(QStringLiteral("WatchForAllParamsState"), parentState);

    auto processParamValue = [this](WatchForMavlinkMessageState *state, const mavlink_message_t &message) -> bool {
        mavlink_param_value_t param_value{};
        mavlink_msg_param_value_decode(&message, &param_value);

        const int componentId = message.compid;
        const int paramIndex = static_cast<int>(param_value.param_index);

        if (paramIndex < 0 || paramIndex >= param_value.param_count) {
            return true;    // Continue processing further PARAM_VALUE messages
        }

        if (!_paramCountMap.contains(componentId)) {
            _paramCountMap[componentId] = static_cast<int>(param_value.param_count);
            QList<int> waitingList;
            waitingList.reserve(static_cast<int>(param_value.param_count));
            for (int i = 0; i < param_value.param_count; ++i) {
                waitingList.append(i);
            }
            _missingParamIndicesMap[componentId] = waitingList;
        }

        if (_missingParamIndicesMap.contains(componentId)) {
            _missingParamIndicesMap[componentId].removeAll(paramIndex);
            if (_missingParamIndicesMap[componentId].isEmpty()) {
                _missingParamIndicesMap.remove(componentId);
            }
        }

        if (_missingParamIndicesMap.isEmpty()) {
            qCDebug(QGCStateMachineLog) << "All PARAM_VALUE messages received" << state->stateName();
            emit allParamsReceived();
        }

        emit receivedParamValue();

        return true; // Continue processing further PARAM_VALUE messages
    };

    // FIXME: What about the time on phase2 watch which can have REQUEST_READ retries
    auto watchParamValueState = new WatchForMavlinkMessageState(MAVLINK_MSG_ID_PARAM_VALUE, kWaitForParamValueMs, processParamValue, compositeParentState);
    auto finalState = new QGCFinalState(compositeParentState);

    compositeParentState->setInitialState(watchParamValueState);
    watchParamValueState->addThisTransition(&QGCState::advance, finalState);

    connect(watchParamValueState, &WatchForMavlinkMessageState::timeout, this, [this, watchParamValueState]() {
        qCDebug(QGCStateMachineLog) << "Timeout waiting for all PARAM_VALUE messages" << watchParamValueState->stateName();
        emit watchParamValueState->advance();
    });

    return compositeParentState;
}

QGCState* ParamRequestListStateMachine::_createRequestMissingParamsState(QState* parentState)
{
    auto compositeParentState = new QGCState(QStringLiteral("RequestMissingParamsState"), parentState);

    // Add states and transitions as per the design

    return compositeParentState;
}
