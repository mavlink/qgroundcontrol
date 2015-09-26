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

/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "MissionManager.h"
#include "Vehicle.h"
#include "MAVLinkProtocol.h"

QGC_LOGGING_CATEGORY(MissionManagerLog, "MissionManagerLog")

MissionManager::MissionManager(Vehicle* vehicle)
    : QThread()
    , _vehicle(vehicle)
    , _cMissionItems(0)
    , _canEdit(true)
    , _ackTimeoutTimer(NULL)
    , _retryAck(AckNone)
{
    moveToThread(this);
    
    connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &MissionManager::_mavlinkMessageReceived);
    
    connect(this, &MissionManager::_writeMissionItemsOnThread, this, &MissionManager::_writeMissionItems);
    connect(this, &MissionManager::_requestMissionItemsOnThread, this, &MissionManager::_requestMissionItems);
    
    start();
}

MissionManager::~MissionManager()
{

}

void MissionManager::run(void)
{
    _ackTimeoutTimer = new QTimer(this);
    _ackTimeoutTimer->setSingleShot(true);
    _ackTimeoutTimer->setInterval(_ackTimeoutMilliseconds);
    
    connect(_ackTimeoutTimer, &QTimer::timeout, this, &MissionManager::_ackTimeout);
    
    _requestMissionItems();
    
    exec();
}

void MissionManager::requestMissionItems(void)
{
    emit _requestMissionItemsOnThread();
}

void MissionManager::writeMissionItems(const QmlObjectListModel& missionItems)
{
    _missionItems.clear();
    for (int i=0; i<missionItems.count(); i++) {
        _missionItems.append(new MissionItem(*qobject_cast<const MissionItem*>(missionItems[i])));
    }

    emit _writeMissionItemsOnThread();
}

void MissionManager::_requestMissionItems(void)
{
    qCDebug(MissionManagerLog) << "_requestMissionItems";
    
    mavlink_message_t               message;
    mavlink_mission_request_list_t  request;
    
    _clearMissionItems();
    
    request.target_system = _vehicle->id();
    request.target_component = MAV_COMP_ID_MISSIONPLANNER;
    
    mavlink_msg_mission_request_list_encode(MAVLinkProtocol::instance()->getSystemId(), MAVLinkProtocol::instance()->getComponentId(), &message, &request);
    
    _vehicle->sendMessage(message);
    _startAckTimeout(AckMissionCount, message);
}

void MissionManager::_ackTimeout(void)
{
    if (_retryAck == AckNone) {
        qCWarning(MissionManagerLog) << "_ackTimeout timeout with AckNone";
        return;
    }
    
    if (++_retryCount <= _maxRetryCount) {
        qCDebug(MissionManagerLog) << "_ackTimeout retry _retryAck:_retryCount" << _retryAck << _retryCount;
        _vehicle->sendMessage(_retryMessage);
    } else {
        qCDebug(MissionManagerLog) << "_ackTimeout failed after max retries _retryAck:_retryCount" << _retryAck << _retryCount;
    }
}

void MissionManager::_startAckTimeout(AckType_t ack, const mavlink_message_t& message)
{
    _retryAck = ack;
    _retryCount = 0;
    _retryMessage = message;
    
    _ackTimeoutTimer->start();
}

bool MissionManager::_stopAckTimeout(AckType_t expectedAck)
{
    bool success;
    
    _ackTimeoutTimer->stop();
    
    if (_retryAck != expectedAck) {
        qCDebug(MissionManagerLog) << "Invalid ack sequence _retryAck:expectedAck" << _retryAck << expectedAck;
        success = false;
    } else {
        success = true;
    }
    
    _retryAck = AckNone;
    
    return success;
}

void MissionManager::_sendTransactionComplete(void)
{
    qCDebug(MissionManagerLog) << "_sendTransactionComplete read sequence complete";
    
    mavlink_message_t       message;
    mavlink_mission_ack_t   missionAck;
    
    missionAck.target_system =      _vehicle->id();
    missionAck.target_component =   MAV_COMP_ID_MISSIONPLANNER;
    missionAck.type =               MAV_MISSION_ACCEPTED;
    
    mavlink_msg_mission_ack_encode(MAVLinkProtocol::instance()->getSystemId(), MAVLinkProtocol::instance()->getComponentId(), &message, &missionAck);
    
    _vehicle->sendMessage(message);
    
    emit newMissionItemsAvailable();
}

void MissionManager::_handleMissionCount(const mavlink_message_t& message)
{
    mavlink_mission_count_t missionCount;
    
    if (!_stopAckTimeout(AckMissionCount)) {
        return;
    }
    
    mavlink_msg_mission_count_decode(&message, &missionCount);
    
    _cMissionItems = missionCount.count;
    qCDebug(MissionManagerLog) << "_handleMissionCount count:" << _cMissionItems;
    
    if (_cMissionItems == 0) {
        _sendTransactionComplete();
    } else {
        _requestNextMissionItem(0);
    }
}

void MissionManager::_requestNextMissionItem(int sequenceNumber)
{
    qCDebug(MissionManagerLog) << "_requestNextMissionItem sequenceNumber:" << sequenceNumber;
    
    if (sequenceNumber >= _cMissionItems) {
        qCWarning(MissionManagerLog) << "_requestNextMissionItem requested seqeuence number > item count sequenceNumber::_cMissionItems" << sequenceNumber << _cMissionItems;
        return;
    }
    
    mavlink_message_t           message;
    mavlink_mission_request_t   missionRequest;
    
    missionRequest.target_system =      _vehicle->id();
    missionRequest.target_component =   MAV_COMP_ID_MISSIONPLANNER;
    missionRequest.seq =                sequenceNumber;
    _expectedSequenceNumber =           sequenceNumber;
    
    mavlink_msg_mission_request_encode(MAVLinkProtocol::instance()->getSystemId(), MAVLinkProtocol::instance()->getComponentId(), &message, &missionRequest);
    
    _vehicle->sendMessage(message);
    _startAckTimeout(AckMissionItem, message);
}

void MissionManager::_handleMissionItem(const mavlink_message_t& message)
{
    mavlink_mission_item_t missionItem;
    
    if (!_stopAckTimeout(AckMissionItem)) {
        return;
    }
    
    mavlink_msg_mission_item_decode(&message, &missionItem);
    
    qCDebug(MissionManagerLog) << "_handleMissionItem sequenceNumber:" << missionItem.seq;
    
    if (missionItem.seq != _expectedSequenceNumber) {
        qCDebug(MissionManagerLog) << "_handleMissionItem mission item received out of sequence expected:actual" << _expectedSequenceNumber << missionItem.seq;
        return;
    }
        
    MissionItem* item = new MissionItem(this,
                                        missionItem.seq,
                                        QGeoCoordinate(missionItem.x, missionItem.y, missionItem.z),
                                        missionItem.param1,
                                        missionItem.param2,
                                        missionItem.param3,
                                        missionItem.param3,
                                        missionItem.autocontinue,
                                        missionItem.current,
                                        missionItem.frame,
                                        missionItem.command);
    _missionItems.append(item);
    
    if (!item->canEdit()) {
        _canEdit = false;
        emit canEditChanged(false);
    }
    
    int nextSequenceNumber = missionItem.seq + 1;
    if (nextSequenceNumber == _cMissionItems) {
        _sendTransactionComplete();
    } else {
        _requestNextMissionItem(nextSequenceNumber);
    }
}

void MissionManager::_clearMissionItems(void)
{
    _cMissionItems = 0;
    _missionItems.clear();
}

void MissionManager::_writeMissionItems(void)
{
    qCDebug(MissionManagerLog) << "writeMissionItems count:" << _missionItems.count();
    
    if (inProgress()) {
        qCDebug(MissionManagerLog) << "writeMissionItems called while transaction in progress";
        // FIXME: Better error handling
        return;
    }
    
    mavlink_message_t       message;
    mavlink_mission_count_t missionCount;
    
    missionCount.target_system = _vehicle->id();
    missionCount.target_component = MAV_COMP_ID_MISSIONPLANNER;
    missionCount.count = _missionItems.count();
    
    mavlink_msg_mission_count_encode(MAVLinkProtocol::instance()->getSystemId(), MAVLinkProtocol::instance()->getComponentId(), &message, &missionCount);
    
    _vehicle->sendMessage(message);
    _startAckTimeout(AckMissionRequest, message);
}

void MissionManager::_handleMissionRequest(const mavlink_message_t& message)
{
    mavlink_mission_request_t missionRequest;
    
    if (!_stopAckTimeout(AckMissionRequest)) {
        return;
    }
    
    mavlink_msg_mission_request_decode(&message, &missionRequest);
    
    qCDebug(MissionManagerLog) << "_handleMissionRequest sequenceNumber:" << missionRequest.seq;
    
    if (missionRequest.seq >= _missionItems.count()) {
        qCDebug(MissionManagerLog) << "_handleMissionRequest invalid sequence number requested:count" << missionRequest.seq << _missionItems.count();
        return;
    }
    
    mavlink_message_t       messageOut;
    mavlink_mission_item_t  missionItem;
    
    MissionItem* item = (MissionItem*)_missionItems[missionRequest.seq];
    
    missionItem.target_system =     _vehicle->id();
    missionItem.target_component =  MAV_COMP_ID_MISSIONPLANNER;
    missionItem.seq =               missionRequest.seq;
    missionItem.command =           item->command();
    missionItem.x =                 item->coordinate().latitude();
    missionItem.y =                 item->coordinate().longitude();
    missionItem.z =                 item->coordinate().altitude();
    missionItem.param1 =            item->param1();
    missionItem.param2 =            item->param2();
    missionItem.param3 =            item->param3();
    missionItem.param4 =            item->param4();
    missionItem.frame =             item->frame();
    missionItem.current =           missionRequest.seq == 0;
    missionItem.autocontinue =      item->autoContinue();
    
    mavlink_msg_mission_item_encode(MAVLinkProtocol::instance()->getSystemId(), MAVLinkProtocol::instance()->getComponentId(), &messageOut, &missionItem);
    
    _vehicle->sendMessage(messageOut);
    // FIXME: This ack sequence isn't quite write
    _startAckTimeout(AckMissionRequest, messageOut);
}

void MissionManager::_handleMissionAck(const mavlink_message_t& message)
{
    mavlink_mission_ack_t missionAck;
    
    if (!_stopAckTimeout(AckMissionRequest)) {
        return;
    }
    
    mavlink_msg_mission_ack_decode(&message, &missionAck);
    
    if (missionAck.type == MAV_MISSION_ACCEPTED) {
        qCDebug(MissionManagerLog) << "_handleMissionAck write sequence complete";
    } else {
        qCDebug(MissionManagerLog) << "_handleMissionAck ack error:" << missionAck.type;
    }
}

/// Called when a new mavlink message for out vehicle is received
void MissionManager::_mavlinkMessageReceived(const mavlink_message_t& message)
{
    switch (message.msgid) {
        case MAVLINK_MSG_ID_MISSION_COUNT:
            _handleMissionCount(message);
            break;

        case MAVLINK_MSG_ID_MISSION_ITEM:
            _handleMissionItem(message);
            break;
            
        case MAVLINK_MSG_ID_MISSION_REQUEST:
            _handleMissionRequest(message);
            break;
            
        case MAVLINK_MSG_ID_MISSION_ACK:
            _handleMissionAck(message);
            break;
            
        case MAVLINK_MSG_ID_MISSION_ITEM_REACHED:
            // FIXME: NYI
            break;
            
        case MAVLINK_MSG_ID_MISSION_CURRENT:
            // FIXME: NYI
            break;
    }
}

QmlObjectListModel* MissionManager::copyMissionItems(void)
{
    QmlObjectListModel* list = new QmlObjectListModel();
    
    for (int i=0; i<_missionItems.count(); i++) {
        list->append(new MissionItem(*qobject_cast<const MissionItem*>(_missionItems[i])));
    }
    
    return list;
}
