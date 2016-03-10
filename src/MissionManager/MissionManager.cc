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
#include "FirmwarePlugin.h"
#include "MAVLinkProtocol.h"
#include "QGCApplication.h"

QGC_LOGGING_CATEGORY(MissionManagerLog, "MissionManagerLog")

MissionManager::MissionManager(Vehicle* vehicle)
    : _vehicle(vehicle)
    , _dedicatedLink(NULL)
    , _ackTimeoutTimer(NULL)
    , _retryAck(AckNone)
    , _readTransactionInProgress(false)
    , _writeTransactionInProgress(false)
    , _currentMissionItem(-1)
{
    connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &MissionManager::_mavlinkMessageReceived);
    
    _ackTimeoutTimer = new QTimer(this);
    _ackTimeoutTimer->setSingleShot(true);
    _ackTimeoutTimer->setInterval(_ackTimeoutMilliseconds);
    
    connect(_ackTimeoutTimer, &QTimer::timeout, this, &MissionManager::_ackTimeout);
}

MissionManager::~MissionManager()
{

}

void MissionManager::writeMissionItems(const QList<MissionItem*>& missionItems)
{
    bool skipFirstItem = !_vehicle->firmwarePlugin()->sendHomePositionToVehicle();

    _missionItems.clear();

    int firstIndex = skipFirstItem ? 1 : 0;
    
    for (int i=firstIndex; i<missionItems.count(); i++) {
        MissionItem* item = new MissionItem(*missionItems[i]);
        _missionItems.append(item);

        item->setIsCurrentItem(i == firstIndex);

        if (skipFirstItem) {
            // Home is in sequence 0, remainder of items start at sequence 1
            item->setSequenceNumber(item->sequenceNumber() - 1);
            if (item->command() == MAV_CMD_DO_JUMP) {
                item->setParam1((int)item->param1() - 1);
            }
        }
    }
    emit newMissionItemsAvailable();

    qCDebug(MissionManagerLog) << "writeMissionItems count:" << _missionItems.count();
    
    if (inProgress()) {
        qCDebug(MissionManagerLog) << "writeMissionItems called while transaction in progress";
        return;
    }

    // Prime write list
    for (int i=0; i<_missionItems.count(); i++) {
        _itemIndicesToWrite << i;
    }
    _writeTransactionInProgress = true;

    mavlink_message_t       message;
    mavlink_mission_count_t missionCount;

    missionCount.target_system = _vehicle->id();
    missionCount.target_component = MAV_COMP_ID_MISSIONPLANNER;
    missionCount.count = _missionItems.count();

    mavlink_msg_mission_count_encode(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(), qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(), &message, &missionCount);

    _dedicatedLink = _vehicle->priorityLink();
    _vehicle->sendMessageOnLink(_dedicatedLink, message);
    _startAckTimeout(AckMissionRequest);
    emit inProgressChanged(true);
}

void MissionManager::writeArduPilotGuidedMissionItem(const QGeoCoordinate& gotoCoord, bool altChangeOnly)
{
    if (inProgress()) {
        qCDebug(MissionManagerLog) << "writeArduPilotGuidedMissionItem called while transaction in progress";
        return;
    }

    _writeTransactionInProgress = true;

    mavlink_message_t       messageOut;
    mavlink_mission_item_t  missionItem;

    missionItem.target_system =     _vehicle->id();
    missionItem.target_component =  0;
    missionItem.seq =               0;
    missionItem.command =           MAV_CMD_NAV_WAYPOINT;
    missionItem.param1 =            0;
    missionItem.param2 =            0;
    missionItem.param3 =            0;
    missionItem.param4 =            0;
    missionItem.x =                 gotoCoord.latitude();
    missionItem.y =                 gotoCoord.longitude();
    missionItem.z =                 gotoCoord.altitude();
    missionItem.frame =             MAV_FRAME_GLOBAL_RELATIVE_ALT;
    missionItem.current =           altChangeOnly ? 3 : 2;
    missionItem.autocontinue =      true;

    mavlink_msg_mission_item_encode(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(), qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(), &messageOut, &missionItem);

    _dedicatedLink = _vehicle->priorityLink();
    _vehicle->sendMessageOnLink(_dedicatedLink, messageOut);
    _startAckTimeout(AckGuidedItem);
    emit inProgressChanged(true);
}

void MissionManager::requestMissionItems(void)
{
    qCDebug(MissionManagerLog) << "requestMissionItems read sequence";
    
    mavlink_message_t               message;
    mavlink_mission_request_list_t  request;
    
    _requestItemRetryCount = 0;
    _itemIndicesToRead.clear();
    _readTransactionInProgress = true;
    _clearMissionItems();
    
    request.target_system = _vehicle->id();
    request.target_component = MAV_COMP_ID_MISSIONPLANNER;
    
    mavlink_msg_mission_request_list_encode(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(), qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(), &message, &request);
    
    _dedicatedLink = _vehicle->priorityLink();
    _vehicle->sendMessageOnLink(_dedicatedLink, message);
    _startAckTimeout(AckMissionCount);
    emit inProgressChanged(true);
}

void MissionManager::_ackTimeout(void)
{
    AckType_t timedOutAck = _retryAck;
    
    _retryAck = AckNone;
    
    if (timedOutAck == AckNone) {
        qCWarning(MissionManagerLog) << "_ackTimeout timeout with AckNone";
        _sendError(InternalError, "Internal error occured during Mission Item communication: _ackTimeOut:_retryAck == AckNone");
        return;
    }
    
    _sendError(AckTimeoutError, QString("Vehicle did not respond to mission item communication: %1").arg(_ackTypeToString(timedOutAck)));
    _finishTransaction(false);
}

void MissionManager::_startAckTimeout(AckType_t ack)
{
    _retryAck = ack;
    _ackTimeoutTimer->start();
}

bool MissionManager::_stopAckTimeout(AckType_t expectedAck)
{
    bool        success = false;
    AckType_t   savedRetryAck = _retryAck;
    
    _retryAck = AckNone;
    
    _ackTimeoutTimer->stop();
    
    if (savedRetryAck != expectedAck) {
        _sendError(ProtocolOrderError, QString("Vehicle responded incorrectly to mission item protocol sequence: %1:%2").arg(_ackTypeToString(savedRetryAck)).arg(_ackTypeToString(expectedAck)));
        _finishTransaction(false);
        success = false;
    } else {
        success = true;
    }
    
    return success;
}

void MissionManager::_readTransactionComplete(void)
{
    qCDebug(MissionManagerLog) << "_readTransactionComplete read sequence complete";
    
    mavlink_message_t       message;
    mavlink_mission_ack_t   missionAck;
    
    missionAck.target_system =      _vehicle->id();
    missionAck.target_component =   MAV_COMP_ID_MISSIONPLANNER;
    missionAck.type =               MAV_MISSION_ACCEPTED;
    
    mavlink_msg_mission_ack_encode(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(), qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(), &message, &missionAck);
    
    _vehicle->sendMessageOnLink(_dedicatedLink, message);

    _finishTransaction(true);
    emit newMissionItemsAvailable();
}

void MissionManager::_handleMissionCount(const mavlink_message_t& message)
{
    mavlink_mission_count_t missionCount;
    
    if (!_stopAckTimeout(AckMissionCount)) {
        return;
    }
    
    mavlink_msg_mission_count_decode(&message, &missionCount);
    qCDebug(MissionManagerLog) << "_handleMissionCount count:" << missionCount.count;

    if (missionCount.count == 0) {
        _readTransactionComplete();
    } else {
        // Prime read list
        for (int i=0; i<missionCount.count; i++) {
            _itemIndicesToRead << i;
        }
        _requestNextMissionItem();
    }
}

void MissionManager::_requestNextMissionItem(void)
{
    qCDebug(MissionManagerLog) << "_requestNextMissionItem sequenceNumber:" << _itemIndicesToRead[0];

    if (_itemIndicesToRead.count() == 0) {
        _sendError(InternalError, "Internal Error: Call to Vehicle _requestNextMissionItem with no more indices to read");
        return;
    }
    
    mavlink_message_t           message;
    mavlink_mission_request_t   missionRequest;
    
    missionRequest.target_system =      _vehicle->id();
    missionRequest.target_component =   MAV_COMP_ID_MISSIONPLANNER;
    missionRequest.seq =                _itemIndicesToRead[0];
    
    mavlink_msg_mission_request_encode(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(), qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(), &message, &missionRequest);
    
    _vehicle->sendMessageOnLink(_dedicatedLink, message);
    _startAckTimeout(AckMissionItem);
}

void MissionManager::_handleMissionItem(const mavlink_message_t& message)
{
    mavlink_mission_item_t missionItem;
    
    if (!_stopAckTimeout(AckMissionItem)) {
        return;
    }
    
    mavlink_msg_mission_item_decode(&message, &missionItem);
    
    qCDebug(MissionManagerLog) << "_handleMissionItem sequenceNumber:" << missionItem.seq;
    
    if (_itemIndicesToRead.contains(missionItem.seq)) {
        _requestItemRetryCount = 0;
        _itemIndicesToRead.removeOne(missionItem.seq);

        MissionItem* item = new MissionItem(missionItem.seq,
                                            (MAV_CMD)missionItem.command,
                                            (MAV_FRAME)missionItem.frame,
                                            missionItem.param1,
                                            missionItem.param2,
                                            missionItem.param3,
                                            missionItem.param4,
                                            missionItem.x,
                                            missionItem.y,
                                            missionItem.z,
                                            missionItem.autocontinue,
                                            missionItem.current,
                                            this);

        if (item->command() == MAV_CMD_DO_JUMP && !_vehicle->firmwarePlugin()->sendHomePositionToVehicle()) {
            // Home is in position 0
            item->setParam1((int)item->param1() + 1);
        }

        _missionItems.append(item);
    } else {
        qCDebug(MissionManagerLog) << "_handleMissionItem mission item received item index which was not requested, disregrarding:" << missionItem.seq;
        if (++_requestItemRetryCount > _maxRetryCount) {
            _sendError(RequestRangeError, QString("Vehicle would not send item %1 after max retries. Read from Vehicle failed.").arg(_itemIndicesToRead[0]));
            _finishTransaction(false);
            return;
        }
    }
    
    if (_itemIndicesToRead.count() == 0) {
        _readTransactionComplete();
    } else {
        _requestNextMissionItem();
    }
}

void MissionManager::_clearMissionItems(void)
{
    _itemIndicesToRead.clear();
    _missionItems.clear();
}

void MissionManager::_handleMissionRequest(const mavlink_message_t& message)
{
    mavlink_mission_request_t missionRequest;
    
    if (!_stopAckTimeout(AckMissionRequest)) {
        return;
    }
    
    mavlink_msg_mission_request_decode(&message, &missionRequest);
    
    qCDebug(MissionManagerLog) << "_handleMissionRequest sequenceNumber:" << missionRequest.seq;
    
    if (!_itemIndicesToWrite.contains(missionRequest.seq)) {
        if (missionRequest.seq > _missionItems.count()) {
            _sendError(RequestRangeError, QString("Vehicle requested item outside range, count:request %1:%2. Send to Vehicle failed.").arg(_missionItems.count()).arg(missionRequest.seq));
            _finishTransaction(false);
            return;
        } else {
            qCDebug(MissionManagerLog) << "_handleMissionRequest sequence number requested which has already been sent, sending again:" << missionRequest.seq;
        }
    } else {
        _itemIndicesToWrite.removeOne(missionRequest.seq);
    }
    
    mavlink_message_t       messageOut;
    mavlink_mission_item_t  missionItem;
    
    MissionItem* item = _missionItems[missionRequest.seq];
    
    missionItem.target_system =     _vehicle->id();
    missionItem.target_component =  MAV_COMP_ID_MISSIONPLANNER;
    missionItem.seq =               missionRequest.seq;
    missionItem.command =           item->command();
    missionItem.param1 =            item->param1();
    missionItem.param2 =            item->param2();
    missionItem.param3 =            item->param3();
    missionItem.param4 =            item->param4();
    missionItem.x =                 item->param5();
    missionItem.y =                 item->param6();
    missionItem.z =                 item->param7();
    missionItem.frame =             item->frame();
    missionItem.current =           missionRequest.seq == 0;
    missionItem.autocontinue =      item->autoContinue();
    
    mavlink_msg_mission_item_encode(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(), qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(), &messageOut, &missionItem);
    
    _vehicle->sendMessageOnLink(_dedicatedLink, messageOut);
    _startAckTimeout(AckMissionRequest);
}

void MissionManager::_handleMissionAck(const mavlink_message_t& message)
{
    mavlink_mission_ack_t missionAck;
    
    // Save the retry ack before calling _stopAckTimeout since we'll need it to determine what
    // type of a protocol sequence we are in.
    AckType_t savedRetryAck = _retryAck;
    
    // We can get a MISSION_ACK with an error at any time, so if the Acks don't match it is not
    // a protocol sequence error. Call _stopAckTimeout with _retryAck so it will succeed no
    // matter what.
    if (!_stopAckTimeout(_retryAck)) {
        return;
    }
    
    mavlink_msg_mission_ack_decode(&message, &missionAck);
    
    qCDebug(MissionManagerLog) << "_handleMissionAck type:" << _missionResultToString((MAV_MISSION_RESULT)missionAck.type);

    switch (savedRetryAck) {
        case AckNone:
            // State machine is idle. Vehicle is confused.
            _sendError(VehicleError, QString("Vehicle sent unexpected MISSION_ACK message, error: %1").arg(_missionResultToString((MAV_MISSION_RESULT)missionAck.type)));
            break;
        case AckMissionCount:
            // MISSION_COUNT message expected
            _sendError(VehicleError, QString("Vehicle returned error: %1.").arg(_missionResultToString((MAV_MISSION_RESULT)missionAck.type)));
            _finishTransaction(false);
            break;
        case AckMissionItem:
            // MISSION_ITEM expected
            _sendError(VehicleError, QString("Vehicle returned error: %1. Partial list of mission items may have been returned.").arg(_missionResultToString((MAV_MISSION_RESULT)missionAck.type)));
            _finishTransaction(false);
            break;
        case AckMissionRequest:
            // MISSION_REQUEST is expected, or MISSION_ACK to end sequence
            if (missionAck.type == MAV_MISSION_ACCEPTED) {
                if (_itemIndicesToWrite.count() == 0) {
                    qCDebug(MissionManagerLog) << "_handleMissionAck write sequence complete";
                    _finishTransaction(true);
                } else {
                    _sendError(MissingRequestsError, QString("Vehicle did not request all items during write sequence, missed count %1. Vehicle only has partial list of mission items.").arg(_itemIndicesToWrite.count()));
                    _finishTransaction(false);
                }
            } else {
                _sendError(VehicleError, QString("Vehicle returned error: %1. Vehicle only has partial list of mission items.").arg(_missionResultToString((MAV_MISSION_RESULT)missionAck.type)));
                _finishTransaction(false);
            }
            break;
        case AckGuidedItem:
            // MISSION_REQUEST is expected, or MISSION_ACK to end sequence
            if (missionAck.type == MAV_MISSION_ACCEPTED) {
                qCDebug(MissionManagerLog) << "_handleMissionAck guide mode item accepted";
                _finishTransaction(true);
            } else {
                _sendError(VehicleError, QString("Vehicle returned error: %1. Vehicle did not accept guided item.").arg(_missionResultToString((MAV_MISSION_RESULT)missionAck.type)));
                _finishTransaction(false);
            }
            break;
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
            _handleMissionCurrent(message);
            break;
    }
}

void MissionManager::_sendError(ErrorCode_t errorCode, const QString& errorMsg)
{
    qCDebug(MissionManagerLog) << "Sending error" << errorCode << errorMsg;

    emit error(errorCode, errorMsg);
}

QString MissionManager::_ackTypeToString(AckType_t ackType)
{
    switch (ackType) {
        case AckNone:
            return QString("No Ack");
        case AckMissionCount:
            return QString("MISSION_COUNT");
        case AckMissionItem:
            return QString("MISSION_ITEM");
        case AckMissionRequest:
            return QString("MISSION_REQUEST");
        case AckGuidedItem:
            return QString("Guided Mode Item");
        default:
            qWarning(MissionManagerLog) << "Fell off end of switch statement";
            return QString("QGC Internal Error");
    }    
}

QString MissionManager::_missionResultToString(MAV_MISSION_RESULT result)
{
    switch (result) {
        case MAV_MISSION_ACCEPTED:
            return QString("Mission accepted (MAV_MISSION_ACCEPTED)");
            break;
        case MAV_MISSION_ERROR:
            return QString("Unspecified error (MAV_MISSION_ERROR)");
            break;
        case MAV_MISSION_UNSUPPORTED_FRAME:
            return QString("Coordinate frame is not supported (MAV_MISSION_UNSUPPORTED_FRAME)");
            break;
        case MAV_MISSION_UNSUPPORTED:
            return QString("Command is not supported (MAV_MISSION_UNSUPPORTED)");
            break;
        case MAV_MISSION_NO_SPACE:
            return QString("Mission item exceeds storage space (MAV_MISSION_NO_SPACE)");
            break;
        case MAV_MISSION_INVALID:
            return QString("One of the parameters has an invalid value (MAV_MISSION_INVALID)");
            break;
        case MAV_MISSION_INVALID_PARAM1:
            return QString("Param1 has an invalid value (MAV_MISSION_INVALID_PARAM1)");
            break;
        case MAV_MISSION_INVALID_PARAM2:
            return QString("Param2 has an invalid value (MAV_MISSION_INVALID_PARAM2)");
            break;
        case MAV_MISSION_INVALID_PARAM3:
            return QString("param3 has an invalid value (MAV_MISSION_INVALID_PARAM3)");
            break;
        case MAV_MISSION_INVALID_PARAM4:
            return QString("Param4 has an invalid value (MAV_MISSION_INVALID_PARAM4)");
            break;
        case MAV_MISSION_INVALID_PARAM5_X:
            return QString("X/Param5 has an invalid value (MAV_MISSION_INVALID_PARAM5_X)");
            break;
        case MAV_MISSION_INVALID_PARAM6_Y:
            return QString("Y/Param6 has an invalid value (MAV_MISSION_INVALID_PARAM6_Y)");
            break;
        case MAV_MISSION_INVALID_PARAM7:
            return QString("Param7 has an invalid value (MAV_MISSION_INVALID_PARAM7)");
            break;
        case MAV_MISSION_INVALID_SEQUENCE:
            return QString("Received mission item out of sequence (MAV_MISSION_INVALID_SEQUENCE)");
            break;
        case MAV_MISSION_DENIED:
            return QString("Not accepting any mission commands (MAV_MISSION_DENIED)");
            break;
        default:
            qWarning(MissionManagerLog) << "Fell off end of switch statement";
            return QString("QGC Internal Error");
    }
}

void MissionManager::_finishTransaction(bool success)
{
    if (!success && _readTransactionInProgress) {
        // Read from vehicle failed, clear partial list
        _missionItems.clear();
        emit newMissionItemsAvailable();
    }

    _readTransactionInProgress = false;
    _writeTransactionInProgress = false;
    _itemIndicesToRead.clear();
    _itemIndicesToWrite.clear();

    emit inProgressChanged(false);
}

bool MissionManager::inProgress(void)
{
    return _readTransactionInProgress || _writeTransactionInProgress;
}

void MissionManager::_handleMissionCurrent(const mavlink_message_t& message)
{
    mavlink_mission_current_t missionCurrent;

    mavlink_msg_mission_current_decode(&message, &missionCurrent);

    qCDebug(MissionManagerLog) << "_handleMissionCurrent seq:" << missionCurrent.seq;
    if (missionCurrent.seq != _currentMissionItem) {
        _currentMissionItem = missionCurrent.seq;
        emit currentItemChanged(_currentMissionItem);
    }
}
