/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ParamRequestListPhase1StateMachine.h"
#include "ParamHashCheckStateMachine.h"
#include "WatchForMavlinkMessageState.h"

ParamRequestListPhase1StateMachine::ParamRequestListPhase1StateMachine(Vehicle* vehicle, uint8_t componentId, QObject* parent)
    : QGCStateMachine(QStringLiteral("ParamRequestListPhase1"), vehicle, parent)
    , _componentId(componentId)
{
    _setupStateGraph();
}

void ParamRequestListPhase1StateMachine::_setStateGraph()
{
    auto paramRequestListEncoder = [this](uint8_t systemId, uint8_t componentId, mavlink_message_t* message) {
        mavlink_msg_param_request_list_pack(systemId,
                                           componentId,
                                           message,
                                           vehicle()->id(),
                                           _componentId);
    };

    auto sendParamRequestListState = new SendMavlinkMessageState(this, paramRequestListEncoder, kMaxParamRequestListRetries);
    auto parallelState = _createParallelState();
    auto finalState = new QGCFinalState(this);

    setInitialState(sendParamRequestListState);

    sendParamRequestListState->addTransition(sendParamRequestListState, &SendMavlinkMessageState::advance, parallelState);
    parallelState->addTransition(parallelState, &QGCStateMachine::finished, finalState);
}

QGCState* ParamRequestListPhase1StateMachine::_createParallelState()
{
    // Create parallel state
    auto parallelState = new QGCState(QStringLiteral("Parallel container"), this);
    parallelState->setChildMode(QState::ParallelStates);

    auto watchForAllParamsState = _createWatchForAllParamsState(parallelState);
    auto hashCheckStateMachine = new ParamHashCheckStateMachine(vehicle(), parallelState);

    parallelState->addTransition(watchForAllParamsState, &WatchForMavlinkMessageState::timeout, parallelState);

    return parallelState;
}

QGCState* ParamRequestListPhase1StateMachine::_createWatchForAllParamsState(QState* parentState)
{
    auto processParamValue = [this](const mavlink_message_t &message) -> bool {
        mavlink_param_value_t param_value{};
        mavlink_msg_param_value_decode(&message, &param_value);

        int componentId = param_value.target_component;
        int paramIndex = static_cast<int>(param_value.param_index);

        // If we haven't seen this component id before, set up its param count and waiting list
        if (!_paramCountMap.contains(componentId)) {
            _paramCountMap[componentId] = static_cast<int>(param_value.param_count);
            QList<int> waitingList;
            for (int i = 0; i < param_value.param_count; i++) {
                waitingList.append(i);
            }
            _waitingReadParamIndexMap[componentId] = waitingList;
        }

        // Remove this param index from the waiting list
        if (_waitingReadParamIndexMap.contains(componentId)) {
            _waitingReadParamIndexMap[componentId].removeAll(paramIndex);
            if (_waitingReadParamIndexMap[componentId].isEmpty()) {
                _waitingReadParamIndexMap.remove(componentId);
            }
        }

        // Check if all parameters have been received
        if (_waitingReadParamIndexMap.isEmpty()) {
            qCDebug(QGCStateMachineLog) << "All PARAM_VALUE messages received" << stateName();
            return false;   // Stop further processing
        }

        return true;    // Continue processing further PARAM_VALUE messages
    };

    auto watchParamValueState = new WatchForMavlinkMessageState(parentState, MAVLINK_MSG_ID_PARAM_VALUE, kWaitForParamValueMs, processParamValue);
    connect(watchParamValueState, &WatchForMavlinkMessageState::timeout, this, [this, watchParamValueState]() {
        qCDebug(QGCStateMachineLog) << "Timeout waiting for all PARAM_VALUE messages" << stateName();
        emit watchParamValueState->advance();
    });

    return watchParamValueState;
}
