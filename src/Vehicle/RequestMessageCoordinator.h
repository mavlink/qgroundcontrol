#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QPointer>

#include "VehicleTypes.h"

class MavCommandQueue;
class Vehicle;

/// Coordinates MAV_CMD_REQUEST_MESSAGE workflows: per-component queueing, ack/message
/// correlation, duplicate suppression, and timeout handling for the requested message.
///
/// Layers on top of MavCommandQueue for the actual command send + ack callback.
class RequestMessageCoordinator : public QObject, public VehicleTypes
{
    Q_OBJECT

public:
    RequestMessageCoordinator(Vehicle* vehicle, MavCommandQueue* commandQueue);
    ~RequestMessageCoordinator();

    /// Requests the vehicle to send the specified message. Will retry a number of times.
    ///     @param resultHandler Callback for result
    ///     @param resultHandlerData Opaque data passed back to resultHandler
    void requestMessage(RequestMessageResultHandler resultHandler, void* resultHandlerData,
                        int compId, int messageId,
                        float param1 = 0.0f, float param2 = 0.0f, float param3 = 0.0f, float param4 = 0.0f, float param5 = 0.0f);

    /// Called for every inbound mavlink message so the coordinator can correlate arrivals
    /// with outstanding requests and enforce per-request timeouts.
    void handleReceivedMessage(const mavlink_message_t& message);

    /// Clear pending state without firing callbacks (used during vehicle shutdown).
    void stop();

    static QString failureCodeToString(RequestMessageResultHandlerFailureCode_t failureCode);

private:
    typedef struct RequestMessageInfo {
        QPointer<Vehicle>           vehicle;                        // QPointer automatically becomes null when Vehicle is destroyed
        RequestMessageCoordinator*  coordinator         = nullptr;  // Back-pointer so the static ack handler can reach the instance.
        int                         compId              = 0;
        int                         msgId               = 0;
        float                       param1              = 0.0f;
        float                       param2              = 0.0f;
        float                       param3              = 0.0f;
        float                       param4              = 0.0f;
        float                       param5              = 0.0f;
        RequestMessageResultHandler resultHandler       = nullptr;
        void*                       resultHandlerData   = nullptr;
        bool                        commandAckReceived  = false;    // We keep track of the ack/message being received since the order in which this will come in is random
        bool                        messageReceived     = false;    // We only delete the allocated RequestMessageInfo when both the message is received and we get the ack
        QElapsedTimer               messageWaitElapsedTimer;        // Elapsed time since we started waiting message to show up
        mavlink_message_t           message;
    } RequestMessageInfo_t;

    void _removeInfo(int compId, int msgId);
    bool _duplicate(int compId, int msgId) const;
    void _sendNow(RequestMessageInfo_t* info);
    void _sendNextFromQueue(int compId);

    static void _cmdResultHandler(void* resultHandlerData, int compId, const mavlink_command_ack_t& ack, MavCmdResultFailureCode_t failureCode);

    Vehicle*                                                _vehicle     = nullptr;
    MavCommandQueue*                                        _commandQueue = nullptr;

    QMap<int /* compId */, QMap<int /* msgId */, RequestMessageInfo_t*>> _infoMap;   // Active requests awaiting response
    QMap<int /* compId */, QList<RequestMessageInfo_t*>>                  _queueMap; // Per-component queue waiting for active request to finish
};
