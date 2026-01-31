#include "RequestMessageState.h"
#include "QGCLoggingCategory.h"
#include "VehicleLinkManager.h"

RequestMessageState::RequestMessageState(QState* parent,
                                         uint32_t messageId,
                                         MessageHandler messageHandler,
                                         int compId,
                                         int timeoutMsecs,
                                         float param1,
                                         float param2,
                                         float param3,
                                         float param4,
                                         float param5)
    : WaitStateBase(QStringLiteral("RequestMessageState:%1").arg(messageId), parent, timeoutMsecs)
    , _messageId(messageId)
    , _messageHandler(std::move(messageHandler))
    , _compId(compId)
    , _param1(param1)
    , _param2(param2)
    , _param3(param3)
    , _param4(param4)
    , _param5(param5)
{
}

void RequestMessageState::onWaitEntered()
{
    WaitStateBase::onWaitEntered();

    SharedLinkInterfacePtr sharedLink = vehicle()->vehicleLinkManager()->primaryLink().lock();

    if (!sharedLink) {
        qCDebug(QGCStateMachineLog) << "Skipping request due to no primary link" << stateName();
        _failureCode = Vehicle::RequestMessageFailureCommandNotAcked;
        waitFailed();
        return;
    }

    if (sharedLink->linkConfiguration()->isHighLatency() || sharedLink->isLogReplay()) {
        qCDebug(QGCStateMachineLog) << "Skipping request due to link type" << stateName();
        // Not an error - just skip on high latency/replay links
        waitComplete();
        return;
    }

    qCDebug(QGCStateMachineLog) << "Requesting message id" << _messageId << stateName();
    vehicle()->requestMessage(_resultHandler, this, _compId, _messageId, _param1, _param2, _param3, _param4, _param5);
}

void RequestMessageState::onWaitTimeout()
{
    qCDebug(QGCStateMachineLog) << "Timeout waiting for message" << _messageId << stateName();
    _failureCode = Vehicle::RequestMessageFailureMessageNotReceived;
    WaitStateBase::onWaitTimeout();
}

void RequestMessageState::_resultHandler(void* resultHandlerData,
                                         MAV_RESULT commandResult,
                                         Vehicle::RequestMessageResultHandlerFailureCode_t failureCode,
                                         const mavlink_message_t& message)
{
    auto* state = static_cast<RequestMessageState*>(resultHandlerData);
    state->_failureCode = failureCode;
    state->_commandResult = commandResult;

    if (failureCode == Vehicle::RequestMessageNoFailure) {
        qCDebug(QGCStateMachineLog) << "Message received successfully" << state->stateName();

        if (state->_messageHandler) {
            state->_messageHandler(state->vehicle(), message);
        }

        emit state->messageReceived(message);
        state->waitComplete();
    } else {
        switch (failureCode) {
        case Vehicle::RequestMessageFailureCommandError:
            qCDebug(QGCStateMachineLog) << "Command error" << commandResult << state->stateName();
            break;
        case Vehicle::RequestMessageFailureCommandNotAcked:
            qCDebug(QGCStateMachineLog) << "Command not acked" << state->stateName();
            break;
        case Vehicle::RequestMessageFailureMessageNotReceived:
            qCDebug(QGCStateMachineLog) << "Message not received" << state->stateName();
            break;
        case Vehicle::RequestMessageFailureDuplicateCommand:
            qCDebug(QGCStateMachineLog) << "Duplicate command" << state->stateName();
            break;
        default:
            qCDebug(QGCStateMachineLog) << "Unknown failure" << failureCode << state->stateName();
            break;
        }
        state->waitFailed();
    }
}
