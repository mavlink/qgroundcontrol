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
    int32_t rate;
    bool unambiguous;  ///< No other SET_MESSAGE_INTERVAL to compId was pending when this was sent
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

    MessageIntervalManager* const manager = ctx->manager;

    if ((result != MAV_RESULT_ACCEPTED) || (failureCode != RequestMessageNoFailure)) {
        (void) manager->_unsupportedMessageIds.insert(ctx->compId, ctx->msgId);
    }

    QQueue<uint16_t>& queue = manager->_intervalRequestQueues[ctx->compId];
    if (!queue.isEmpty()) {
        (void) queue.dequeue();
    }
    manager->_sendQueuedMessageIntervalRequest(ctx->compId);
}

void MessageIntervalManager::_requestMessageInterval(uint8_t compId, uint16_t msgId)
{
    // RequestMessageCoordinator fails a MESSAGE_INTERVAL request while another to the same component
    // is outstanding, whichever message it is for, so send them one at a time
    QQueue<uint16_t>& queue = _intervalRequestQueues[compId];
    if (_unsupportedMessageIds.contains(compId, msgId) || queue.contains(msgId)) {
        return;
    }

    queue.enqueue(msgId);
    if (queue.count() == 1) {
        _sendQueuedMessageIntervalRequest(compId);
    }
}

void MessageIntervalManager::_sendQueuedMessageIntervalRequest(uint8_t compId)
{
    QQueue<uint16_t>& queue = _intervalRequestQueues[compId];
    while (!queue.isEmpty() && _unsupportedMessageIds.contains(compId, queue.head())) {
        (void) queue.dequeue();
    }
    if (queue.isEmpty()) {
        return;
    }

    const uint16_t msgId = queue.head();
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
    MessageIntervalManager* const manager = ctx->manager;

    if (failureCode == MavCmdResultCommandResultOnly) {
        switch (ack.result) {
            case MAV_RESULT_ACCEPTED:
                manager->_requestMessageInterval(ctx->compId, ctx->msgId);
                break;
            case MAV_RESULT_DENIED:  // ArduPilot: no such message
            case MAV_RESULT_UNSUPPORTED:
                // COMMAND_ACK doesn't say which message it is for, and acks are matched to the oldest
                // pending command, so if this command's ack was lost a later one's could be matched to
                // it. So only remember the refusal if no other SET_MESSAGE_INTERVAL to the component
                // was pending, either when this was sent or now.
                if (ctx->unambiguous && !manager->_commandQueue->isPending(ctx->compId, MAV_CMD_SET_MESSAGE_INTERVAL)) {
                    qCDebug(MessageIntervalManagerLog) << "compId" << ctx->compId << "refused msgId" << ctx->msgId
                                                       << "rate" << ctx->rate << "- not requesting it again";
                    (void) manager->_refusedSetRates.insert({ctx->compId, ctx->msgId}, ctx->rate);
                }
                break;
            default:
                break;
        }
    }

    QQueue<MsgIdRate>& queue = manager->_setRateQueues[ctx->compId];
    if (!queue.isEmpty()) {
        (void) queue.dequeue();
    }
    manager->_sendQueuedMessageRate(ctx->compId);
}

bool MessageIntervalManager::messageRateRefused(uint8_t compId, uint16_t msgId, int32_t rate) const
{
    return _refusedSetRates.contains({compId, msgId}, rate);
}

void MessageIntervalManager::setMessageRate(uint8_t compId, uint16_t msgId, int32_t rate)
{
    QQueue<MsgIdRate>& queue = _setRateQueues[compId];
    queue.enqueue({msgId, rate});
    if (queue.count() == 1) {
        _sendQueuedMessageRate(compId);
    }
}

void MessageIntervalManager::_sendQueuedMessageRate(uint8_t compId)
{
    QQueue<MsgIdRate>& queue = _setRateQueues[compId];
    while (!queue.isEmpty() && messageRateRefused(compId, queue.head().first, queue.head().second)) {
        qCDebug(MessageIntervalManagerLog)
            << "skipping SET_MESSAGE_INTERVAL for msgId" << queue.head().first << "refused by compId" << compId;
        (void) queue.dequeue();
    }
    if (queue.isEmpty()) {
        return;
    }

    const auto [msgId, rate] = queue.head();
    const bool unambiguous = !_commandQueue->isPending(compId, MAV_CMD_SET_MESSAGE_INTERVAL);
    auto* ctx = new SetRateData{this, compId, msgId, rate, unambiguous};
    const MavCmdAckHandlerInfo_t handlerInfo = {
        /* .resultHandler        = */ &MessageIntervalManager::_setMessageRateCommandResultHandler,
        /* .resultHandlerData    = */ ctx,
        /* .progressHandler      = */ nullptr,
        /* .progressHandlerData  = */ nullptr,
    };

    const float interval = (rate > 0) ? 1000000.0 / rate : rate;

    // The result handler sends the next queued request, possibly before this returns
    _commandQueue->sendCommandWithHandler(
        &handlerInfo,
        compId,
        MAV_CMD_SET_MESSAGE_INTERVAL,
        static_cast<float>(msgId),
        interval);
}
