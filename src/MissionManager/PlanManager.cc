/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
    : _vehicle                  (vehicle)
    , _planType                 (planType)
    , _dedicatedLink            (nullptr)
    , _ackTimeoutTimer          (nullptr)
    , _expectedAck              (AckNone)
    , _transactionInProgress    (TransactionNone)
    , _resumeMission            (false)
    , _lastMissionRequest       (-1)
    , _missionItemCountToRead   (-1)
    , _currentMissionIndex      (-1)
    , _lastCurrentIndex         (-1)
{
    _ackTimeoutTimer = new QTimer(this);
    _ackTimeoutTimer->setSingleShot(true);

    connect(_ackTimeoutTimer, &QTimer::timeout, this, &PlanManager::_ackTimeout);
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

    _retryCount = 0;
    _setTransactionInProgress(TransactionWrite);
    _connectToMavlink();
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
        MissionItem* item = missionItems[i];
        _writeMissionItems.append(item); // PlanManager takes control of passed MissionItem

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
                                        MAV_COMP_ID_AUTOPILOT1,
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
    _setTransactionInProgress(TransactionRead);
    _connectToMavlink();
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
                                               MAV_COMP_ID_AUTOPILOT1,
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
        _sendError(InternalError, tr("Internal error occurred during Mission Item communication: _ackTimeOut:_expectedAck == AckNone"));
        break;
    case AckMissionCount:
        // MISSION_COUNT message expected
        if (_retryCount > _maxRetryCount) {
            _sendError(VehicleError, tr("Mission request list failed, maximum retries exceeded."));
            _finishTransaction(false);
        } else {
            _retryCount++;
            qCDebug(PlanManagerLog) << tr("Retrying %1 REQUEST_LIST retry Count").arg(_planTypeString()) << _retryCount;
            _requestList();
        }
        break;
    case AckMissionItem:
        // MISSION_ITEM expected
        if (_retryCount > _maxRetryCount) {
            _sendError(VehicleError, tr("Mission read failed, maximum retries exceeded."));
            _finishTransaction(false);
        } else {
            _retryCount++;
            qCDebug(PlanManagerLog) << tr("Retrying %1 MISSION_REQUEST retry Count").arg(_planTypeString()) << _retryCount;
            _requestNextMissionItem();
        }
        break;
    case AckMissionRequest:
        // MISSION_REQUEST is expected, or MISSION_ACK to end sequence
        if (_itemIndicesToWrite.count() == 0) {
            // Vehicle did not send final MISSION_ACK at end of sequence
            _sendError(VehicleError, tr("Mission write failed, vehicle failed to send final ack."));
            _finishTransaction(false);
        } else if (_itemIndicesToWrite[0] == 0) {
            // Vehicle did not respond to MISSION_COUNT, try again
            if (_retryCount > _maxRetryCount) {
                _sendError(VehicleError, tr("Mission write mission count failed, maximum retries exceeded."));
                _finishTransaction(false);
            } else {
                _retryCount++;
                qCDebug(PlanManagerLog) << QStringLiteral("Retrying %1 MISSION_COUNT retry Count").arg(_planTypeString()) << _retryCount;
                _writeMissionCount();
            }
        } else {
            // Vehicle did not request all items from ground station
            _sendError(AckTimeoutError, tr("Vehicle did not request all items from ground station: %1").arg(_ackTypeToString(_expectedAck)));
            _expectedAck = AckNone;
            _finishTransaction(false);
        }
        break;
    case AckMissionClearAll:
        // MISSION_ACK expected
        if (_retryCount > _maxRetryCount) {
            _sendError(VehicleError, tr("Mission remove all, maximum retries exceeded."));
            _finishTransaction(false);
        } else {
            _retryCount++;
            qCDebug(PlanManagerLog) << tr("Retrying %1 MISSION_CLEAR_ALL retry Count").arg(_planTypeString()) << _retryCount;
            _removeAllWorker();
        }
        break;
    case AckGuidedItem:
        // MISSION_REQUEST is expected, or MISSION_ACK to end sequence
    default:
        _sendError(AckTimeoutError, tr("Vehicle did not respond to mission item communication: %1").arg(_ackTypeToString(_expectedAck)));
        _expectedAck = AckNone;
        _finishTransaction(false);
    }
}

void PlanManager::_startAckTimeout(AckType_t ack)
{
    switch (ack) {
    case AckMissionItem:
        // We are actively trying to get the mission item, so we don't want to wait as long.
        _ackTimeoutTimer->setInterval(_retryTimeoutMilliseconds);
        break;
    case AckNone:
        // FALLTHROUGH
    case AckMissionCount:
        // FALLTHROUGH
    case AckMissionRequest:
        // FALLTHROUGH
    case AckMissionClearAll:
        // FALLTHROUGH
    case AckGuidedItem:
        _ackTimeoutTimer->setInterval(_ackTimeoutMilliseconds);
        break;
    }

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
            qCDebug(PlanManagerLog) << QString("Out of sequence ack %1 expected:received %2:%3").arg(_planTypeString().arg(_ackTypeToString(_expectedAck)).arg(_ackTypeToString(receivedAck)));
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
                                      MAV_COMP_ID_AUTOPILOT1,
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
        _missionItemCountToRead = missionCount.count;
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
                                                  MAV_COMP_ID_AUTOPILOT1,
                                                  _itemIndicesToRead[0],
                _planType);
    } else {
        mavlink_msg_mission_request_pack_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                              qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                              _dedicatedLink->mavlinkChannel(),
                                              &message,
                                              _vehicle->id(),
                                              MAV_COMP_ID_AUTOPILOT1,
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

    emit progressPct((double)seq / (double)_missionItemCountToRead);
    
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
    MAV_MISSION_TYPE    missionRequestMissionType;
    uint16_t            missionRequestSeq;

    if (missionItemInt) {
        mavlink_mission_request_int_t missionRequest;
        mavlink_msg_mission_request_int_decode(&message, &missionRequest);
        missionRequestMissionType = static_cast<MAV_MISSION_TYPE>(missionRequest.mission_type);
        missionRequestSeq = missionRequest.seq;
    } else {
        mavlink_mission_request_t missionRequest;
        mavlink_msg_mission_request_decode(&message, &missionRequest);
        missionRequestMissionType = static_cast<MAV_MISSION_TYPE>(missionRequest.mission_type);
        missionRequestSeq = missionRequest.seq;
    }

    if (missionRequestMissionType != _planType) {
        // if there was a previous transaction with a different mission_type, it can happen that we receive
        // a stale message here, for example when the MAV ran into a timeout and sent a message twice
        qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionRequest %1 Incorrect mission_type received expected:actual").arg(_planTypeString()) << _planType << missionRequestMissionType;
        return;
    }
    
    if (!_checkForExpectedAck(AckMissionRequest)) {
        return;
    }

    qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionRequest %1 sequenceNumber").arg(_planTypeString()) << missionRequestSeq;

    if (missionRequestSeq > _writeMissionItems.count() - 1) {
        _sendError(RequestRangeError, tr("Vehicle requested item outside range, count:request %1:%2. Send to Vehicle failed.").arg(_writeMissionItems.count()).arg(missionRequestSeq));
        _finishTransaction(false);
        return;
    }

    emit progressPct((double)missionRequestSeq / (double)_writeMissionItems.count());

    _lastMissionRequest = missionRequestSeq;
    if (!_itemIndicesToWrite.contains(missionRequestSeq)) {
        qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionRequest %1 sequence number requested which has already been sent, sending again:").arg(_planTypeString()) << missionRequestSeq;
    } else {
        _itemIndicesToWrite.removeOne(missionRequestSeq);
    }
    
    MissionItem* item = _writeMissionItems[missionRequestSeq];
    qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionRequest %1 sequenceNumber:command").arg(_planTypeString()) << missionRequestSeq << item->command();

    mavlink_message_t   messageOut;
    if (missionItemInt || _vehicle->apmFirmware() /* ArduPilot always expects to get MISSION_ITEM_INT no matter what */) {
        mavlink_msg_mission_item_int_pack_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                               qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                               _dedicatedLink->mavlinkChannel(),
                                               &messageOut,
                                               _vehicle->id(),
                                               MAV_COMP_ID_AUTOPILOT1,
                                               missionRequestSeq,
                                               item->frame(),
                                               item->command(),
                                               missionRequestSeq == 0,
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
                                           MAV_COMP_ID_AUTOPILOT1,
                                           missionRequestSeq,
                                           item->frame(),
                                           item->command(),
                                           missionRequestSeq == 0,
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

    if (_vehicle->apmFirmware() && missionAck.type == MAV_MISSION_INVALID_SEQUENCE) {
        // ArduPilot sends these Acks which can happen just due to noisy links causing duplicated requests being responded to.
        // As far as I'm concerned this is incorrect protocol implementation but we need to deal with it anyway. So we just
        // ignore it and if things really go haywire the timeouts will fire to fail the overall transaction.
        qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionAck ArduPilot sending possibly bogus MAV_MISSION_INVALID_SEQUENCE").arg(_planTypeString()) << _planType;
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
	qCDebug(PlanManagerLog) << QStringLiteral("Vehicle sent unexpected MISSION_ACK message, error: %1").arg(_missionResultToString((MAV_MISSION_RESULT)missionAck.type));
        break;
    case AckMissionCount:
        // MISSION_COUNT message expected
        _sendError(VehicleError, tr("Vehicle returned error: %1.").arg(_missionResultToString((MAV_MISSION_RESULT)missionAck.type)));
        _finishTransaction(false);
        break;
    case AckMissionItem:
        // MISSION_ITEM expected
        _sendError(VehicleError, tr("Vehicle returned error: %1.").arg(_missionResultToString((MAV_MISSION_RESULT)missionAck.type)));
        _finishTransaction(false);
        break;
    case AckMissionRequest:
        // MISSION_REQUEST is expected, or MISSION_ACK to end sequence
        if (missionAck.type == MAV_MISSION_ACCEPTED) {
            if (_itemIndicesToWrite.count() == 0) {
                qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionAck write sequence complete %1").arg(_planTypeString());
                _finishTransaction(true);
            } else {
                _sendError(MissingRequestsError, tr("Vehicle did not request all items during write sequence, missed count %1.").arg(_itemIndicesToWrite.count()));
                _finishTransaction(false);
            }
        } else {
            _sendError(VehicleError, tr("Vehicle returned error: %1.").arg(_missionResultToString((MAV_MISSION_RESULT)missionAck.type)));
            _finishTransaction(false);
        }
        break;
    case AckMissionClearAll:
        // MISSION_ACK expected
        if (missionAck.type != MAV_MISSION_ACCEPTED) {
            _sendError(VehicleError, tr("Vehicle returned error: %1. Vehicle remove all failed.").arg(_missionResultToString((MAV_MISSION_RESULT)missionAck.type)));
        }
        _finishTransaction(missionAck.type == MAV_MISSION_ACCEPTED);
        break;
    case AckGuidedItem:
        // MISSION_REQUEST is expected, or MISSION_ACK to end sequence
        if (missionAck.type == MAV_MISSION_ACCEPTED) {
            qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionAck %1 guided mode item accepted").arg(_planTypeString());
            _finishTransaction(true, true /* apmGuidedItemWrite */);
        } else {
            _sendError(VehicleError, tr("Vehicle returned error: %1. %2Vehicle did not accept guided item.").arg(_missionResultToString((MAV_MISSION_RESULT)missionAck.type)));
            _finishTransaction(false, true /* apmGuidedItemWrite */);
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
    if (_lastMissionRequest >= 0 && _lastMissionRequest < _writeMissionItems.count()) {
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
        resultString = tr("Mission accepted (MAV_MISSION_ACCEPTED)");
        break;
    case MAV_MISSION_ERROR:
        resultString = tr("Unspecified error (MAV_MISSION_ERROR)");
        break;
    case MAV_MISSION_UNSUPPORTED_FRAME:
        resultString = tr("Coordinate frame is not supported (MAV_MISSION_UNSUPPORTED_FRAME)");
        break;
    case MAV_MISSION_UNSUPPORTED:
        resultString = tr("Command is not supported (MAV_MISSION_UNSUPPORTED)");
        break;
    case MAV_MISSION_NO_SPACE:
        resultString = tr("Mission item exceeds storage space (MAV_MISSION_NO_SPACE)");
        break;
    case MAV_MISSION_INVALID:
        resultString = tr("One of the parameters has an invalid value (MAV_MISSION_INVALID)");
        break;
    case MAV_MISSION_INVALID_PARAM1:
        resultString = tr("Param1 has an invalid value (MAV_MISSION_INVALID_PARAM1)");
        break;
    case MAV_MISSION_INVALID_PARAM2:
        resultString = tr("Param2 has an invalid value (MAV_MISSION_INVALID_PARAM2)");
        break;
    case MAV_MISSION_INVALID_PARAM3:
        resultString = tr("Param3 has an invalid value (MAV_MISSION_INVALID_PARAM3)");
        break;
    case MAV_MISSION_INVALID_PARAM4:
        resultString = tr("Param4 has an invalid value (MAV_MISSION_INVALID_PARAM4)");
        break;
    case MAV_MISSION_INVALID_PARAM5_X:
        resultString = tr("X/Param5 has an invalid value (MAV_MISSION_INVALID_PARAM5_X)");
        break;
    case MAV_MISSION_INVALID_PARAM6_Y:
        resultString = tr("Y/Param6 has an invalid value (MAV_MISSION_INVALID_PARAM6_Y)");
        break;
    case MAV_MISSION_INVALID_PARAM7:
        resultString = tr("Param7 has an invalid value (MAV_MISSION_INVALID_PARAM7)");
        break;
    case MAV_MISSION_INVALID_SEQUENCE:
        resultString = tr("Received mission item out of sequence (MAV_MISSION_INVALID_SEQUENCE)");
        break;
    case MAV_MISSION_DENIED:
        resultString = tr("Not accepting any mission commands (MAV_MISSION_DENIED)");
        break;
    default:
        qWarning(PlanManagerLog) << QStringLiteral("Fell off end of switch statement %1").arg(_planTypeString());
        resultString = tr("QGC Internal Error");
    }

    return resultString + lastRequestString;
}

void PlanManager::_finishTransaction(bool success, bool apmGuidedItemWrite)
{
    emit progressPct(1);
    _disconnectFromMavlink();

    _itemIndicesToRead.clear();
    _itemIndicesToWrite.clear();

    // First thing we do is clear the transaction. This way inProgesss is off when we signal transaction complete.
    TransactionType_t currentTransactionType = _transactionInProgress;
    _setTransactionInProgress(TransactionNone);

    switch (currentTransactionType) {
    case TransactionRead:
        if (!success) {
            // Read from vehicle failed, clear partial list
            _clearAndDeleteMissionItems();
        }
        emit newMissionItemsAvailable(false);
        break;
    case TransactionWrite:
        // No need to do anything for ArduPilot guided go to waypoint write
        if (!apmGuidedItemWrite) {
            if (success) {
                // Write succeeded, update internal list to be current
                if (_planType == MAV_MISSION_TYPE_MISSION) {
                    _currentMissionIndex = -1;
                    _lastCurrentIndex = -1;
                    emit currentIndexChanged(-1);
                    emit lastCurrentIndexChanged(-1);
                }
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
        }
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

void PlanManager::_removeAllWorker(void)
{
    mavlink_message_t message;

    qCDebug(PlanManagerLog) << "_removeAllWorker";

    emit progressPct(0);

    _connectToMavlink();
    _dedicatedLink = _vehicle->priorityLink();
    mavlink_msg_mission_clear_all_pack_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                            qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                            _dedicatedLink->mavlinkChannel(),
                                            &message,
                                            _vehicle->id(),
                                            MAV_COMP_ID_AUTOPILOT1,
                                            _planType);
    _vehicle->sendMessageOnLink(_vehicle->priorityLink(), message);
    _startAckTimeout(AckMissionClearAll);
}

void PlanManager::removeAll(void)
{
    if (inProgress()) {
        return;
    }

    qCDebug(PlanManagerLog) << QStringLiteral("removeAll %1").arg(_planTypeString());

    _clearAndDeleteMissionItems();

    if (_planType == MAV_MISSION_TYPE_MISSION) {
        _currentMissionIndex = -1;
        _lastCurrentIndex = -1;
        emit currentIndexChanged(-1);
        emit lastCurrentIndexChanged(-1);
    }

    _retryCount = 0;
    _setTransactionInProgress(TransactionRemoveAll);

    _removeAllWorker();
}

void PlanManager::_clearAndDeleteMissionItems(void)
{
    for (int i=0; i<_missionItems.count(); i++) {
        // Using deleteLater here causes too much transient memory to stack up
        delete _missionItems[i];
    }
    _missionItems.clear();
}


void PlanManager::_clearAndDeleteWriteMissionItems(void)
{
    for (int i=0; i<_writeMissionItems.count(); i++) {
        // Using deleteLater here causes too much transient memory to stack up
        delete _writeMissionItems[i];
    }
    _writeMissionItems.clear();
}

void PlanManager::_connectToMavlink(void)
{
    connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &PlanManager::_mavlinkMessageReceived);
}

void PlanManager::_disconnectFromMavlink(void)
{
    disconnect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &PlanManager::_mavlinkMessageReceived);
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

void PlanManager::_setTransactionInProgress(TransactionType_t type)
{
    if (_transactionInProgress  != type) {
        qCDebug(PlanManagerLog) << "_setTransactionInProgress" << _planTypeString() << type;
        _transactionInProgress = type;
        emit inProgressChanged(inProgress());
    }
}
