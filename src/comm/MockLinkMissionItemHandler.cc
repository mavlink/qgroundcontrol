/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

#include "MockLinkMissionItemHandler.h"
#include "MockLink.h"

#include <QDebug>

QGC_LOGGING_CATEGORY(MockLinkMissionItemHandlerLog, "MockLinkMissionItemHandlerLog")

MockLinkMissionItemHandler::MockLinkMissionItemHandler(MockLink* mockLink)
    : QObject(mockLink)
    , _mockLink(mockLink)
{
    Q_ASSERT(mockLink);
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
    
    mavlink_mission_request_list_t request;
    
    mavlink_msg_mission_request_list_decode(&msg, &request);
    
    Q_ASSERT(request.target_system == _mockLink->vehicleId());
    
    mavlink_message_t   responseMsg;
    
    mavlink_msg_mission_count_pack(_mockLink->vehicleId(),
                                   MAV_COMP_ID_MISSIONPLANNER,
                                   &responseMsg,            // Outgoing message
                                   msg.sysid,               // Target is original sender
                                   msg.compid,              // Target is original sender
                                   _missionItems.count());  // Number of mission items
    _mockLink->respondWithMavlinkMessage(responseMsg);
}

void MockLinkMissionItemHandler::_handleMissionRequest(const mavlink_message_t& msg)
{
    qCDebug(MockLinkMissionItemHandlerLog) << "_handleMissionRequest read sequence";
    
    mavlink_mission_request_t request;
    
    mavlink_msg_mission_request_decode(&msg, &request);
    
    Q_ASSERT(request.target_system == _mockLink->vehicleId());
    Q_ASSERT(request.seq < _missionItems.count());
    
    mavlink_message_t   responseMsg;
    
    mavlink_mission_item_t item = _missionItems[request.seq];
    
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

void MockLinkMissionItemHandler::_handleMissionCount(const mavlink_message_t& msg)
{
    qCDebug(MockLinkMissionItemHandlerLog) << "_handleMissionCount write sequence";
    
    mavlink_mission_count_t missionCount;
    
    mavlink_msg_mission_count_decode(&msg, &missionCount);
    Q_ASSERT(missionCount.target_system == _mockLink->vehicleId());
    
    _writeSequenceCount = missionCount.count;
    Q_ASSERT(_writeSequenceCount >= 0);
    
    // FIXME: Set up a timer for a failed write sequence
    
    _missionItems.clear();
    
    if (_writeSequenceCount == 0) {
        // FIXME: NYI
    } else {
        _writeSequenceIndex = 0;
        _requestNextMissionItem(_writeSequenceIndex);
    }
}

void MockLinkMissionItemHandler::_requestNextMissionItem(int sequenceNumber)
{
    qCDebug(MockLinkMissionItemHandlerLog) << "_requestNextMissionItem write sequence sequenceNumber:" << sequenceNumber;
    
    if (sequenceNumber >= _writeSequenceCount) {
        qCWarning(MockLinkMissionItemHandlerLog) << "_requestNextMissionItem requested seqeuence number > write count sequenceNumber::_writeSequenceCount" << sequenceNumber << _writeSequenceCount;
        return;
    }
    
    mavlink_message_t           message;
    mavlink_mission_request_t   missionRequest;
    
    missionRequest.target_system =      MAVLinkProtocol::instance()->getSystemId();
    missionRequest.target_component =   MAVLinkProtocol::instance()->getComponentId();
    missionRequest.seq =                sequenceNumber;
    
    mavlink_msg_mission_request_encode(_mockLink->vehicleId(), MAV_COMP_ID_MISSIONPLANNER, &message, &missionRequest);
    _mockLink->respondWithMavlinkMessage(message);

    // FIXME: Timeouts
}


void MockLinkMissionItemHandler::_handleMissionItem(const mavlink_message_t& msg)
{
    qCDebug(MockLinkMissionItemHandlerLog) << "_handleMissionItem write sequence";
    
    mavlink_mission_item_t missionItem;
    
    mavlink_msg_mission_item_decode(&msg, &missionItem);
    
    Q_ASSERT(missionItem.target_system == _mockLink->vehicleId());
    
    Q_ASSERT(!_missionItems.contains(missionItem.seq));
    Q_ASSERT(missionItem.seq == _writeSequenceIndex);
    
    _missionItems[missionItem.seq] = missionItem;
    
    // FIXME: Timeouts
    
    _writeSequenceIndex++;
    if (_writeSequenceIndex < _writeSequenceCount) {
        _requestNextMissionItem(_writeSequenceIndex);
    } else {
        qCDebug(MockLinkMissionItemHandlerLog) << "_handleMissionItem sending final ack, write sequence complete";
        mavlink_message_t       message;
        mavlink_mission_ack_t   missionAck;
        
        missionAck.target_system =      MAVLinkProtocol::instance()->getSystemId();
        missionAck.target_component =   MAVLinkProtocol::instance()->getComponentId();
        missionAck.type =               MAV_MISSION_ACCEPTED;
        
        mavlink_msg_mission_ack_encode(_mockLink->vehicleId(), MAV_COMP_ID_MISSIONPLANNER, &message, &missionAck);
        _mockLink->respondWithMavlinkMessage(message);
    }
}
