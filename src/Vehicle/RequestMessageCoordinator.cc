#include "RequestMessageCoordinator.h"

#include "MAVLinkLib.h"
#include "MavCommandQueue.h"
#include "AppMessages.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(RequestMessageCoordinatorLog, "Vehicle.RequestMessageCoordinator")

RequestMessageCoordinator::RequestMessageCoordinator(Vehicle* vehicle, MavCommandQueue* commandQueue)
    : QObject(vehicle)
    , _vehicle(vehicle)
    , _commandQueue(commandQueue)
{
}

RequestMessageCoordinator::~RequestMessageCoordinator()
{
    stop();
}

void RequestMessageCoordinator::stop()
{
    for (auto& compIdEntry : _infoMap) {
        qDeleteAll(compIdEntry);
    }
    _infoMap.clear();

    for (auto& requestQueue : _queueMap) {
        qDeleteAll(requestQueue);
    }
    _queueMap.clear();
}

bool RequestMessageCoordinator::_duplicate(int compId, int msgId) const
{
    const mavlink_message_info_t* info = mavlink_get_message_info_by_id(msgId);
    const QString msgName = info ? QString(info->name) : QString::number(msgId);

    if (_infoMap.contains(compId) && _infoMap[compId].contains(msgId)) {
        qCDebug(RequestMessageCoordinatorLog) << "duplicate in active map - compId:msgId" << compId << msgName;
        return true;
    }

    if (_queueMap.contains(compId)) {
        for (const RequestMessageInfo_t* requestMessageInfo : _queueMap[compId]) {
            if (requestMessageInfo->msgId == msgId) {
                qCDebug(RequestMessageCoordinatorLog) << "duplicate in queue - compId:msgId" << compId << msgName;
                return true;
            }
        }
    }

    return false;
}

void RequestMessageCoordinator::_removeInfo(int compId, int msgId)
{
    if (_infoMap.contains(compId) && _infoMap[compId].contains(msgId)) {
        const mavlink_message_info_t* info = mavlink_get_message_info_by_id(msgId);
        const QString msgName = info ? QString(info->name) : QString::number(msgId);

        delete _infoMap[compId][msgId];
        _infoMap[compId].remove(msgId);
        qCDebug(RequestMessageCoordinatorLog)
                << "removed active request compId:msgId"
                << compId
                << msgName
                << "remainingActive"
                << _infoMap[compId].count();
        if (_infoMap[compId].isEmpty()) {
            _infoMap.remove(compId);
        }
        _sendNextFromQueue(compId);
    } else {
        qWarning() << "compId:msgId not found" << compId << msgId;
    }
}

void RequestMessageCoordinator::_sendNow(RequestMessageInfo_t* info)
{
    const mavlink_message_info_t* msgInfo = mavlink_get_message_info_by_id(info->msgId);
    const QString msgName = msgInfo ? QString(msgInfo->name) : QString::number(info->msgId);

    _infoMap[info->compId][info->msgId] = info;

    const int queueDepth = _queueMap.contains(info->compId) ? _queueMap[info->compId].count() : 0;
    qCDebug(RequestMessageCoordinatorLog)
            << "sending now compId:msgId"
            << info->compId
            << msgName
            << "queueDepth"
            << queueDepth;

    MavCmdAckHandlerInfo_t handlerInfo {};
    handlerInfo.resultHandler     = _cmdResultHandler;
    handlerInfo.resultHandlerData = info;

    _commandQueue->sendWorker(false,                                    // commandInt
                              false,                                    // showError
                              &handlerInfo,
                              info->compId,
                              MAV_CMD_REQUEST_MESSAGE,
                              MAV_FRAME_GLOBAL,
                              info->msgId,
                              info->param1,
                              info->param2,
                              info->param3,
                              info->param4,
                              info->param5,
                              0);
}

void RequestMessageCoordinator::_sendNextFromQueue(int compId)
{
    if (_infoMap.contains(compId) && !_infoMap[compId].isEmpty()) {
        qCDebug(RequestMessageCoordinatorLog)
                << "active request still in progress for compId"
                << compId
                << "activeCount"
                << _infoMap[compId].count();
        return;
    }

    if (!_queueMap.contains(compId) || _queueMap[compId].isEmpty()) {
        qCDebug(RequestMessageCoordinatorLog) << "no queued request for compId" << compId;
        _queueMap.remove(compId);
        return;
    }

    RequestMessageInfo_t* info = _queueMap[compId].takeFirst();
    const mavlink_message_info_t* msgInfo = mavlink_get_message_info_by_id(info->msgId);
    const QString msgName = msgInfo ? QString(msgInfo->name) : QString::number(info->msgId);
    const int remainingQueue = _queueMap[compId].count();
    qCDebug(RequestMessageCoordinatorLog)
            << "dequeued next request compId:msgId"
            << compId
            << msgName
            << "remainingQueue"
            << remainingQueue;

    if (_queueMap[compId].isEmpty()) {
        _queueMap.remove(compId);
    }

    _sendNow(info);
}

void RequestMessageCoordinator::handleReceivedMessage(const mavlink_message_t& message)
{
    if (_infoMap.contains(message.compid) && _infoMap[message.compid].contains(message.msgid)) {
        auto pInfo              = _infoMap[message.compid][message.msgid];
        auto resultHandler      = pInfo->resultHandler;
        auto resultHandlerData  = pInfo->resultHandlerData;

        pInfo->messageReceived = true;
        pInfo->message = message;

        const mavlink_message_info_t* info = mavlink_get_message_info_by_id(message.msgid);
        QString msgName = info ? QString(info->name) : QString::number(message.msgid);
        const int activeCount = _infoMap.contains(message.compid) ? _infoMap[message.compid].count() : 0;
        const int queueDepth  = _queueMap.contains(message.compid) ? _queueMap[message.compid].count() : 0;
        qCDebug(RequestMessageCoordinatorLog)
                << "message received - compId:msgId"
                << message.compid
                << msgName
                << "activeCount"
                << activeCount
                << "queueDepth"
                << queueDepth;

        if (pInfo->commandAckReceived) {
            _removeInfo(message.compid, message.msgid);
            (*resultHandler)(resultHandlerData, MAV_RESULT_ACCEPTED, RequestMessageNoFailure, message);
        }
        return;
    }

    // Every inbound message doubles as a timeout tick. _removeInfo mutates _infoMap,
    // so snapshot the first timed-out entry and handle it after the loops exit.
    int                         timedOutCompId        = -1;
    int                         timedOutMsgId         = -1;
    RequestMessageResultHandler timedOutHandler       = nullptr;
    void*                       timedOutHandlerData   = nullptr;
    for (auto& compIdEntry : _infoMap) {
        for (auto info : compIdEntry) {
            // Unit-test environments can have enough scheduling jitter that a 50ms
            // response deadline causes false request-message timeouts.
            const int messageWaitTimeoutMs = QGC::runningUnitTests() ? 500 : 1000;
            if (info->messageWaitElapsedTimer.isValid() && info->messageWaitElapsedTimer.elapsed() > messageWaitTimeoutMs) {
                const mavlink_message_info_t* msgInfo = mavlink_get_message_info_by_id(info->msgId);
                QString msgName = msgInfo ? msgInfo->name : QString::number(info->msgId);
                const int queueDepth = _queueMap.contains(info->compId) ? _queueMap[info->compId].count() : 0;
                qCDebug(RequestMessageCoordinatorLog)
                        << "request message timed out - compId:msgId"
                        << info->compId
                        << msgName
                        << "queueDepth"
                        << queueDepth;

                timedOutCompId      = info->compId;
                timedOutMsgId       = info->msgId;
                timedOutHandler     = info->resultHandler;
                timedOutHandlerData = info->resultHandlerData;
                break;
            }
        }
        if (timedOutHandler) {
            break;
        }
    }

    if (timedOutHandler) {
        _removeInfo(timedOutCompId, timedOutMsgId);
        mavlink_message_t timeoutMessage = {};
        (*timedOutHandler)(timedOutHandlerData, MAV_RESULT_FAILED, RequestMessageFailureMessageNotReceived, timeoutMessage);
    }
}

void RequestMessageCoordinator::_cmdResultHandler(void* resultHandlerData_, [[maybe_unused]] int compId, const mavlink_command_ack_t& ack, MavCmdResultFailureCode_t failureCode)
{
    auto info               = static_cast<RequestMessageInfo_t*>(resultHandlerData_);
    auto resultHandler      = info->resultHandler;
    auto resultHandlerData  = info->resultHandlerData;
    Vehicle* vehicle        = info->vehicle;  // QPointer converts to raw pointer, null if Vehicle destroyed

    // Vehicle was destroyed before callback fired - clean up and return without accessing vehicle
    if (!vehicle) {
        qCDebug(RequestMessageCoordinatorLog) << "Vehicle destroyed before callback - skipping";
        delete info;
        return;
    }

    auto* coordinator = info->coordinator;

    info->commandAckReceived = true;
    const mavlink_message_info_t* msgInfo = mavlink_get_message_info_by_id(info->msgId);
    const QString msgName = msgInfo ? QString(msgInfo->name) : QString::number(info->msgId);
    qCDebug(RequestMessageCoordinatorLog)
            << "ack for requestMessage compId:msgId"
            << info->compId
            << msgName
            << "ack"
            << QGCMAVLink::mavResultToString(static_cast<MAV_RESULT>(ack.result))
            << "failureCode"
            << MavCommandQueue::failureCodeToString(failureCode);

    if (ack.result != MAV_RESULT_ACCEPTED) {
        mavlink_message_t                        ackMessage = {};
        RequestMessageResultHandlerFailureCode_t requestMessageFailureCode = RequestMessageNoFailure;

        switch (failureCode) {
        case MavCmdResultCommandResultOnly:
            requestMessageFailureCode = RequestMessageFailureCommandError;
            break;
        case MavCmdResultFailureNoResponseToCommand:
            requestMessageFailureCode = RequestMessageFailureCommandNotAcked;
            break;
        case MavCmdResultFailureDuplicateCommand:
            requestMessageFailureCode = RequestMessageFailureDuplicate;
            break;
        }

        coordinator->_removeInfo(info->compId, info->msgId);
        (*resultHandler)(resultHandlerData, static_cast<MAV_RESULT>(ack.result), requestMessageFailureCode, ackMessage);
        return;
    }

    if (info->messageReceived) {
        auto message = info->message;
        coordinator->_removeInfo(info->compId, info->msgId);
        (*resultHandler)(resultHandlerData, MAV_RESULT_ACCEPTED, RequestMessageNoFailure, message);
        return;
    }

    // We have the ack, but we are still waiting for the message. Start the timer to wait for the message
    info->messageWaitElapsedTimer.start();
}

void RequestMessageCoordinator::requestMessage(RequestMessageResultHandler resultHandler, void* resultHandlerData, int compId, int messageId, float param1, float param2, float param3, float param4, float param5)
{
    const mavlink_message_info_t* msgInfo = mavlink_get_message_info_by_id(messageId);
    const QString msgName = msgInfo ? QString(msgInfo->name) : QString::number(messageId);
    const int activeCount = _infoMap.contains(compId) ? _infoMap[compId].count() : 0;
    const int queueDepth  = _queueMap.contains(compId) ? _queueMap[compId].count() : 0;
    qCDebug(RequestMessageCoordinatorLog)
            << "incoming request compId:msgId"
            << compId
            << msgName
            << "activeCount"
            << activeCount
            << "queueDepth"
            << queueDepth;

    auto info               = new RequestMessageInfo_t;
    info->vehicle           = _vehicle;
    info->coordinator       = this;
    info->compId            = compId;
    info->msgId             = messageId;
    info->param1            = param1;
    info->param2            = param2;
    info->param3            = param3;
    info->param4            = param4;
    info->param5            = param5;
    info->resultHandler     = resultHandler;
    info->resultHandlerData = resultHandlerData;

    if (_duplicate(compId, messageId)) {
        mavlink_message_t ackMessage = {};
        qCWarning(RequestMessageCoordinatorLog) << "failing exact duplicate compId:msgId" << compId << msgName;
        (*resultHandler)(resultHandlerData,
                         MAV_RESULT_FAILED,
                         RequestMessageFailureDuplicate,
                         ackMessage);
        delete info;
        return;
    }

    if (_infoMap.contains(compId) && !_infoMap[compId].isEmpty()) {
        _queueMap[compId].append(info);
        qCDebug(RequestMessageCoordinatorLog)
                << "queued request compId:msgId"
                << compId
                << msgName
                << "newQueueDepth"
                << _queueMap[compId].count();
        return;
    }

    _sendNow(info);
}

QString RequestMessageCoordinator::failureCodeToString(RequestMessageResultHandlerFailureCode_t failureCode)
{
    switch (failureCode) {
    case RequestMessageNoFailure:
        return QStringLiteral("No Failure");
    case RequestMessageFailureCommandError:
        return QStringLiteral("Command Error");
    case RequestMessageFailureCommandNotAcked:
        return QStringLiteral("Command Not Acked");
    case RequestMessageFailureMessageNotReceived:
        return QStringLiteral("Message Not Received");
    case RequestMessageFailureDuplicate:
        return QStringLiteral("Duplicate Request");
    default:
        return QStringLiteral("Unknown (%1)").arg(failureCode);
    }
}
