#include "MessageIntervalManager.h"

#include <memory>

#include <QtCore/QPointer>

#include "MavCommandQueue.h"
#include "MAVLinkLib.h"
#include "QGCLoggingCategory.h"
#include "RequestMessageCoordinator.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(MessageIntervalManagerLog, "Vehicle.MessageIntervalManager")

namespace {

// Per-request heap state — concurrent requests must not share a single msgId slot.
struct RequestIntervalData {
    QPointer<MessageIntervalManager> manager;
    uint8_t                          compId;
    uint16_t                         msgId;
};

struct SetRateData {
    QPointer<MessageIntervalManager> manager;
    uint8_t                          compId;
    uint16_t                         msgId;
};

} // namespace

MessageIntervalManager::MessageIntervalManager(Vehicle* vehicle, MavCommandQueue* commandQueue, RequestMessageCoordinator* reqMsgCoord)
    : QObject(vehicle)
    , _vehicle(vehicle)
    , _commandQueue(commandQueue)
    , _reqMsgCoord(reqMsgCoord)
{
}

void MessageIntervalManager::handleMessageInterval(const mavlink_message_t& message)
{
    mavlink_message_interval_t data;
    mavlink_msg_message_interval_decode(&message, &data);

    const MavCompMsgId compMsgId = {message.compid, data.message_id};
    const int32_t rate = (data.interval_us > 0) ? 1000000.0 / data.interval_us : data.interval_us;

    if (!_mavlinkMsgIntervals.contains(compMsgId) || _mavlinkMsgIntervals.value(compMsgId) != rate) {
        (void) _mavlinkMsgIntervals.insert(compMsgId, rate);
        emit mavlinkMsgIntervalsChanged(message.compid, data.message_id, rate);
    }
}

void MessageIntervalManager::_requestMessageIntervalResultHandler(void* resultHandlerData, MAV_RESULT result, RequestMessageResultHandlerFailureCode_t failureCode, const mavlink_message_t& /*message*/)
{
    // `message` is indeterminate on failure paths — use the tracked msgId instead of decoding.
    std::unique_ptr<RequestIntervalData> ctx(static_cast<RequestIntervalData*>(resultHandlerData));
    if (!ctx || !ctx->manager) {
        return;
    }

    if ((result != MAV_RESULT_ACCEPTED) || (failureCode != RequestMessageNoFailure)) {
        (void) ctx->manager->_unsupportedMessageIds.insert(ctx->compId, ctx->msgId);
    }
}

void MessageIntervalManager::_requestMessageInterval(uint8_t compId, uint16_t msgId)
{
    if (_unsupportedMessageIds.contains(compId, msgId)) {
        return;
    }

    auto* ctx = new RequestIntervalData{this, compId, msgId};
    _reqMsgCoord->requestMessage(
        &MessageIntervalManager::_requestMessageIntervalResultHandler,
        ctx,
        compId,
        MAVLINK_MSG_ID_MESSAGE_INTERVAL,
        msgId);
}

int32_t MessageIntervalManager::getMessageRate(uint8_t compId, uint16_t msgId)
{
    // TODO: Use QGCMavlinkMessage
    const MavCompMsgId compMsgId = {compId, msgId};
    int32_t rate = 0;
    if (_mavlinkMsgIntervals.contains(compMsgId)) {
        rate = _mavlinkMsgIntervals.value(compMsgId);
    } else {
        _requestMessageInterval(compId, msgId);
    }
    return rate;
}

void MessageIntervalManager::_setMessageRateCommandResultHandler(void* resultHandlerData, int /*compId*/, const mavlink_command_ack_t& ack, MavCmdResultFailureCode_t failureCode)
{
    std::unique_ptr<SetRateData> ctx(static_cast<SetRateData*>(resultHandlerData));
    if (!ctx || !ctx->manager) {
        return;
    }

    if ((ack.result == MAV_RESULT_ACCEPTED) && (failureCode == MavCmdResultCommandResultOnly)) {
        ctx->manager->_requestMessageInterval(ctx->compId, ctx->msgId);
    }
}

void MessageIntervalManager::setMessageRate(uint8_t compId, uint16_t msgId, int32_t rate)
{
    auto* ctx = new SetRateData{this, compId, msgId};
    const MavCmdAckHandlerInfo_t handlerInfo = {
        /* .resultHandler        = */ &MessageIntervalManager::_setMessageRateCommandResultHandler,
        /* .resultHandlerData    = */ ctx,
        /* .progressHandler      = */ nullptr,
        /* .progressHandlerData  = */ nullptr,
    };

    const float interval = (rate > 0) ? 1000000.0 / rate : rate;

    _commandQueue->sendCommandWithHandler(
        &handlerInfo,
        compId,
        MAV_CMD_SET_MESSAGE_INTERVAL,
        static_cast<float>(msgId),
        interval);
}

