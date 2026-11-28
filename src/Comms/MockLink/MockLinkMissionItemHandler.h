#pragma once

#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QTimer>

#include "MAVLinkLib.h"

class MockLink;

class MockLinkMissionItemHandler : public QObject
{
    Q_OBJECT

public:
    MockLinkMissionItemHandler(MockLink *mockLink);
    ~MockLinkMissionItemHandler();

    // Prepares for destruction on correct thread
    void shutdown();

    /// Called to handle mission item related messages. All messages should be passed to this method.
    /// It will handle the appropriate set.
    ///     @return true: message handled
    bool handleMavlinkMessage(const mavlink_message_t &msg);

    enum FailureMode_t {
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
        FailWriteFinalAckFirstResponse,     // Don't send the final MISSION_ACK the first time, send it if the last item is sent again
        FailWriteFinalAckErrorOnResend,     // Don't send the final MISSION_ACK, answer the last item being sent again with an error ack (ArduPilot)
        FailWriteFinalAckErrorAck,          // Send an error as the final MISSION_ACK
        FailWriteFinalAckMissingRequests,   // Send the MISSION_ACK before all items have been requested
    };

    /// Sets a failure mode for unit testing
    ///     @param failureMode Type of failure to simulate
    ///     @param failureAckResult Error to send if one the ack error modes
    void setFailureMode(FailureMode_t failureMode, MAV_MISSION_RESULT failureAckResult);

    /// Called to send a MISSION_ACK message while the MissionManager is in idle state
    void sendUnexpectedMissionAck(MAV_MISSION_RESULT ackType);

    /// Called to send a MISSION_ITEM message while the MissionManager is in idle state
    void sendUnexpectedMissionItem();

    /// Called to send a MISSION_REQUEST message while the MissionManager is in idle state
    void sendUnexpectedMissionRequest();

    /// Reset the state of the MissionItemHandler to no items, no transactions in progress.
    void reset()
    {
        _missionItems.clear();
        _requestListCounts.clear();
        _planChanged(MAV_MISSION_TYPE_MISSION);
    }

    /// Test-only: seeds a simple multirotor mission (takeoff, waypoint, RTL) so that a
    /// connecting GCS will download a non-empty mission.
    void loadSimpleMultirotorMission();

    void setSendHomePositionOnEmptyList(bool sendHomePositionOnEmptyList) { _sendHomePositionOnEmptyList = sendHomePositionOnEmptyList; }

    int requestListCount(MAV_MISSION_TYPE type) const { return _requestListCounts.value(type, 0); }
    void clearRequestListCounts() { _requestListCounts.clear(); }

    /// Test-only: change the plan on the vehicle as another GCS would
    void appendPlanItem(MAV_MISSION_TYPE type);
    void clearPlan(MAV_MISSION_TYPE type);

    /// Sends MISSION_CURRENT with the plan ids, from this handler's thread so it stays in order with MISSION_ACK
    void sendMissionCurrent() const;

private slots:
    void _missionItemResponseTimeout();

private:
    typedef QMap<uint16_t, mavlink_mission_item_int_t> MissionItemList_t;

    void _handleMissionRequestList(const mavlink_message_t &msg);
    void _handleMissionRequest(const mavlink_message_t &msg);
    void _handleMissionItem(const mavlink_message_t &msg);
    void _handleMissionCount(const mavlink_message_t &msg);
    void _handleMissionClearAll(const mavlink_message_t &msg);
    void _requestNextMissionItem(int sequenceNumber);
    void _sendAck(MAV_MISSION_RESULT ackType) const;
    void _startMissionItemResponseTimer();
    MissionItemList_t& _planItems(MAV_MISSION_TYPE type);
    void _planChanged(MAV_MISSION_TYPE type);
    uint32_t _planId(MAV_MISSION_TYPE type) const;

    MockLink *_mockLink = nullptr;

    int _writeSequenceCount = 0;    ///< Numbers of items about to be written
    int _writeSequenceIndex = 0;    ///< Current index being reqested

    MAV_MISSION_TYPE _requestType = MAV_MISSION_TYPE_MISSION;
    MissionItemList_t _missionItems;
    MissionItemList_t _fenceItems;
    MissionItemList_t _rallyItems;

    QTimer _missionItemResponseTimer;
    FailureMode_t _failureMode = FailNone;
    MAV_MISSION_RESULT _failureAckResult;
    bool _sendHomePositionOnEmptyList = false;
    bool _failReadRequestListFirstResponse = true;
    bool _failReadRequest1FirstResponse = true;
    bool _failWriteMissionCountFirstResponse = true;
    bool _failWriteFinalAckFirstResponse = true;
    QMap<MAV_MISSION_TYPE, int> _requestListCounts;

    // Opaque plan ids, 0 for an empty plan
    QMap<MAV_MISSION_TYPE, uint32_t> _planIds;
    uint32_t _lastPlanId = 0;
};
