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
    : _vehicle(vehicle)
    , _cMissionItems(0)
    , _canEdit(true)
    , _ackTimeoutTimer(NULL)
    , _retryAck(AckNone)
{
    connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &MissionManager::_mavlinkMessageReceived);
    
    _ackTimeoutTimer = new QTimer(this);
    _ackTimeoutTimer->setSingleShot(true);
    _ackTimeoutTimer->setInterval(_ackTimeoutMilliseconds);
    
    connect(_ackTimeoutTimer, &QTimer::timeout, this, &MissionManager::_ackTimeout);
    
    requestMissionItems();
}

MissionManager::~MissionManager()
{

}

void MissionManager::writeMissionItems(const QmlObjectListModel& missionItems, bool skipFirstItem)
{
    _retryCount = 0;
    _missionItems.clear();
    
    for (int i=skipFirstItem ? 1: 0; i<missionItems.count(); i++) {
        _missionItems.append(new MissionItem(*qobject_cast<const MissionItem*>(missionItems[i])));
    }
    
    if (skipFirstItem) {
        for (int i=0; i<_missionItems.count(); i++) {
            MissionItem* item = qobject_cast<MissionItem*>(_missionItems[i]);
            
            if (item->command() == MAV_CMD_CONDITION_DELAY) {
                item->setParam1((int)item->param1() - 1);
            }
        }
    }

    qCDebug(MissionManagerLog) << "writeMissionItems count:" << _missionItems.count();
    
    if (inProgress()) {
        qCDebug(MissionManagerLog) << "writeMissionItems called while transaction in progress";
        return;
    }
    
    mavlink_message_t       message;
    mavlink_mission_count_t missionCount;
    
    _expectedSequenceNumber = 0;
    
    missionCount.target_system = _vehicle->id();
    missionCount.target_component = MAV_COMP_ID_MISSIONPLANNER;
    missionCount.count = _missionItems.count();
    
    mavlink_msg_mission_count_encode(MAVLinkProtocol::instance()->getSystemId(), MAVLinkProtocol::instance()->getComponentId(), &message, &missionCount);
    
    _vehicle->sendMessage(message);
    _startAckTimeout(AckMissionRequest);
    emit inProgressChanged(true);
}

void MissionManager::_retryWrite(void)
{
    qCDebug(MissionManagerLog) << "_retryWrite";
    
    mavlink_message_t       message;
    mavlink_mission_count_t missionCount;
    
    _expectedSequenceNumber = 0;
    
    missionCount.target_system = _vehicle->id();
    missionCount.target_component = MAV_COMP_ID_MISSIONPLANNER;
    missionCount.count = _missionItems.count();
    
    mavlink_msg_mission_count_encode(MAVLinkProtocol::instance()->getSystemId(), MAVLinkProtocol::instance()->getComponentId(), &message, &missionCount);
    
    _vehicle->sendMessage(message);
    _startAckTimeout(AckMissionRequest);
}

void MissionManager::requestMissionItems(void)
{
    qCDebug(MissionManagerLog) << "requestMissionItems read sequence";
    
    mavlink_message_t               message;
    mavlink_mission_request_list_t  request;
    
    _retryCount = 0;
    _clearMissionItems();
    
    request.target_system = _vehicle->id();
    request.target_component = MAV_COMP_ID_MISSIONPLANNER;
    
    mavlink_msg_mission_request_list_encode(MAVLinkProtocol::instance()->getSystemId(), MAVLinkProtocol::instance()->getComponentId(), &message, &request);
    
    _vehicle->sendMessage(message);
    _startAckTimeout(AckMissionCount);
    emit inProgressChanged(true);
}

void MissionManager::_retryRead(void)
{
    qCDebug(MissionManagerLog) << "_retryRead";
    
    mavlink_message_t               message;
    mavlink_mission_request_list_t  request;
    
    _clearMissionItems();
    
    request.target_system = _vehicle->id();
    request.target_component = MAV_COMP_ID_MISSIONPLANNER;
    
    mavlink_msg_mission_request_list_encode(MAVLinkProtocol::instance()->getSystemId(), MAVLinkProtocol::instance()->getComponentId(), &message, &request);
    
    _vehicle->sendMessage(message);
    _startAckTimeout(AckMissionCount);
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
    
    if (!_retrySequence(timedOutAck)) {
        qCDebug(MissionManagerLog) << "_ackTimeout failed after max retries _retryAck:_retryCount" << _ackTypeToString(timedOutAck) << _retryCount;
        _sendError(AckTimeoutError, QString("Vehicle did not respond to mission item communication: %1").arg(_ackTypeToString(timedOutAck)));
    }
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
        qCDebug(MissionManagerLog) << "Invalid ack sequence _retryAck:expectedAck" << _ackTypeToString(savedRetryAck) << _ackTypeToString(expectedAck);
        
        if (_retrySequence(expectedAck)) {
            _sendError(ProtocolOrderError, QString("Vehicle responded incorrectly to mission item protocol sequence: %1:%2").arg(_ackTypeToString(savedRetryAck)).arg(_ackTypeToString(expectedAck)));
        }
        success = false;
    } else {
        success = true;
    }
    
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
    emit inProgressChanged(false);
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
        emit newMissionItemsAvailable();
        emit inProgressChanged(false);
    } else {
        _requestNextMissionItem(0);
    }
}

void MissionManager::_requestNextMissionItem(int sequenceNumber)
{
    qCDebug(MissionManagerLog) << "_requestNextMissionItem sequenceNumber:" << sequenceNumber;
    
    if (sequenceNumber >= _cMissionItems) {
        qCWarning(MissionManagerLog) << "_requestNextMissionItem requested seqeuence number > item count sequenceNumber::_cMissionItems" << sequenceNumber << _cMissionItems;
        _sendError(InternalError, QString("QGroundControl requested mission item outside of range (internal error): %1:%2").arg(sequenceNumber).arg(_cMissionItems));
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
    
    if (missionItem.seq != _expectedSequenceNumber) {
        qCDebug(MissionManagerLog) << "_handleMissionItem mission item received out of sequence expected:actual" << _expectedSequenceNumber << missionItem.seq;
        if (!_retrySequence(AckMissionItem)) {
            _sendError(ItemMismatchError, QString("Vehicle returned incorrect mission item: %1:%2").arg(_expectedSequenceNumber).arg(missionItem.seq));
        }
        return;
    }
        
    MissionItem* item = new MissionItem(this,
                                        missionItem.seq,
                                        QGeoCoordinate(missionItem.x, missionItem.y, missionItem.z),
                                        missionItem.command,
                                        missionItem.param1,
                                        missionItem.param2,
                                        missionItem.param3,
                                        missionItem.param4,
                                        missionItem.autocontinue,
                                        missionItem.current,
                                        missionItem.frame);
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

void MissionManager::_handleMissionRequest(const mavlink_message_t& message)
{
    mavlink_mission_request_t missionRequest;
    
    if (!_stopAckTimeout(AckMissionRequest)) {
        return;
    }
    
    mavlink_msg_mission_request_decode(&message, &missionRequest);
    
    qCDebug(MissionManagerLog) << "_handleMissionRequest sequenceNumber:" << missionRequest.seq;
    
    if (missionRequest.seq != _expectedSequenceNumber) {
        qCDebug(MissionManagerLog) << "_handleMissionRequest invalid sequence number requested: _expectedSequenceNumber:missionRequest.seq" << _expectedSequenceNumber << missionRequest.seq;
        
        if (!_retrySequence(AckMissionRequest)) {
            _sendError(ItemMismatchError, QString("Vehicle requested incorrect mission item: %1:%2").arg(_expectedSequenceNumber).arg(missionRequest.seq));
        }
        return;
    }
    
    _expectedSequenceNumber++;
    
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
    _startAckTimeout(AckMissionRequest);
}

void MissionManager::_handleMissionAck(const mavlink_message_t& message)
{
    mavlink_mission_ack_t missionAck;
    
    // Save th retry ack before calling _stopAckTimeout since we'll need it to determine what
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
            qCDebug(MissionManagerLog) << "_handleMissionAck vehicle sent ack while state machine is idle: error:" << _missionResultToString((MAV_MISSION_RESULT)missionAck.type);
            _sendError(VehicleError, QString("Vehicle sent unexpected MISSION_ACK message, error: %1").arg(_missionResultToString((MAV_MISSION_RESULT)missionAck.type)));
            break;
        case AckMissionCount:
            // MISSION_COUNT message expected
            qCDebug(MissionManagerLog) << "_handleMissionAck vehicle sent ack when MISSION_COUNT expected: error:" << _missionResultToString((MAV_MISSION_RESULT)missionAck.type);
            if (!_retrySequence(AckMissionCount)) {
                _sendError(VehicleError, QString("Vehicle returned error: %1. Partial list of mission items may have been returned.").arg(_missionResultToString((MAV_MISSION_RESULT)missionAck.type)));
            }
            break;
        case AckMissionItem:
            // MISSION_ITEM expected
            qCDebug(MissionManagerLog) << "_handleMissionAck vehicle sent ack when MISSION_ITEM expected: error:" << _missionResultToString((MAV_MISSION_RESULT)missionAck.type);
            if (!_retrySequence(AckMissionItem)) {
                _sendError(VehicleError, QString("Vehicle returned error: %1. Partial list of mission items may have been returned.").arg(_missionResultToString((MAV_MISSION_RESULT)missionAck.type)));
            }
            break;
        case AckMissionRequest:
            // MISSION_REQUEST is expected, or MISSION_ACK to end sequence
            if (missionAck.type == MAV_MISSION_ACCEPTED) {
                if (_expectedSequenceNumber == _missionItems.count()) {
                    qCDebug(MissionManagerLog) << "_handleMissionAck write sequence complete";
                    emit inProgressChanged(false);
                } else {
                    qCDebug(MissionManagerLog) << "_handleMissionAck vehicle did not reqeust all items: _expectedSequenceNumber:_missionItems.count" << _expectedSequenceNumber << _missionItems.count();
                    if (!_retrySequence(AckMissionRequest)) {
                        _sendError(MissingRequestsError, QString("Vehicle did not request all items during write sequence %1:%2. Vehicle only has partial list of mission items.").arg(_expectedSequenceNumber).arg(_missionItems.count()));
                    }
                }
            } else {
                qCDebug(MissionManagerLog) << "_handleMissionAck ack error:" << _missionResultToString((MAV_MISSION_RESULT)missionAck.type);
                if (!_retrySequence(AckMissionRequest)) {
                    _sendError(VehicleError, QString("Vehicle returned error: %1.  Vehicle only has partial list of mission items.").arg(_missionResultToString((MAV_MISSION_RESULT)missionAck.type)));
                }
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

void MissionManager::_sendError(ErrorCode_t errorCode, const QString& errorMsg)
{
    emit inProgressChanged(false);
    emit error(errorCode, errorMsg);
}

/// Retry the protocol sequence given the specified ack
/// @return true: sequence retried, false: out of retries
bool MissionManager::_retrySequence(AckType_t ackType)
{
    qCDebug(MissionManagerLog) << "_retrySequence ackType:" << _ackTypeToString(ackType) << "_retryCount" << _retryCount;
    
    switch (ackType) {
        case AckMissionCount:
        case AckMissionItem:
            if (++_retryCount <= _maxRetryCount) {
                // We are in the middle of a read sequence, start over
                _retryRead();
                return true;
            } else {
                // Read sequence failed, signal for what we have up to this point
                emit newMissionItemsAvailable();
                return false;
            }
            break;
        case AckMissionRequest:
            if (++_retryCount <= _maxRetryCount) {
                // We are in the middle of a write sequence, start over
                _retryWrite();
                return true;
            } else {
                return false;
            }
            break;
        default:
            qCWarning(MissionManagerLog) << "_retrySequence fell through switch: ackType:" << _ackTypeToString(ackType);
            _sendError(InternalError, QString("Internal error occured during Mission Item communication: _retrySequence fell through switch: ackType:").arg(_ackTypeToString(ackType)));
            return false;
    }
}

QString MissionManager::_ackTypeToString(AckType_t ackType)
{
    switch (ackType) {
        case AckNone:   // State machine is idle
            return QString("No Ack");
        case AckMissionCount:   // MISSION_COUNT message expected
            return QString("MISSION_COUNT");
        case AckMissionItem:  ///< MISSION_ITEM expected
            return QString("MISSION_ITEM");
        case AckMissionRequest: ///< MISSION_REQUEST is expected, or MISSION_ACK to end sequence
            return QString("MISSION_REQUEST");
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