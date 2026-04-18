#pragma once

#include <utility>

#include <QtCore/QHash>
#include <QtCore/QMultiHash>
#include <QtCore/QObject>

#include "VehicleTypes.h"

class MavCommandQueue;
class RequestMessageCoordinator;
class Vehicle;

/// Tracks per-component MAVLink message intervals and mediates SET_MESSAGE_INTERVAL
/// commands plus MESSAGE_INTERVAL request/response flows.
///
/// Layers on top of MavCommandQueue (for SET_MESSAGE_INTERVAL) and
/// RequestMessageCoordinator (for MESSAGE_INTERVAL queries).
class MessageIntervalManager : public QObject, public VehicleTypes
{
    Q_OBJECT

public:
    MessageIntervalManager(Vehicle* vehicle, MavCommandQueue* commandQueue, RequestMessageCoordinator* reqMsgCoord);

    /// Returns the last-known rate (Hz) for (compId, msgId). If unknown, issues an
    /// asynchronous MESSAGE_INTERVAL request and returns 0.
    int32_t getMessageRate(uint8_t compId, uint16_t msgId);

    /// Sends MAV_CMD_SET_MESSAGE_INTERVAL to update the rate for the given message id,
    /// then requests MESSAGE_INTERVAL on ack to confirm the new rate.
    void setMessageRate(uint8_t compId, uint16_t msgId, int32_t rate);

    /// Decode a MESSAGE_INTERVAL message and update the cache, emitting a change
    /// signal if the rate changed.
    void handleMessageInterval(const mavlink_message_t& message);

signals:
    void mavlinkMsgIntervalsChanged(uint8_t compid, uint16_t msgId, int32_t rate);

private:
    using MavCompMsgId = std::pair<uint8_t /* compId */, uint16_t /* msgId */>;

    static void _requestMessageIntervalResultHandler(void* resultHandlerData, MAV_RESULT result, RequestMessageResultHandlerFailureCode_t failureCode, const mavlink_message_t& message);
    static void _setMessageRateCommandResultHandler(void* resultHandlerData, int compId, const mavlink_command_ack_t& ack, MavCmdResultFailureCode_t failureCode);

    void _requestMessageInterval(uint8_t compId, uint16_t msgId);

    Vehicle*                    _vehicle      = nullptr;
    MavCommandQueue*            _commandQueue = nullptr;
    RequestMessageCoordinator*  _reqMsgCoord  = nullptr;

    QHash<MavCompMsgId, int32_t>   _mavlinkMsgIntervals;
    QMultiHash<uint8_t, uint16_t>  _unsupportedMessageIds;
};
