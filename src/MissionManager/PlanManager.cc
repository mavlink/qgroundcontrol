/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "PlanManager.h"
#include "Vehicle.h"
#include "FirmwarePlugin.h"
#include "MAVLinkProtocol.h"
#include "QGCApplication.h"
#include "MissionCommandTree.h"
#include "MissionCommandUIInfo.h"

QGC_LOGGING_CATEGORY(PlanManagerLog, "PlanManagerLog")

PlanManager::PlanManager(Vehicle* vehicle, MAV_MISSION_TYPE planType)
    : _vehicle(vehicle)
    , _planType(planType)
    , _dedicatedLink(NULL)
    , _ackTimeoutTimer(NULL)
    , _expectedAck(AckNone)
    , _transactionInProgress(TransactionNone)
    , _resumeMission(false)
    , _lastMissionRequest(-1)
    , _currentMissionIndex(-1)
    , _lastCurrentIndex(-1)
{
    _ackTimeoutTimer = new QTimer(this);
    _ackTimeoutTimer->setSingleShot(true);
    _ackTimeoutTimer->setInterval(_ackTimeoutMilliseconds);
    
    connect(_ackTimeoutTimer, &QTimer::timeout, this, &PlanManager::_ackTimeout);
    connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &PlanManager::_mavlinkMessageReceived);
}

PlanManager::~PlanManager()
{

}

void PlanManager::_writeMissionItemsWorker(void)
{
    _lastMissionRequest = -1;

    emit progressPct(0);

    qCDebug(PlanManagerLog) << QStringLiteral("writeMissionItems %1 count:").arg(_planTypeString()) << _writeMissionItems.count();

    // Prime write list
    _itemIndicesToWrite.clear();
    for (int i=0; i<_writeMissionItems.count(); i++) {
        _itemIndicesToWrite << i;
    }

    _transactionInProgress = TransactionWrite;
    _retryCount = 0;
    emit inProgressChanged(true);
    _writeMissionCount();
}


void PlanManager::writeMissionItems(const QList<MissionItem*>& missionItems)
{
    if (_vehicle->isOfflineEditingVehicle()) {
        return;
    }

    if (inProgress()) {
        qCDebug(PlanManagerLog) << QStringLiteral("writeMissionItems %1 called while transaction in progress").arg(_planTypeString());
        return;
    }

    _clearAndDeleteWriteMissionItems();

    bool skipFirstItem = _planType == MAV_MISSION_TYPE_MISSION && !_vehicle->firmwarePlugin()->sendHomePositionToVehicle();

    int firstIndex = skipFirstItem ? 1 : 0;

    for (int i=firstIndex; i<missionItems.count(); i++) {
        MissionItem* item = new MissionItem(*missionItems[i]);
        _writeMissionItems.append(item);

        item->setIsCurrentItem(i == firstIndex);

        if (skipFirstItem) {
            // Home is in sequence 0, remainder of items start at sequence 1
            item->setSequenceNumber(item->sequenceNumber() - 1);
            if (item->command() == MAV_CMD_DO_JUMP) {
                item->setParam1((int)item->param1() - 1);
            }
        }
    }

    _writeMissionItemsWorker();
}

/// This begins the write sequence with the vehicle. This may be called during a retry.
void PlanManager::_writeMissionCount(void)
{
    qCDebug(PlanManagerLog) << QStringLiteral("_writeMissionCount %1 count:_retryCount").arg(_planTypeString()) << _writeMissionItems.count() << _retryCount;

    mavlink_message_t message;

    _dedicatedLink = _vehicle->priorityLink();
    mavlink_msg_mission_count_pack_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                        qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                        _dedicatedLink->mavlinkChannel(),
                                        &message,
                                        _vehicle->id(),
                                        MAV_COMP_ID_MISSIONPLANNER,
                                        _writeMissionItems.count(),
                                        _planType);

    _vehicle->sendMessageOnLink(_dedicatedLink, message);
    _startAckTimeout(AckMissionRequest);
}

void PlanManager::loadFromVehicle(void)
{
    if (_vehicle->isOfflineEditingVehicle()) {
        return;
    }

    qCDebug(PlanManagerLog) << QStringLiteral("loadFromVehicle %1 read sequence").arg(_planTypeString());

    if (inProgress()) {
        qCDebug(PlanManagerLog) << QStringLiteral("loadFromVehicle %1 called while transaction in progress").arg(_planTypeString());
        return;
    }

    _retryCount = 0;
    _transactionInProgress = TransactionRead;
    emit inProgressChanged(true);
    _requestList();
}

/// Internal call to request list of mission items. May be called during a retry sequence.
void PlanManager::_requestList(void)
{
    qCDebug(PlanManagerLog) << QStringLiteral("_requestList %1 _planType:_retryCount").arg(_planTypeString()) << _planType << _retryCount;

    mavlink_message_t message;

    _itemIndicesToRead.clear();
    _clearMissionItems();

    _dedicatedLink = _vehicle->priorityLink();
    mavlink_msg_mission_request_list_pack_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                               qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                               _dedicatedLink->mavlinkChannel(),
                                               &message,
                                               _vehicle->id(),
                                               MAV_COMP_ID_MISSIONPLANNER,
                                               _planType);

    _vehicle->sendMessageOnLink(_dedicatedLink, message);
    _startAckTimeout(AckMissionCount);
}

void PlanManager::_ackTimeout(void)
{
    if (_expectedAck == AckNone) {
        return;
    }

    switch (_expectedAck) {
    case AckNone:
        qCWarning(PlanManagerLog) << QStringLiteral("_ackTimeout %1 timeout with AckNone").arg(_planTypeString());
        _sendError(InternalError, "Internal error occurred during Mission Item communication: _ackTimeOut:_expectedAck == AckNone");
        break;
    case AckMissionCount:
        // MISSION_COUNT message expected
        if (_retryCount > _maxRetryCount) {
            _sendError(VehicleError, QStringLiteral("Mission request list failed, maximum retries exceeded."));
            _finishTransaction(false);
        } else {
            _retryCount++;
            qCDebug(PlanManagerLog) << QStringLiteral("Retrying %1 REQUEST_LIST retry Count").arg(_planTypeString()) << _retryCount;
            _requestList();
        }
        break;
    case AckMissionItem:
        // MISSION_ITEM expected
        if (_retryCount > _maxRetryCount) {
            _sendError(VehicleError, QStringLiteral("Mission read failed, maximum retries exceeded."));
            _finishTransaction(false);
        } else {
            _retryCount++;
            qCDebug(PlanManagerLog) << QStringLiteral("Retrying %1 MISSION_REQUEST retry Count").arg(_planTypeString()) << _retryCount;
            _requestNextMissionItem();
        }
        break;
    case AckMissionRequest:
        // MISSION_REQUEST is expected, or MISSION_ACK to end sequence
        if (_itemIndicesToWrite.count() == 0) {
            // Vehicle did not send final MISSION_ACK at end of sequence
            _sendError(VehicleError, QStringLiteral("Mission write failed, vehicle failed to send final ack."));
            _finishTransaction(false);
        } else if (_itemIndicesToWrite[0] == 0) {
            // Vehicle did not respond to MISSION_COUNT, try again
            if (_retryCount > _maxRetryCount) {
                _sendError(VehicleError, QStringLiteral("Mission write mission count failed, maximum retries exceeded."));
                _finishTransaction(false);
            } else {
                _retryCount++;
                qCDebug(PlanManagerLog) << QStringLiteral("Retrying %1 MISSION_COUNT retry Count").arg(_planTypeString()) << _retryCount;
                _writeMissionCount();
            }
        } else {
            // Vehicle did not request all items from ground station
            _sendError(AckTimeoutError, QString("Vehicle did not request all items from ground station: %1").arg(_ackTypeToString(_expectedAck)));
            _expectedAck = AckNone;
            _finishTransaction(false);
        }
        break;
    case AckMissionClearAll:
        // MISSION_ACK expected
        if (_retryCount > _maxRetryCount) {
            _sendError(VehicleError, QStringLiteral("Mission remove all, maximum retries exceeded."));
            _finishTransaction(false);
        } else {
            _retryCount++;
            qCDebug(PlanManagerLog) << QStringLiteral("Retrying %1 MISSION_CLEAR_ALL retry Count").arg(_planTypeString()) << _retryCount;
            _removeAllWorker();
        }
        break;
    case AckGuidedItem:
        // MISSION_REQUEST is expected, or MISSION_ACK to end sequence
    default:
        _sendError(AckTimeoutError, QString("Vehicle did not respond to mission item communication: %1").arg(_ackTypeToString(_expectedAck)));
        _expectedAck = AckNone;
        _finishTransaction(false);
    }
}

void PlanManager::_startAckTimeout(AckType_t ack)
{
    _expectedAck = ack;
    _ackTimeoutTimer->start();
}

/// Checks the received ack against the expected ack. If they match the ack timeout timer will be stopped.
/// @return true: received ack matches expected ack
bool PlanManager::_checkForExpectedAck(AckType_t receivedAck)
{
    if (receivedAck == _expectedAck) {
        _expectedAck = AckNone;
        _ackTimeoutTimer->stop();
        return true;
    } else {
        if (_expectedAck == AckNone) {
            // Don't worry about unexpected mission commands, just ignore them; ArduPilot updates home position using
            // spurious MISSION_ITEMs.
        } else {
            // We just warn in this case, this could be crap left over from a previous transaction or the vehicle going bonkers.
            // Whatever it is we let the ack timeout handle any error output to the user.
            qCDebug(PlanManagerLog) << QString("Out of sequence ack expected:received %1:%2 %1").arg(_ackTypeToString(_expectedAck)).arg(_ackTypeToString(receivedAck)).arg(_planTypeString());
        }
        return false;
    }
}

void PlanManager::_readTransactionComplete(void)
{
    qCDebug(PlanManagerLog) << "_readTransactionComplete read sequence complete";
    
    mavlink_message_t message;
    
    mavlink_msg_mission_ack_pack_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                      qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                      _dedicatedLink->mavlinkChannel(),
                                      &message,
                                      _vehicle->id(),
                                      MAV_COMP_ID_MISSIONPLANNER,
                                      MAV_MISSION_ACCEPTED,
                                      _planType);
    
    _vehicle->sendMessageOnLink(_dedicatedLink, message);

    _finishTransaction(true);
}

void PlanManager::_handleMissionCount(const mavlink_message_t& message)
{
    mavlink_mission_count_t missionCount;

    mavlink_msg_mission_count_decode(&message, &missionCount);

    if (missionCount.mission_type != _planType) {
        // if there was a previous transaction with a different mission_type, it can happen that we receive
        // a stale message here, for example when the MAV ran into a timeout and sent a message twice
        qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionCount %1 Incorrect mission_type received expected:actual").arg(_planTypeString()) << _planType << missionCount.mission_type;
        return;
    }

    if (!_checkForExpectedAck(AckMissionCount)) {
        return;
    }

    qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionCount %1 count:").arg(_planTypeString()) << missionCount.count;

    _retryCount = 0;

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

void PlanManager::_requestNextMissionItem(void)
{
    if (_itemIndicesToRead.count() == 0) {
        _sendError(InternalError, "Internal Error: Call to Vehicle _requestNextMissionItem with no more indices to read");
        return;
    }

    qCDebug(PlanManagerLog) << QStringLiteral("_requestNextMissionItem %1 sequenceNumber:retry").arg(_planTypeString()) << _itemIndicesToRead[0] << _retryCount;

    mavlink_message_t message;
    if (_vehicle->capabilityBits() & MAV_PROTOCOL_CAPABILITY_MISSION_INT) {
        mavlink_msg_mission_request_int_pack_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                                  qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                                  _dedicatedLink->mavlinkChannel(),
                                                  &message,
                                                  _vehicle->id(),
                                                  MAV_COMP_ID_MISSIONPLANNER,
                                                  _itemIndicesToRead[0],
                _planType);
    } else {
        mavlink_msg_mission_request_pack_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                              qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                              _dedicatedLink->mavlinkChannel(),
                                              &message,
                                              _vehicle->id(),
                                              MAV_COMP_ID_MISSIONPLANNER,
                                              _itemIndicesToRead[0],
                _planType);
    }
    
    _vehicle->sendMessageOnLink(_dedicatedLink, message);
    _startAckTimeout(AckMissionItem);
}

void PlanManager::_handleMissionItem(const mavlink_message_t& message, bool missionItemInt)
{
    MAV_CMD     command;
    MAV_FRAME   frame;
    double      param1;
    double      param2;
    double      param3;
    double      param4;
    double      param5;
    double      param6;
    double      param7;
    bool        autoContinue;
    bool        isCurrentItem;
    int         seq;

    if (missionItemInt) {
        mavlink_mission_item_int_t missionItem;
        mavlink_msg_mission_item_int_decode(&message, &missionItem);

        command =       (MAV_CMD)missionItem.command,
                frame =         (MAV_FRAME)missionItem.frame,
                param1 =        missionItem.param1;
        param2 =        missionItem.param2;
        param3 =        missionItem.param3;
        param4 =        missionItem.param4;
        param5 =        (double)missionItem.x / qPow(10.0, 7.0);
        param6 =        (double)missionItem.y / qPow(10.0, 7.0);
        param7 =        (double)missionItem.z;
        autoContinue =  missionItem.autocontinue;
        isCurrentItem = missionItem.current;
        seq =           missionItem.seq;
    } else {
        mavlink_mission_item_t missionItem;
        mavlink_msg_mission_item_decode(&message, &missionItem);

        command =       (MAV_CMD)missionItem.command,
                frame =         (MAV_FRAME)missionItem.frame,
                param1 =        missionItem.param1;
        param2 =        missionItem.param2;
        param3 =        missionItem.param3;
        param4 =        missionItem.param4;
        param5 =        missionItem.x;
        param6 =        missionItem.y;
        param7 =        missionItem.z;
        autoContinue =  missionItem.autocontinue;
        isCurrentItem = missionItem.current;
        seq =           missionItem.seq;
    }

    // We don't support editing ALT_INT frames so change on the way in.
    if (frame == MAV_FRAME_GLOBAL_INT) {
        frame = MAV_FRAME_GLOBAL;
    } else if (frame == MAV_FRAME_GLOBAL_RELATIVE_ALT_INT) {
        frame = MAV_FRAME_GLOBAL_RELATIVE_ALT;
    }
    

    bool ardupilotHomePositionUpdate = false;
    if (!_checkForExpectedAck(AckMissionItem)) {
        if (_vehicle->apmFirmware() && seq ==  0 && _planType == MAV_MISSION_TYPE_MISSION) {
            ardupilotHomePositionUpdate = true;
        } else {
            qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionItem %1 dropping spurious item seq:command:current").arg(_planTypeString()) << seq << command << isCurrentItem;
            return;
        }
    }

    qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionItem %1 seq:command:current:ardupilotHomePositionUpdate").arg(_planTypeString()) << seq << command << isCurrentItem << ardupilotHomePositionUpdate;

    if (ardupilotHomePositionUpdate) {
        QGeoCoordinate newHomePosition(param5, param6, param7);
        _vehicle->_setHomePosition(newHomePosition);
        return;
    }
    
    if (_itemIndicesToRead.contains(seq)) {
        _itemIndicesToRead.removeOne(seq);

        MissionItem* item = new MissionItem(seq,
                                            command,
                                            frame,
                                            param1,
                                            param2,
                                            param3,
                                            param4,
                                            param5,
                                            param6,
                                            param7,
                                            autoContinue,
                                            isCurrentItem,
                                            this);

        if (item->command() == MAV_CMD_DO_JUMP && !_vehicle->firmwarePlugin()->sendHomePositionToVehicle()) {
            // Home is in position 0
            item->setParam1((int)item->param1() + 1);
        }

        _missionItems.append(item);
    } else {
        qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionItem %1 mission item received item index which was not requested, disregrarding:").arg(_planTypeString()) << seq;
        // We have to put the ack timeout back since it was removed above
        _startAckTimeout(AckMissionItem);
        return;
    }

    emit progressPct((double)seq / (double)_missionItems.count());
    
    _retryCount = 0;
    if (_itemIndicesToRead.count() == 0) {
        _readTransactionComplete();
    } else {
        _requestNextMissionItem();
    }
}

void PlanManager::_clearMissionItems(void)
{
    _itemIndicesToRead.clear();
    _clearAndDeleteMissionItems();
}

void PlanManager::_handleMissionRequest(const mavlink_message_t& message, bool missionItemInt)
{
    mavlink_mission_request_t missionRequest;
    
    mavlink_msg_mission_request_decode(&message, &missionRequest);

    if (missionRequest.mission_type != _planType) {
        // if there was a previous transaction with a different mission_type, it can happen that we receive
        // a stale message here, for example when the MAV ran into a timeout and sent a message twice
        qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionRequest %1 Incorrect mission_type received expected:actual").arg(_planTypeString()) << _planType << missionRequest.mission_type;
        return;
    }
    
    if (!_checkForExpectedAck(AckMissionRequest)) {
        return;
    }

    qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionRequest %1 sequenceNumber").arg(_planTypeString()) << missionRequest.seq;

    if (missionRequest.seq > _writeMissionItems.count() - 1) {
        _sendError(RequestRangeError, QString("Vehicle requested item outside range, count:request %1:%2. Send to Vehicle failed.").arg(_writeMissionItems.count()).arg(missionRequest.seq));
        _finishTransaction(false);
        return;
    }

    emit progressPct((double)missionRequest.seq / (double)_writeMissionItems.count());

    _lastMissionRequest = missionRequest.seq;
    if (!_itemIndicesToWrite.contains(missionRequest.seq)) {
        qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionRequest %1 sequence number requested which has already been sent, sending again:").arg(_planTypeString()) << missionRequest.seq;
    } else {
        _itemIndicesToWrite.removeOne(missionRequest.seq);
    }
    
    MissionItem* item = _writeMissionItems[missionRequest.seq];
    qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionRequest %1 sequenceNumber:command").arg(_planTypeString()) << missionRequest.seq << item->command();

    mavlink_message_t   messageOut;
    if (missionItemInt) {
        mavlink_msg_mission_item_int_pack_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                               qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                               _dedicatedLink->mavlinkChannel(),
                                               &messageOut,
                                               _vehicle->id(),
                                               MAV_COMP_ID_MISSIONPLANNER,
                                               missionRequest.seq,
                                               item->frame(),
                                               item->command(),
                                               missionRequest.seq == 0,
                                               item->autoContinue(),
                                               item->param1(),
                                               item->param2(),
                                               item->param3(),
                                               item->param4(),
                                               item->param5() * qPow(10.0, 7.0),
                                               item->param6() * qPow(10.0, 7.0),
                                               item->param7(),
                                               _planType);
    } else {
        mavlink_msg_mission_item_pack_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                           qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                           _dedicatedLink->mavlinkChannel(),
                                           &messageOut,
                                           _vehicle->id(),
                                           MAV_COMP_ID_MISSIONPLANNER,
                                           missionRequest.seq,
                                           item->frame(),
                                           item->command(),
                                           missionRequest.seq == 0,
                                           item->autoContinue(),
                                           item->param1(),
                                           item->param2(),
                                           item->param3(),
                                           item->param4(),
                                           item->param5(),
                                           item->param6(),
                                           item->param7(),
                                           _planType);
    }
    
    _vehicle->sendMessageOnLink(_dedicatedLink, messageOut);
    _startAckTimeout(AckMissionRequest);
}

void PlanManager::_handleMissionAck(const mavlink_message_t& message)
{
    mavlink_mission_ack_t missionAck;
    
    mavlink_msg_mission_ack_decode(&message, &missionAck);
    if (missionAck.mission_type != _planType) {
        // if there was a previous transaction with a different mission_type, it can happen that we receive
        // a stale message here, for example when the MAV ran into a timeout and sent a message twice
        qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionAck %1 Incorrect mission_type received expected:actual").arg(_planTypeString()) << _planType << missionAck.mission_type;
        return;
    }

    // Save the retry ack before calling _checkForExpectedAck since we'll need it to determine what
    // type of a protocol sequence we are in.
    AckType_t savedExpectedAck = _expectedAck;
    
    // We can get a MISSION_ACK with an error at any time, so if the Acks don't match it is not
    // a protocol sequence error. Call _checkForExpectedAck with _retryAck so it will succeed no
    // matter what.
    if (!_checkForExpectedAck(_expectedAck)) {
        return;
    }

    qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionAck %1 type:").arg(_planTypeString()) << _missionResultToString((MAV_MISSION_RESULT)missionAck.type);

    switch (savedExpectedAck) {
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
        _sendError(VehicleError, QString("Vehicle returned error: %1.").arg(_missionResultToString((MAV_MISSION_RESULT)missionAck.type)));
        _finishTransaction(false);
        break;
    case AckMissionRequest:
        // MISSION_REQUEST is expected, or MISSION_ACK to end sequence
        if (missionAck.type == MAV_MISSION_ACCEPTED) {
            if (_itemIndicesToWrite.count() == 0) {
                qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionAck write sequence complete").arg(_planTypeString());
                _finishTransaction(true);
            } else {
                _sendError(MissingRequestsError, QString("Vehicle did not request all items during write sequence, missed count %1.").arg(_itemIndicesToWrite.count()));
                _finishTransaction(false);
            }
        } else {
            _sendError(VehicleError, QString("Vehicle returned error: %1.").arg(_missionResultToString((MAV_MISSION_RESULT)missionAck.type)));
            _finishTransaction(false);
        }
        break;
    case AckMissionClearAll:
        // MISSION_ACK expected
        if (missionAck.type != MAV_MISSION_ACCEPTED) {
            _sendError(VehicleError, QString("Vehicle returned error: %1. Vehicle remove all failed.").arg(_missionResultToString((MAV_MISSION_RESULT)missionAck.type)));
        }
        _finishTransaction(missionAck.type == MAV_MISSION_ACCEPTED);
        break;
    case AckGuidedItem:
        // MISSION_REQUEST is expected, or MISSION_ACK to end sequence
        if (missionAck.type == MAV_MISSION_ACCEPTED) {
            qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionAck %1 guided mode item accepted").arg(_planTypeString());
            _finishTransaction(true);
        } else {
            _sendError(VehicleError, QString("Vehicle returned error: %1. %2Vehicle did not accept guided item.").arg(_missionResultToString((MAV_MISSION_RESULT)missionAck.type)));
            _finishTransaction(false);
        }
        break;
    }
}
/// Called when a new mavlink message for out vehicle is received
void PlanManager::_mavlinkMessageReceived(const mavlink_message_t& message)
{
    switch (message.msgid) {
    case MAVLINK_MSG_ID_MISSION_COUNT:
        _handleMissionCount(message);
        break;

    case MAVLINK_MSG_ID_MISSION_ITEM:
        _handleMissionItem(message, false /* missionItemInt */);
        break;

    case MAVLINK_MSG_ID_MISSION_ITEM_INT:
        _handleMissionItem(message, true /* missionItemInt */);
        break;

    case MAVLINK_MSG_ID_MISSION_REQUEST:
        _handleMissionRequest(message, false /* missionItemInt */);
        break;

    case MAVLINK_MSG_ID_MISSION_REQUEST_INT:
        _handleMissionRequest(message, true /* missionItemInt */);
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

void PlanManager::_sendError(ErrorCode_t errorCode, const QString& errorMsg)
{
    qCDebug(PlanManagerLog) << QStringLiteral("Sending %1 error").arg(_planTypeString()) << errorCode << errorMsg;

    emit error(errorCode, errorMsg);
}

QString PlanManager::_ackTypeToString(AckType_t ackType)
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
        qWarning(PlanManagerLog) << QStringLiteral("Fell off end of switch statement %1").arg(_planTypeString());
        return QString("QGC Internal Error");
    }
}

QString PlanManager::_lastMissionReqestString(MAV_MISSION_RESULT result)
{
    if (_lastMissionRequest != -1 && _lastMissionRequest >= 0 && _lastMissionRequest < _writeMissionItems.count()) {
        MissionItem* item = _writeMissionItems[_lastMissionRequest];

        switch (result) {
        case MAV_MISSION_UNSUPPORTED_FRAME:
            return QString(". Frame: %1").arg(item->frame());
        case MAV_MISSION_UNSUPPORTED:
        {
            const MissionCommandUIInfo* uiInfo = qgcApp()->toolbox()->missionCommandTree()->getUIInfo(_vehicle, item->command());
            QString friendlyName;
            QString rawName;
            if (uiInfo) {
                friendlyName = uiInfo->friendlyName();
                rawName = uiInfo->rawName();
            }
            return QString(". Command: (%1, %2, %3)").arg(friendlyName).arg(rawName).arg(item->command());
        }
        case MAV_MISSION_INVALID_PARAM1:
            return QString(". Param1: %1").arg(item->param1());
        case MAV_MISSION_INVALID_PARAM2:
            return QString(". Param2: %1").arg(item->param2());
        case MAV_MISSION_INVALID_PARAM3:
            return QString(". Param3: %1").arg(item->param3());
        case MAV_MISSION_INVALID_PARAM4:
            return QString(". Param4: %1").arg(item->param4());
        case MAV_MISSION_INVALID_PARAM5_X:
            return QString(". Param5: %1").arg(item->param5());
        case MAV_MISSION_INVALID_PARAM6_Y:
            return QString(". Param6: %1").arg(item->param6());
        case MAV_MISSION_INVALID_PARAM7:
            return QString(". Param7: %1").arg(item->param7());
        case MAV_MISSION_INVALID_SEQUENCE:
            return QString(". Sequence: %1").arg(item->sequenceNumber());
        default:
            break;
        }
    }

    return QString();
}

QString PlanManager::_missionResultToString(MAV_MISSION_RESULT result)
{
    QString resultString;
    QString lastRequestString = _lastMissionReqestString(result);

    switch (result) {
    case MAV_MISSION_ACCEPTED:
        resultString = QString("Mission accepted (MAV_MISSION_ACCEPTED)");
        break;
    case MAV_MISSION_ERROR:
        resultString = QString("Unspecified error (MAV_MISSION_ERROR)");
        break;
    case MAV_MISSION_UNSUPPORTED_FRAME:
        resultString = QString("Coordinate frame is not supported (MAV_MISSION_UNSUPPORTED_FRAME)");
        break;
    case MAV_MISSION_UNSUPPORTED:
        resultString = QString("Command is not supported (MAV_MISSION_UNSUPPORTED)");
        break;
    case MAV_MISSION_NO_SPACE:
        resultString = QString("Mission item exceeds storage space (MAV_MISSION_NO_SPACE)");
        break;
    case MAV_MISSION_INVALID:
        resultString = QString("One of the parameters has an invalid value (MAV_MISSION_INVALID)");
        break;
    case MAV_MISSION_INVALID_PARAM1:
        resultString = QString("Param1 has an invalid value (MAV_MISSION_INVALID_PARAM1)");
        break;
    case MAV_MISSION_INVALID_PARAM2:
        resultString = QString("Param2 has an invalid value (MAV_MISSION_INVALID_PARAM2)");
        break;
    case MAV_MISSION_INVALID_PARAM3:
        resultString = QString("Param3 has an invalid value (MAV_MISSION_INVALID_PARAM3)");
        break;
    case MAV_MISSION_INVALID_PARAM4:
        resultString = QString("Param4 has an invalid value (MAV_MISSION_INVALID_PARAM4)");
        break;
    case MAV_MISSION_INVALID_PARAM5_X:
        resultString = QString("X/Param5 has an invalid value (MAV_MISSION_INVALID_PARAM5_X)");
        break;
    case MAV_MISSION_INVALID_PARAM6_Y:
        resultString = QString("Y/Param6 has an invalid value (MAV_MISSION_INVALID_PARAM6_Y)");
        break;
    case MAV_MISSION_INVALID_PARAM7:
        resultString = QString("Param7 has an invalid value (MAV_MISSION_INVALID_PARAM7)");
        break;
    case MAV_MISSION_INVALID_SEQUENCE:
        resultString = QString("Received mission item out of sequence (MAV_MISSION_INVALID_SEQUENCE)");
        break;
    case MAV_MISSION_DENIED:
        resultString = QString("Not accepting any mission commands (MAV_MISSION_DENIED)");
        break;
    default:
        qWarning(PlanManagerLog) << QStringLiteral("Fell off end of switch statement %1").arg(_planTypeString());
        resultString = QString("QGC Internal Error");
    }

    return resultString + lastRequestString;
}

void PlanManager::_finishTransaction(bool success)
{
    emit progressPct(1);

    _itemIndicesToRead.clear();
    _itemIndicesToWrite.clear();

    // First thing we do is clear the transaction. This way inProgesss is off when we signal transaction complete.
    TransactionType_t currentTransactionType = _transactionInProgress;
    _transactionInProgress = TransactionNone;
    if (currentTransactionType != TransactionNone) {
        _transactionInProgress = TransactionNone;
        qCDebug(PlanManagerLog) << QStringLiteral("inProgressChanged %1").arg(_planTypeString());
        emit inProgressChanged(false);
    }

    switch (currentTransactionType) {
    case TransactionRead:
        if (!success) {
            // Read from vehicle failed, clear partial list
            _clearAndDeleteMissionItems();
        }
        emit newMissionItemsAvailable(false);
        break;
    case TransactionWrite:
        if (success) {
            // Write succeeded, update internal list to be current            
            _currentMissionIndex = -1;
            _lastCurrentIndex = -1;
            emit currentIndexChanged(-1);
            emit lastCurrentIndexChanged(-1);
            _clearAndDeleteMissionItems();
            for (int i=0; i<_writeMissionItems.count(); i++) {
                _missionItems.append(_writeMissionItems[i]);
            }
            _writeMissionItems.clear();
        } else {
            // Write failed, throw out the write list
            _clearAndDeleteWriteMissionItems();
        }
        emit sendComplete(!success /* error */);
        break;
    case TransactionRemoveAll:
        emit removeAllComplete(!success /* error */);
        break;
    default:
        break;
    }

    if (_resumeMission) {
        _resumeMission = false;
        if (success) {
            emit resumeMissionReady();
        } else {
            emit resumeMissionUploadFail();
        }
    }
}

bool PlanManager::inProgress(void) const
{
    return _transactionInProgress != TransactionNone;
}

void PlanManager::_handleMissionCurrent(const mavlink_message_t& message)
{
    mavlink_mission_current_t missionCurrent;

    mavlink_msg_mission_current_decode(&message, &missionCurrent);

    if (missionCurrent.seq != _currentMissionIndex) {
        qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionCurrent %1 currentIndex:").arg(_planTypeString()) << missionCurrent.seq;
        _currentMissionIndex = missionCurrent.seq;
        emit currentIndexChanged(_currentMissionIndex);
    }

    if (_vehicle->flightMode() == _vehicle->missionFlightMode() && _currentMissionIndex != _lastCurrentIndex) {
        qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionCurrent %1 lastCurrentIndex:").arg(_planTypeString()) << _currentMissionIndex;
        _lastCurrentIndex = _currentMissionIndex;
        emit lastCurrentIndexChanged(_lastCurrentIndex);
    }
}

void PlanManager::_removeAllWorker(void)
{
    mavlink_message_t message;

    qCDebug(PlanManagerLog) << "_removeAllWorker";

    emit progressPct(0);

    _dedicatedLink = _vehicle->priorityLink();
    mavlink_msg_mission_clear_all_pack_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                            qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                            _dedicatedLink->mavlinkChannel(),
                                            &message,
                                            _vehicle->id(),
                                            MAV_COMP_ID_MISSIONPLANNER,
                                            _planType);
    _vehicle->sendMessageOnLink(_vehicle->priorityLink(), message);
    _startAckTimeout(AckMissionClearAll);
}

void PlanManager::removeAll(void)
{
    if (inProgress()) {
        return;
    }

    qCDebug(PlanManagerLog) << QStringLiteral("removeAll").arg(_planTypeString());

    _clearAndDeleteMissionItems();

    _currentMissionIndex = -1;
    _lastCurrentIndex = -1;
    emit currentIndexChanged(-1);
    emit lastCurrentIndexChanged(-1);

    _transactionInProgress = TransactionRemoveAll;
    _retryCount = 0;
    emit inProgressChanged(true);

    _removeAllWorker();
}

void PlanManager::_clearAndDeleteMissionItems(void)
{
    for (int i=0; i<_missionItems.count(); i++) {
        _missionItems[i]->deleteLater();
    }
    _missionItems.clear();
}


void PlanManager::_clearAndDeleteWriteMissionItems(void)
{
    for (int i=0; i<_writeMissionItems.count(); i++) {
        _writeMissionItems[i]->deleteLater();
    }
    _writeMissionItems.clear();
}

QString PlanManager::_planTypeString(void)
{
    switch (_planType) {
    case MAV_MISSION_TYPE_MISSION:
        return QStringLiteral("T:Mission");
    case MAV_MISSION_TYPE_FENCE:
        return QStringLiteral("T:GeoFence");
    case MAV_MISSION_TYPE_RALLY:
        return QStringLiteral("T:Rally");
    default:
        qWarning() << "Unknown plan type" << _planType;
        return QStringLiteral("T:Unknown");
    }
}
