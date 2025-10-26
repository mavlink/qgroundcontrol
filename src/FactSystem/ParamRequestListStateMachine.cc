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
#include "WatchForMavlinkMessageState.h"
#include "Vehicle.h"

ParamRequestListStateMachine::ParamRequestListStateMachine(Vehicle* vehicle, uint8_t componentID)
    : QGCStateMachine(QStringLiteral("ParamRequestList"), vehicle, nullptr /* auto delete */)
    , _componentID(componentID)
{
    _setupStateGraph();
}

void ParamRequestListStateMachine::_setupStateGraph()
{
    _paramCountMap.clear();
    _missingParamIndicesMap.clear();

    auto paramRequestListEncoder = [this](SendMavlinkMessageState */*state*/, uint8_t systemId, uint8_t componentId, mavlink_message_t* message) {
        mavlink_msg_param_request_list_pack(systemId,
                                           componentId,
                                           message,
                                           vehicle()->id(),
                                           _componentID);
    };

    auto sendParamRequestListState = new SendMavlinkMessageState(paramRequestListEncoder, kMaxParamRequestListRetries, this);
    auto parallelState = _createParallelState();
    auto finalState = new QGCFinalState(this);

    setInitialState(sendParamRequestListState);

    sendParamRequestListState->addThisTransition(&SendMavlinkMessageState::advance, parallelState);
    parallelState->addThisTransition            (&QState::finished,                 finalState);
}

QGCState* ParamRequestListStateMachine::_createParallelState()
{
    auto parallelState = new QGCState(QStringLiteral("Parallel container"), this);
    parallelState->setChildMode(QState::ParallelStates);

    (void) _createWatchForAllParamsState(parallelState);
    (void) new ParamHashCheckStateMachine(vehicle(), parallelState);

    return parallelState;
}

QGCState* ParamRequestListStateMachine::_createWatchForAllParamsState(QState* parentState)
{
    auto compositeState = new QGCState(QStringLiteral("WatchForAllParamsState"), parentState);

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
            return false;   // Stop further processing
        }

        return true;    // Continue processing further PARAM_VALUE messages
    };

    auto watchParamValueState = new WatchForMavlinkMessageState(MAVLINK_MSG_ID_PARAM_VALUE, kWaitForParamValueMs, processParamValue, compositeState);
    auto finalState = new QGCFinalState(compositeState);

    compositeState->setInitialState(watchParamValueState);
    watchParamValueState->addThisTransition(&QGCState::advance, finalState);

    connect(watchParamValueState, &WatchForMavlinkMessageState::timeout, this, [this, watchParamValueState]() {
        qCDebug(QGCStateMachineLog) << "Timeout waiting for all PARAM_VALUE messages" << watchParamValueState->stateName();
        emit parametersMissing(_missingParamIndicesMap);
        emit watchParamValueState->advance();
    });

    return compositeState;
}