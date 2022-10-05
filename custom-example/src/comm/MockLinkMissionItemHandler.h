/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

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
        FailReadRequestListFirstResponse,   // Don't send MISSION_COUNT in response to first MISSION_REQUEST_LIST, allow subsequent to go through
        FailReadRequest0NoResponse,         // Don't send MISSION_ITEM in response to MISSION_REQUEST item 0
        FailReadRequest1NoResponse,         // Don't send MISSION_ITEM in response to MISSION_REQUEST item 1
        FailReadRequest1FirstResponse,      // Don't send MISSION_ITEM in response to MISSION_REQUEST item 1 on first try, allow subsequent request to go through
        FailReadRequest0IncorrectSequence,  // Respond to MISSION_REQUEST 0 with incorrect sequence number in  MISSION_ITEM
        FailReadRequest1IncorrectSequence,  // Respond to MISSION_REQUEST 1 with incorrect sequence number in  MISSION_ITEM
        FailReadRequest0ErrorAck,           // Respond to MISSION_REQUEST 0 with MISSION_ACK error
        FailReadRequest1ErrorAck,           // Respond to MISSION_REQUEST 1 bogus MISSION_ACK error
        FailWriteMissionCountNoResponse,    // Don't respond to MISSION_COUNT with MISSION_REQUEST 0
        FailWriteMissionCountFirstResponse, // Don't respond to first MISSION_COUNT with MISSION_REQUEST 0, respond to subsequent MISSION_COUNT requests
        FailWriteRequest1NoResponse,        // Don't respond to MISSION_ITEM 0 with MISSION_REQUEST 1
        FailWriteRequest0IncorrectSequence, // Item 0 MISSION_REQUEST sent has wrong sequence number
        FailWriteRequest1IncorrectSequence, // Item 1 MISSION_REQUEST sent has wrong sequence number
        FailWriteRequest0ErrorAck,          // Instead of sending MISSION_REQUEST 0, send MISSION_ACK error
        FailWriteRequest1ErrorAck,          // Instead of sending MISSION_REQUEST 1, send MISSION_ACK error
        FailWriteFinalAckNoResponse,        // Don't send the final MISSION_ACK
        FailWriteFinalAckErrorAck,          // Send an error as the final MISSION_ACK
        FailWriteFinalAckMissingRequests,   // Send the MISSION_ACK before all items have been requested
    } FailureMode_t;

    /// Sets a failure mode for unit testing
    ///     @param failureMode Type of failure to simulate
    ///     @param failureAckResult Error to send if one the ack error modes
    void setFailureMode(FailureMode_t failureMode, MAV_MISSION_RESULT failureAckResult);
    
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
    void _handleMissionRequestList      (const mavlink_message_t& msg);
    void _handleMissionRequest          (const mavlink_message_t& msg);
    void _handleMissionItem             (const mavlink_message_t& msg);
    void _handleMissionCount            (const mavlink_message_t& msg);
    void _handleMissionClearAll         (const mavlink_message_t& msg);
    void _requestNextMissionItem        (int sequenceNumber);
    void _sendAck                       (MAV_MISSION_RESULT ackType);
    void _startMissionItemResponseTimer (void);

private:
    MockLink* _mockLink;
    
    int _writeSequenceCount;    ///< Numbers of items about to be written
    int _writeSequenceIndex;    ///< Current index being reqested

    typedef QMap<uint16_t, mavlink_mission_item_int_t> MissionItemList_t;

    MAV_MISSION_TYPE    _requestType;
    MissionItemList_t   _missionItems;
    MissionItemList_t   _fenceItems;
    MissionItemList_t   _rallyItems;

    QTimer*             _missionItemResponseTimer;
    FailureMode_t       _failureMode;
    MAV_MISSION_RESULT  _failureAckResult;
    bool                _sendHomePositionOnEmptyList;
    MAVLinkProtocol*    _mavlinkProtocol;
    bool                _failReadRequestListFirstResponse;
    bool                _failReadRequest1FirstResponse;
    bool                _failWriteMissionCountFirstResponse;
};

