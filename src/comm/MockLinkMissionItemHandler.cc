/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "MockLinkMissionItemHandler.h"
#include "MockLink.h"

#include <QDebug>

QGC_LOGGING_CATEGORY(MockLinkMissionItemHandlerLog, "MockLinkMissionItemHandlerLog")

MockLinkMissionItemHandler::MockLinkMissionItemHandler(MockLink* mockLink, MAVLinkProtocol* mavlinkProtocol)
    : _mockLink(mockLink)
    , _missionItemResponseTimer(NULL)
    , _failureMode(FailNone)
    , _sendHomePositionOnEmptyList(false)
    , _mavlinkProtocol(mavlinkProtocol)
{
    Q_ASSERT(mockLink);
}

MockLinkMissionItemHandler::~MockLinkMissionItemHandler()
{

}

void MockLinkMissionItemHandler::_startMissionItemResponseTimer(void)
{
    if (!_missionItemResponseTimer) {
        _missionItemResponseTimer = new QTimer();
        connect(_missionItemResponseTimer, &QTimer::timeout, this, &MockLinkMissionItemHandler::_missionItemResponseTimeout);
    }
    _missionItemResponseTimer->start(500);
}

bool MockLinkMissionItemHandler::handleMessage(const mavlink_message_t& msg)
{
    switch (msg.msgid) {
        case MAVLINK_MSG_ID_MISSION_REQUEST_LIST:
            _handleMissionRequestList(msg);
            break;
            
        case MAVLINK_MSG_ID_MISSION_REQUEST:
            _handleMissionRequest(msg);
            break;
            
        case MAVLINK_MSG_ID_MISSION_ITEM:
            _handleMissionItem(msg);
            break;
            
        case MAVLINK_MSG_ID_MISSION_COUNT:
            _handleMissionCount(msg);
            break;
            
        case MAVLINK_MSG_ID_MISSION_ACK:
            // Acks are received back for each MISSION_ITEM message
            break;
            
        case MAVLINK_MSG_ID_MISSION_SET_CURRENT:
            // Sets the currently active mission item
            break;
    
        case MAVLINK_MSG_ID_MISSION_CLEAR_ALL:
            // Delete all mission items
            _missionItems.clear();
            break;
            
        default:
            return false;
    }
    
    return true;
}

void MockLinkMissionItemHandler::_handleMissionRequestList(const mavlink_message_t& msg)
{
    qCDebug(MockLinkMissionItemHandlerLog) << "_handleMissionRequestList read sequence";
    
    if (_failureMode != FailReadRequestListNoResponse) {
        mavlink_mission_request_list_t request;
        
        mavlink_msg_mission_request_list_decode(&msg, &request);
        
        Q_ASSERT(request.target_system == _mockLink->vehicleId());

        int itemCount = _missionItems.count();
        if (itemCount == 0 && _sendHomePositionOnEmptyList) {
            itemCount = 1;
        }
        
        mavlink_message_t   responseMsg;
        
        mavlink_msg_mission_count_pack(_mockLink->vehicleId(),
                                       MAV_COMP_ID_MISSIONPLANNER,
                                       &responseMsg,            // Outgoing message
                                       msg.sysid,               // Target is original sender
                                       msg.compid,              // Target is original sender
                                       itemCount);              // Number of mission items
        _mockLink->respondWithMavlinkMessage(responseMsg);
    } else {
        qCDebug(MockLinkMissionItemHandlerLog) << "_handleMissionRequestList not responding due to failure mode";
    }
}

void MockLinkMissionItemHandler::_handleMissionRequest(const mavlink_message_t& msg)
{
    qCDebug(MockLinkMissionItemHandlerLog) << "_handleMissionRequest read sequence";
    
    mavlink_mission_request_t request;
    
    mavlink_msg_mission_request_decode(&msg, &request);
    
    Q_ASSERT(request.target_system == _mockLink->vehicleId());
    Q_ASSERT(request.seq < _missionItems.count());
    
    if ((_failureMode == FailReadRequest0NoResponse && request.seq == 0) ||
        (_failureMode == FailReadRequest1NoResponse && request.seq == 1)) {
        qCDebug(MockLinkMissionItemHandlerLog) << "_handleMissionRequest not responding due to failure mode";
    } else {
        // FIXME: Track whether all items are requested, or requested in sequence
        
        if ((_failureMode == FailReadRequest0IncorrectSequence && request.seq == 0) ||
                (_failureMode == FailReadRequest1IncorrectSequence && request.seq == 1)) {
            // Send back the incorrect sequence number
            qCDebug(MockLinkMissionItemHandlerLog) << "_handleMissionRequest sending bad sequence number";
            request.seq++;
        }
        
        if ((_failureMode == FailReadRequest0ErrorAck && request.seq == 0) ||
            (_failureMode == FailReadRequest1ErrorAck && request.seq == 1)) {
            _sendAck(MAV_MISSION_ERROR);
        } else {
            mavlink_message_t   responseMsg;
            
            mavlink_mission_item_t item;
            if (_missionItems.count() == 0 && _sendHomePositionOnEmptyList) {
                item.frame = MAV_FRAME_GLOBAL_RELATIVE_ALT;
                item.command = MAV_CMD_NAV_WAYPOINT;
                item.current = false;
                item.autocontinue = true;
                item.param1 = item.param2 = item.param3 = item.param4 = item.x = item.y = item.z = 0;
            } else {
                item = _missionItems[request.seq];
            }
            
            mavlink_msg_mission_item_pack(_mockLink->vehicleId(),
                                          MAV_COMP_ID_MISSIONPLANNER,
                                          &responseMsg,            // Outgoing message
                                          msg.sysid,               // Target is original sender
                                          msg.compid,              // Target is original sender
                                          request.seq,             // Index of mission item being sent
                                          item.frame,
                                          item.command,
                                          item.current,
                                          item.autocontinue,
                                          item.param1, item.param2, item.param3, item.param4,
                                          item.x, item.y, item.z);
            _mockLink->respondWithMavlinkMessage(responseMsg);
        }
    }
}

void MockLinkMissionItemHandler::_handleMissionCount(const mavlink_message_t& msg)
{
    mavlink_mission_count_t missionCount;
    
    mavlink_msg_mission_count_decode(&msg, &missionCount);
    Q_ASSERT(missionCount.target_system == _mockLink->vehicleId());
    
    _writeSequenceCount = missionCount.count;
    Q_ASSERT(_writeSequenceCount >= 0);
    
    qCDebug(MockLinkMissionItemHandlerLog) << "_handleMissionCount write sequence _writeSequenceCount:" << _writeSequenceCount;
    
    _missionItems.clear();
    
    if (_writeSequenceCount == 0) {
        _sendAck(MAV_MISSION_ACCEPTED);
    } else {
        _writeSequenceIndex = 0;
        _requestNextMissionItem(_writeSequenceIndex);
    }
}

void MockLinkMissionItemHandler::_requestNextMissionItem(int sequenceNumber)
{
    qCDebug(MockLinkMissionItemHandlerLog) << "_requestNextMissionItem write sequence sequenceNumber:" << sequenceNumber << "_failureMode:" << _failureMode;
    
    if ((_failureMode == FailWriteRequest0NoResponse && sequenceNumber == 0) ||
        (_failureMode == FailWriteRequest1NoResponse && sequenceNumber == 1)) {
        qCDebug(MockLinkMissionItemHandlerLog) << "_requestNextMissionItem not responding due to failure mode";
    } else {
        if (sequenceNumber >= _writeSequenceCount) {
            qCWarning(MockLinkMissionItemHandlerLog) << "_requestNextMissionItem requested seqeuence number > write count sequenceNumber::_writeSequenceCount" << sequenceNumber << _writeSequenceCount;
            return;
        }
        
        if ((_failureMode == FailWriteRequest0IncorrectSequence && sequenceNumber == 0) ||
            (_failureMode == FailWriteRequest1IncorrectSequence && sequenceNumber == 1)) {
            sequenceNumber ++;
        }
        
        if ((_failureMode == FailWriteRequest0ErrorAck && sequenceNumber == 0) ||
            (_failureMode == FailWriteRequest1ErrorAck && sequenceNumber == 1)) {
            qCDebug(MockLinkMissionItemHandlerLog) << "_requestNextMissionItem sending ack error due to failure mode";
            _sendAck(MAV_MISSION_ERROR);
        } else {
            mavlink_message_t           message;
            mavlink_mission_request_t   missionRequest;
            
            missionRequest.target_system =      _mavlinkProtocol->getSystemId();
            missionRequest.target_component =   _mavlinkProtocol->getComponentId();
            missionRequest.seq =                sequenceNumber;
            
            mavlink_msg_mission_request_encode(_mockLink->vehicleId(), MAV_COMP_ID_MISSIONPLANNER, &message, &missionRequest);
            _mockLink->respondWithMavlinkMessage(message);

            // If response with Mission Item doesn't come before timer fires it's an error
            _startMissionItemResponseTimer();
        }
    }
}

void MockLinkMissionItemHandler::_sendAck(MAV_MISSION_RESULT ackType)
{
    qCDebug(MockLinkMissionItemHandlerLog) << "_sendAck write sequence complete ackType:" << ackType;
    
    mavlink_message_t       message;
    mavlink_mission_ack_t   missionAck;
    
    missionAck.target_system =      _mavlinkProtocol->getSystemId();
    missionAck.target_component =   _mavlinkProtocol->getComponentId();
    missionAck.type =               ackType;
    
    mavlink_msg_mission_ack_encode(_mockLink->vehicleId(), MAV_COMP_ID_MISSIONPLANNER, &message, &missionAck);
    _mockLink->respondWithMavlinkMessage(message);
}

void MockLinkMissionItemHandler::_handleMissionItem(const mavlink_message_t& msg)
{
    qCDebug(MockLinkMissionItemHandlerLog) << "_handleMissionItem write sequence";
    
    _missionItemResponseTimer->stop();
    
    mavlink_mission_item_t missionItem;
    
    mavlink_msg_mission_item_decode(&msg, &missionItem);
    
    Q_ASSERT(missionItem.target_system == _mockLink->vehicleId());
    
    _missionItems[missionItem.seq] = missionItem;
    
    _writeSequenceIndex++;
    if (_writeSequenceIndex < _writeSequenceCount) {
        if (_failureMode == FailWriteFinalAckMissingRequests && _writeSequenceIndex == 3) {
            // Send MAV_MISSION_ACCPETED ack too early
            _sendAck(MAV_MISSION_ACCEPTED);
        } else {
            _requestNextMissionItem(_writeSequenceIndex);
        }
    } else {
        if (_failureMode != FailWriteFinalAckNoResponse) {
            MAV_MISSION_RESULT ack = MAV_MISSION_ACCEPTED;
            
            if (_failureMode ==  FailWriteFinalAckErrorAck) {
                ack = MAV_MISSION_ERROR;
            }
            _sendAck(ack);
        }
    }
}

void MockLinkMissionItemHandler::_missionItemResponseTimeout(void)
{
    qWarning() << "Timeout waiting for next MISSION_ITEM";
    Q_ASSERT(false);
}

void MockLinkMissionItemHandler::sendUnexpectedMissionAck(MAV_MISSION_RESULT ackType)
{
    _sendAck(ackType);
}

void MockLinkMissionItemHandler::sendUnexpectedMissionItem(void)
{
    // FIXME: NYI
    Q_ASSERT(false);
}

void MockLinkMissionItemHandler::sendUnexpectedMissionRequest(void)
{
    // FIXME: NYI
    Q_ASSERT(false);
}

void MockLinkMissionItemHandler::setMissionItemFailureMode(FailureMode_t failureMode)
{
    _failureMode = failureMode;
}

void MockLinkMissionItemHandler::shutdown(void)
{
    if (_missionItemResponseTimer) {
        delete _missionItemResponseTimer;
    }
}
