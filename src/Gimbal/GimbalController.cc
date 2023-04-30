#include "GimbalController.h"

QGC_LOGGING_CATEGORY(GimbalLog, "GimbalLog")


GimbalController::GimbalController(Vehicle *vehicle)
    : _vehicle(vehicle)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &GimbalController::_mavlinkMessageReceived);
}

void
GimbalController::_mavlinkMessageReceived(const mavlink_message_t& message)
{
    switch (message.msgid) {
        case MAVLINK_MSG_ID_HEARTBEAT:
            _handleHeartbeat(message);
            break;
        case MAVLINK_MSG_ID_GIMBAL_MANAGER_INFORMATION:
            _handleGimbalInformation(message);
            break;
    }
}

void    
GimbalController::_handleHeartbeat(const mavlink_message_t& message)
{
    for (auto& gimbal : _potentialGimbals) {
        if (gimbal.compID == message.compid) {
            // Already in list.
            // TODO: try again after timeout.
            if (gimbal.shouldRetry) {
                _requestGimbalInformation(gimbal);
            }
            return;
        }
    }

    _potentialGimbals.append(message.compid);
    _requestGimbalInformation(_potentialGimbals.back());
}

void
GimbalController::_handleGimbalInformation(const mavlink_message_t& message)
{
    qCDebug(GimbalLog) << "_handleGimbalInformation(" << message.compid << ")";
    // TODO: 
    // - create gimbal instance and flag it valid
    // - request status at regular interval
}

void
GimbalController::_requestGimbalInformation(GimbalItem& item)
{
    qCDebug(GimbalLog) << "_requestGimbalInformation(" << item.compID << ")";
    if(_vehicle) {
        _vehicle->requestMessage(_requestMessageHandler, (void*)&item,
                                 item.compID, MAVLINK_MSG_ID_GIMBAL_MANAGER_INFORMATION);
    }
}

void GimbalController::_requestMessageHandler(void* resultHandlerData, MAV_RESULT commandResult, Vehicle::RequestMessageResultHandlerFailureCode_t failureCode, const mavlink_message_t&)
{
    GimbalItem* item = (GimbalItem*)resultHandlerData;

    qCDebug(GimbalLog) << "_requestMessageHandler, commandResult: " << commandResult << ", failureCode: " << failureCode;

    switch (failureCode) {
        case Vehicle::RequestMessageNoFailure:
            item->shouldRetry = false;
            break;
        case Vehicle::RequestMessageFailureCommandError:
            item->shouldRetry = true;
            break;
        case Vehicle::RequestMessageFailureCommandNotAcked:
            item->shouldRetry = false;
            break;
        case Vehicle::RequestMessageFailureMessageNotReceived:
            item->shouldRetry = true;
            break;
        case Vehicle::RequestMessageFailureDuplicateCommand:
            item->shouldRetry = true;
            break;
    }
}

GimbalController::GimbalItem::GimbalItem(uint8_t compID_) :
    compID(compID_)
{
}
