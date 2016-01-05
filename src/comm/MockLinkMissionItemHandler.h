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

#ifndef MockLinkMissionItemHandler_H
#define MockLinkMissionItemHandler_H

#include <QObject>
#include <QMap>
#include <QTimer>

#include "QGCMAVLink.h"
#include "QGCLoggingCategory.h"
#include "MAVLinkProtocol.h"

class MockLink;

Q_DECLARE_LOGGING_CATEGORY(MockLinkMissionItemHandlerLog)

class MockLinkMissionItemHandler : public QObject
{
    Q_OBJECT

public:
    MockLinkMissionItemHandler(MockLink* mockLink, MAVLinkProtocol* mavlinkProtocol);
    ~MockLinkMissionItemHandler();
    
    // Prepares for destruction on correct thread
    void shutdown(void);

    /// @brief Called to handle mission item related messages. All messages should be passed to this method.
    ///         It will handle the appropriate set.
    /// @return true: message handled
    bool handleMessage(const mavlink_message_t& msg);
    
    typedef enum {
        FailNone,                           // No failures
        FailReadRequestListNoResponse,      // Don't send MISSION_COUNT in response to MISSION_REQUEST_LIST
        FailReadRequest0NoResponse,         // Don't send MISSION_ITEM in response to MISSION_REQUEST item 0
        FailReadRequest1NoResponse,         // Don't send MISSION_ITEM in response to MISSION_REQUEST item 1
        FailReadRequest0IncorrectSequence,  // Respond to MISSION_REQUEST 0 with incorrect sequence number in  MISSION_ITEM
        FailReadRequest1IncorrectSequence,  // Respond to MISSION_REQUEST 1 with incorrect sequence number in  MISSION_ITEM
        FailReadRequest0ErrorAck,           // Respond to MISSION_REQUEST 0 with MISSION_ACK error
        FailReadRequest1ErrorAck,           // Respond to MISSION_REQUEST 1 bogus MISSION_ACK error
        FailWriteRequest0NoResponse,        // Don't respond to MISSION_COUNT with MISSION_REQUEST 0
        FailWriteRequest1NoResponse,        // Don't respond to MISSION_ITEM 0 with MISSION_REQUEST 1
        FailWriteRequest0IncorrectSequence, // Respond to MISSION_COUNT 0 with MISSION_REQUEST with wrong sequence number
        FailWriteRequest1IncorrectSequence, // Respond to MISSION_ITEM 0 with MISSION_REQUEST with wrong sequence number
        FailWriteRequest0ErrorAck,          // Respond to MISSION_COUNT 0 with MISSION_ACK error
        FailWriteRequest1ErrorAck,          // Respond to MISSION_ITEM 0 with MISSION_ACK error
        FailWriteFinalAckNoResponse,        // Don't send the final MISSION_ACK
        FailWriteFinalAckErrorAck,          // Send an error as the final MISSION_ACK
        FailWriteFinalAckMissingRequests,   // Send the MISSION_ACK before all items have been requested
    } FailureMode_t;

    /// Sets a failure mode for unit testing
    ///     @param failureMode Type of failure to simulate
    void setMissionItemFailureMode(FailureMode_t failureMode);
    
    /// Called to send a MISSION_ACK message while the MissionManager is in idle state
    void sendUnexpectedMissionAck(MAV_MISSION_RESULT ackType);
    
    /// Called to send a MISSION_ITEM message while the MissionManager is in idle state
    void sendUnexpectedMissionItem(void);
    
    /// Called to send a MISSION_REQUEST message while the MissionManager is in idle state
    void sendUnexpectedMissionRequest(void);
    
    /// Reset the state of the MissionItemHandler to no items, no transactions in progress.
    void reset(void) { _missionItems.clear(); }

    void setSendHomePositionOnEmptyList(bool sendHomePositionOnEmptyList) { _sendHomePositionOnEmptyList = sendHomePositionOnEmptyList; }

private slots:
    void _missionItemResponseTimeout(void);

private:
    void _handleMissionRequestList(const mavlink_message_t& msg);
    void _handleMissionRequest(const mavlink_message_t& msg);
    void _handleMissionItem(const mavlink_message_t& msg);
    void _handleMissionCount(const mavlink_message_t& msg);
    void _requestNextMissionItem(int sequenceNumber);
    void _sendAck(MAV_MISSION_RESULT ackType);
    void _startMissionItemResponseTimer(void);

private:
    MockLink* _mockLink;
    
    int _writeSequenceCount;    ///< Numbers of items about to be written
    int _writeSequenceIndex;    ///< Current index being reqested
    
    typedef QMap<uint16_t, mavlink_mission_item_t>   MissionList_t;
    MissionList_t   _missionItems;
    
    QTimer*             _missionItemResponseTimer;
    FailureMode_t       _failureMode;
    bool                _sendHomePositionOnEmptyList;
    MAVLinkProtocol*    _mavlinkProtocol;
};

#endif
