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
    , _expectedAck(AckNone)
    , _transactionInProgress(TransactionNone)
    , _resumeMission(false)
    , _lastMissionRequest(-1)
    , _currentMissionIndex(-1)
    , _lastCurrentIndex(-1)
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

void MissionManager::_writeMissionItemsWorker(void)
{
    _lastMissionRequest = -1;

    emit progressPct(0);

    qCDebug(MissionManagerLog) << "writeMissionItems count:" << _missionItems.count();

    // Prime write list
    for (int i=0; i<_missionItems.count(); i++) {
        _itemIndicesToWrite << i;
    }

    _transactionInProgress = TransactionWrite;
    _retryCount = 0;
    emit inProgressChanged(true);
    _writeMissionCount();

    _currentMissionIndex = -1;
    _lastCurrentIndex = -1;
    emit currentIndexChanged(-1);
    emit lastCurrentIndexChanged(-1);
}


void MissionManager::writeMissionItems(const QList<MissionItem*>& missionItems)
{
    if (_vehicle->isOfflineEditingVehicle()) {
        return;
    }

    if (inProgress()) {
        qCDebug(MissionManagerLog) << "writeMissionItems called while transaction in progress";
        return;
    }

    bool skipFirstItem = !_vehicle->firmwarePlugin()->sendHomePositionToVehicle();

    _clearAndDeleteMissionItems();

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

    _writeMissionItemsWorker();
}

/// This begins the write sequence with the vehicle. This may be called during a retry.
void MissionManager::_writeMissionCount(void)
{
    qCDebug(MissionManagerLog) << "_writeMissionCount count:_retryCount" << _missionItems.count() << _retryCount;

    mavlink_message_t       message;
    mavlink_mission_count_t missionCount;

    memset(&missionCount, 0, sizeof(missionCount));
    missionCount.target_system = _vehicle->id();
    missionCount.target_component = MAV_COMP_ID_MISSIONPLANNER;
    missionCount.count = _missionItems.count();

    _dedicatedLink = _vehicle->priorityLink();
    mavlink_msg_mission_count_encode_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                          qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                          _dedicatedLink->mavlinkChannel(),
                                          &message,
                                          &missionCount);

    _vehicle->sendMessageOnLink(_dedicatedLink, message);
    _startAckTimeout(AckMissionRequest);
}

void MissionManager::writeArduPilotGuidedMissionItem(const QGeoCoordinate& gotoCoord, bool altChangeOnly)
{
    if (inProgress()) {
        qCDebug(MissionManagerLog) << "writeArduPilotGuidedMissionItem called while transaction in progress";
        return;
    }

    _transactionInProgress = TransactionWrite;

    mavlink_message_t       messageOut;
    mavlink_mission_item_t  missionItem;

    memset(&missionItem, 8, sizeof(missionItem));
    missionItem.target_system =     _vehicle->id();
    missionItem.target_component =  _vehicle->defaultComponentId();
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

    _dedicatedLink = _vehicle->priorityLink();
    mavlink_msg_mission_item_encode_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                         qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                         _dedicatedLink->mavlinkChannel(),
                                         &messageOut,
                                         &missionItem);

    _vehicle->sendMessageOnLink(_dedicatedLink, messageOut);
    _startAckTimeout(AckGuidedItem);
    emit inProgressChanged(true);
}

void MissionManager::loadFromVehicle(void)
{
    if (_vehicle->isOfflineEditingVehicle()) {
        return;
    }

    qCDebug(MissionManagerLog) << "loadFromVehicle read sequence";

    if (inProgress()) {
        qCDebug(MissionManagerLog) << "loadFromVehicle called while transaction in progress";
        return;
    }

    _retryCount = 0;
    _transactionInProgress = TransactionRead;
    emit inProgressChanged(true);
    _requestList();
}

/// Internal call to request list of mission items. May be called during a retry sequence.
void MissionManager::_requestList(void)
{
    qCDebug(MissionManagerLog) << "_requestList retry count" << _retryCount;

    mavlink_message_t               message;
    mavlink_mission_request_list_t  request;

    memset(&request, 0, sizeof(request));

    _itemIndicesToRead.clear();
    _clearMissionItems();

    request.target_system = _vehicle->id();
    request.target_component = MAV_COMP_ID_MISSIONPLANNER;

    _dedicatedLink = _vehicle->priorityLink();
    mavlink_msg_mission_request_list_encode_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                                 qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                                 _dedicatedLink->mavlinkChannel(),
                                                 &message,
                                                 &request);

    _vehicle->sendMessageOnLink(_dedicatedLink, message);
    _startAckTimeout(AckMissionCount);
}

void MissionManager::_ackTimeout(void)
{
    if (_expectedAck == AckNone) {
        return;
    }

    switch (_expectedAck) {
    case AckNone:
        qCWarning(MissionManagerLog) << "_ackTimeout timeout with AckNone";
        _sendError(InternalError, "Internal error occurred during Mission Item communication: _ackTimeOut:_expectedAck == AckNone");
        break;
    case AckMissionCount:
        // MISSION_COUNT message expected
        if (_retryCount > _maxRetryCount) {
            _sendError(VehicleError, QStringLiteral("Mission request list failed, maximum retries exceeded."));
            _finishTransaction(false);
        } else {
            _retryCount++;
            qCDebug(MissionManagerLog) << "Retrying REQUEST_LIST retry Count" << _retryCount;
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
            qCDebug(MissionManagerLog) << "Retrying MISSION_REQUEST retry Count" << _retryCount;
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
                qCDebug(MissionManagerLog) << "Retrying MISSION_COUNT retry Count" << _retryCount;
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
            qCDebug(MissionManagerLog) << "Retrying MISSION_CLEAR_ALL retry Count" << _retryCount;
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

void MissionManager::_startAckTimeout(AckType_t ack)
{
    _expectedAck = ack;
    _ackTimeoutTimer->start();
}

/// Checks the received ack against the expected ack. If they match the ack timeout timer will be stopped.
/// @return true: received ack matches expected ack
bool MissionManager::_checkForExpectedAck(AckType_t receivedAck)
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
            qCDebug(MissionManagerLog) << QString("Out of sequence ack expected:received %1:%2").arg(_ackTypeToString(_expectedAck)).arg(_ackTypeToString(receivedAck));
        }
        return false;
    }
}

void MissionManager::_readTransactionComplete(void)
{
    qCDebug(MissionManagerLog) << "_readTransactionComplete read sequence complete";
    
    mavlink_message_t       message;
    mavlink_mission_ack_t   missionAck;

    memset(&missionAck, 0, sizeof(missionAck));
    
    missionAck.target_system =      _vehicle->id();
    missionAck.target_component =   MAV_COMP_ID_MISSIONPLANNER;
    missionAck.type =               MAV_MISSION_ACCEPTED;
    
    mavlink_msg_mission_ack_encode_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                        qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                        _dedicatedLink->mavlinkChannel(),
                                        &message,
                                        &missionAck);
    
    _vehicle->sendMessageOnLink(_dedicatedLink, message);

    _finishTransaction(true);
}

void MissionManager::_handleMissionCount(const mavlink_message_t& message)
{
    mavlink_mission_count_t missionCount;
    
    if (!_checkForExpectedAck(AckMissionCount)) {
        return;
    }

    _retryCount = 0;
    
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
    if (_itemIndicesToRead.count() == 0) {
        _sendError(InternalError, "Internal Error: Call to Vehicle _requestNextMissionItem with no more indices to read");
        return;
    }

    qCDebug(MissionManagerLog) << "_requestNextMissionItem sequenceNumber:retry" << _itemIndicesToRead[0] << _retryCount;

    mavlink_message_t message;
    if (_vehicle->supportsMissionItemInt()) {
        mavlink_mission_request_int_t missionRequest;

        memset(&missionRequest, 0, sizeof(missionRequest));
        missionRequest.target_system =      _vehicle->id();
        missionRequest.target_component =   MAV_COMP_ID_MISSIONPLANNER;
        missionRequest.seq =                _itemIndicesToRead[0];

        mavlink_msg_mission_request_int_encode_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                                    qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                                    _dedicatedLink->mavlinkChannel(),
                                                    &message,
                                                    &missionRequest);
    } else {
        mavlink_mission_request_t missionRequest;

        memset(&missionRequest, 0, sizeof(missionRequest));
        missionRequest.target_system =      _vehicle->id();
        missionRequest.target_component =   MAV_COMP_ID_MISSIONPLANNER;
        missionRequest.seq =                _itemIndicesToRead[0];

        mavlink_msg_mission_request_encode_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                                qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                                _dedicatedLink->mavlinkChannel(),
                                                &message,
                                                &missionRequest);
    }
    
    _vehicle->sendMessageOnLink(_dedicatedLink, message);
    _startAckTimeout(AckMissionItem);
}

void MissionManager::_handleMissionItem(const mavlink_message_t& message, bool missionItemInt)
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
        if (_vehicle->apmFirmware() && seq ==  0) {
            ardupilotHomePositionUpdate = true;
        } else {
            qCDebug(MissionManagerLog) << "_handleMissionItem dropping spurious item seq:command:current" << seq << command << isCurrentItem;
            return;
        }
    }

    qCDebug(MissionManagerLog) << "_handleMissionItem seq:command:current:ardupilotHomePositionUpdate" << seq << command << isCurrentItem << ardupilotHomePositionUpdate;

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
        qCDebug(MissionManagerLog) << "_handleMissionItem mission item received item index which was not requested, disregrarding:" << seq;
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

void MissionManager::_clearMissionItems(void)
{
    _itemIndicesToRead.clear();
    _clearAndDeleteMissionItems();
}

void MissionManager::_handleMissionRequest(const mavlink_message_t& message, bool missionItemInt)
{
    mavlink_mission_request_t missionRequest;
    
    if (!_checkForExpectedAck(AckMissionRequest)) {
        return;
    }
    
    mavlink_msg_mission_request_decode(&message, &missionRequest);
    qCDebug(MissionManagerLog) << "_handleMissionRequest sequenceNumber" << missionRequest.seq;

    if (missionRequest.seq > _missionItems.count() - 1) {
        _sendError(RequestRangeError, QString("Vehicle requested item outside range, count:request %1:%2. Send to Vehicle failed.").arg(_missionItems.count()).arg(missionRequest.seq));
        _finishTransaction(false);
        return;
    }

    emit progressPct((double)missionRequest.seq / (double)_missionItems.count());

    _lastMissionRequest = missionRequest.seq;
    if (!_itemIndicesToWrite.contains(missionRequest.seq)) {
        qCDebug(MissionManagerLog) << "_handleMissionRequest sequence number requested which has already been sent, sending again:" << missionRequest.seq;
    } else {
        _itemIndicesToWrite.removeOne(missionRequest.seq);
    }
    
    MissionItem* item = _missionItems[missionRequest.seq];
    qCDebug(MissionManagerLog) << "_handleMissionRequest sequenceNumber:command" << missionRequest.seq << item->command();

    mavlink_message_t   messageOut;
    if (missionItemInt) {
        mavlink_mission_item_int_t missionItem;

        memset(&missionItem, 0, sizeof(missionItem));
        missionItem.target_system =     _vehicle->id();
        missionItem.target_component =  MAV_COMP_ID_MISSIONPLANNER;
        missionItem.seq =               missionRequest.seq;
        missionItem.command =           item->command();
        missionItem.param1 =            item->param1();
        missionItem.param2 =            item->param2();
        missionItem.param3 =            item->param3();
        missionItem.param4 =            item->param4();
        missionItem.x =                 item->param5() * qPow(10.0, 7.0);
        missionItem.y =                 item->param6() * qPow(10.0, 7.0);
        missionItem.z =                 item->param7();
        missionItem.frame =             item->frame();
        missionItem.current =           missionRequest.seq == 0;
        missionItem.autocontinue =      item->autoContinue();

        mavlink_msg_mission_item_int_encode_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                                 qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                                 _dedicatedLink->mavlinkChannel(),
                                                 &messageOut,
                                                 &missionItem);
    } else {
        mavlink_mission_item_t missionItem;

        memset(&missionItem, 0, sizeof(missionItem));
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

        mavlink_msg_mission_item_encode_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                             qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                             _dedicatedLink->mavlinkChannel(),
                                             &messageOut,
                                             &missionItem);
    }
    
    _vehicle->sendMessageOnLink(_dedicatedLink, messageOut);
    _startAckTimeout(AckMissionRequest);
}

void MissionManager::_handleMissionAck(const mavlink_message_t& message)
{
    mavlink_mission_ack_t missionAck;
    
    // Save the retry ack before calling _checkForExpectedAck since we'll need it to determine what
    // type of a protocol sequence we are in.
    AckType_t savedExpectedAck = _expectedAck;
    
    // We can get a MISSION_ACK with an error at any time, so if the Acks don't match it is not
    // a protocol sequence error. Call _checkForExpectedAck with _retryAck so it will succeed no
    // matter what.
    if (!_checkForExpectedAck(_expectedAck)) {
        return;
    }
    
    mavlink_msg_mission_ack_decode(&message, &missionAck);
    
    qCDebug(MissionManagerLog) << "_handleMissionAck type:" << _missionResultToString((MAV_MISSION_RESULT)missionAck.type);

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
            qCDebug(MissionManagerLog) << "_handleMissionAck guided mode item accepted";
            _finishTransaction(true);
        } else {
            _sendError(VehicleError, QString("Vehicle returned error: %1. %2Vehicle did not accept guided item.").arg(_missionResultToString((MAV_MISSION_RESULT)missionAck.type)));
            _finishTransaction(false);
        }
        break;
    }
}

void MissionManager::_handleCameraFeedback(const mavlink_message_t& message)
{
    mavlink_camera_feedback_t feedback;

    mavlink_msg_camera_feedback_decode(&message, &feedback);

    QGeoCoordinate imageCoordinate((double)feedback.lat / qPow(10.0, 7.0), (double)feedback.lng / qPow(10.0, 7.0), feedback.alt_msl);
    qCDebug(MissionManagerLog) << "_handleCameraFeedback coord:index" << imageCoordinate << feedback.img_idx;
    emit cameraFeedback(imageCoordinate, feedback.img_idx);
}

void MissionManager::_handleCameraImageCaptured(const mavlink_message_t& message)
{
    mavlink_camera_image_captured_t feedback;

    mavlink_msg_camera_image_captured_decode(&message, &feedback);

    QGeoCoordinate imageCoordinate((double)feedback.lat / qPow(10.0, 7.0), (double)feedback.lon / qPow(10.0, 7.0), feedback.alt);
    qCDebug(MissionManagerLog) << "_handleCameraFeedback coord:index" << imageCoordinate << feedback.image_index << feedback.capture_result;
    if (feedback.capture_result == 1) {
        emit cameraFeedback(imageCoordinate, feedback.image_index);
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

    case MAVLINK_MSG_ID_CAMERA_FEEDBACK:
        _handleCameraFeedback(message);
        break;

    case MAVLINK_MSG_ID_CAMERA_IMAGE_CAPTURED:
        _handleCameraImageCaptured(message);
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

QString MissionManager::_lastMissionReqestString(MAV_MISSION_RESULT result)
{
    if (_lastMissionRequest != -1 && _lastMissionRequest >= 0 && _lastMissionRequest < _missionItems.count()) {
        MissionItem* item = _missionItems[_lastMissionRequest];

        switch (result) {
        case MAV_MISSION_UNSUPPORTED_FRAME:
            return QString(". Frame: %1").arg(item->frame());
        case MAV_MISSION_UNSUPPORTED:
            return QString(". Command: %1").arg(item->command());
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

QString MissionManager::_missionResultToString(MAV_MISSION_RESULT result)
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
        qWarning(MissionManagerLog) << "Fell off end of switch statement";
        resultString = QString("QGC Internal Error");
    }

    return resultString + lastRequestString;
}

void MissionManager::_finishTransaction(bool success)
{
    emit progressPct(1);

    _itemIndicesToRead.clear();
    _itemIndicesToWrite.clear();

    // First thing we do is clear the transaction. This way inProgesss is off when we signal transaction complete.
    TransactionType_t currentTransactionType = _transactionInProgress;
    _transactionInProgress = TransactionNone;
    if (currentTransactionType != TransactionNone) {
        _transactionInProgress = TransactionNone;
        qDebug() << "inProgressChanged";
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
        emit sendComplete();
        break;
    case TransactionRemoveAll:
        emit removeAllComplete();
        break;
    default:
        break;
    }

    if (_resumeMission) {
        _resumeMission = false;
        emit resumeMissionReady();
    }
}

bool MissionManager::inProgress(void)
{
    return _transactionInProgress != TransactionNone;
}

void MissionManager::_handleMissionCurrent(const mavlink_message_t& message)
{
    mavlink_mission_current_t missionCurrent;

    mavlink_msg_mission_current_decode(&message, &missionCurrent);

    if (missionCurrent.seq != _currentMissionIndex) {
        qCDebug(MissionManagerLog) << "_handleMissionCurrent currentIndex:" << missionCurrent.seq;
        _currentMissionIndex = missionCurrent.seq;
        emit currentIndexChanged(_currentMissionIndex);
    }

    if (_vehicle->flightMode() == _vehicle->missionFlightMode() && _currentMissionIndex != _lastCurrentIndex) {
        qCDebug(MissionManagerLog) << "_handleMissionCurrent lastCurrentIndex:" << _currentMissionIndex;
        _lastCurrentIndex = _currentMissionIndex;
        emit lastCurrentIndexChanged(_lastCurrentIndex);
    }
}

void MissionManager::_removeAllWorker(void)
{
    mavlink_message_t message;

    qCDebug(MissionManagerLog) << "_removeAllWorker";

    emit progressPct(0);

    _dedicatedLink = _vehicle->priorityLink();
    mavlink_msg_mission_clear_all_pack_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                            qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                            _dedicatedLink->mavlinkChannel(),
                                            &message,
                                            _vehicle->id(),
                                            MAV_COMP_ID_MISSIONPLANNER,
                                            MAV_MISSION_TYPE_MISSION);
    _vehicle->sendMessageOnLink(_vehicle->priorityLink(), message);
    _startAckTimeout(AckMissionClearAll);
}

void MissionManager::removeAll(void)
{
    if (inProgress()) {
        return;
    }

    qCDebug(MissionManagerLog) << "removeAll";

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

void MissionManager::generateResumeMission(int resumeIndex)
{
    if (_vehicle->isOfflineEditingVehicle()) {
        return;
    }

    if (inProgress()) {
        qCDebug(MissionManagerLog) << "generateResumeMission called while transaction in progress";
        return;
    }

    for (int i=0; i<_missionItems.count(); i++) {
        MissionItem* item = _missionItems[i];
        if (item->command() == MAV_CMD_DO_JUMP) {
            qgcApp()->showMessage(tr("Unable to generate resume mission due to MAV_CMD_DO_JUMP command."));
            return;
        }
    }

    int seqNum = 0;
    QList<MissionItem*> resumeMission;

    QList<MAV_CMD> includedResumeCommands;

    // If any command in this list occurs before the resumeIndex it will be added to the front of the mission
    includedResumeCommands << MAV_CMD_DO_CONTROL_VIDEO
                           << MAV_CMD_DO_SET_ROI
                           << MAV_CMD_DO_DIGICAM_CONFIGURE
                           << MAV_CMD_DO_DIGICAM_CONTROL
                           << MAV_CMD_DO_MOUNT_CONFIGURE
                           << MAV_CMD_DO_MOUNT_CONTROL
                           << MAV_CMD_DO_SET_CAM_TRIGG_DIST
                           << MAV_CMD_DO_FENCE_ENABLE
                           << MAV_CMD_IMAGE_START_CAPTURE
                           << MAV_CMD_IMAGE_STOP_CAPTURE
                           << MAV_CMD_VIDEO_START_CAPTURE
                           << MAV_CMD_VIDEO_STOP_CAPTURE;

    bool addHomePosition = _vehicle->firmwarePlugin()->sendHomePositionToVehicle();
    int setCurrentIndex = addHomePosition ? 1 : 0;

    int resumeCommandCount = 0;
    for (int i=0; i<_missionItems.count(); i++) {
        MissionItem* oldItem = _missionItems[i];
        if ((i == 0 && addHomePosition) || i >= resumeIndex || includedResumeCommands.contains(oldItem->command())) {
            if (i < resumeIndex) {
                resumeCommandCount++;
            }
            MissionItem* newItem = new MissionItem(*oldItem, this);
            newItem->setIsCurrentItem( i == setCurrentIndex);
            newItem->setSequenceNumber(seqNum++);
            resumeMission.append(newItem);
        }
    }

    // De-dup and remove no-ops from the commands which were added to the front of the mission
    bool foundROI = false;
    bool foundCamTrigDist = false;
    QList<int> imageStartCameraIds;
    QList<int> imageStopCameraIds;
    QList<int> videoStartCameraIds;
    QList<int> videoStopCameraIds;
    while (resumeIndex >= 0) {
        MissionItem* resumeItem = resumeMission[resumeIndex];
        switch (resumeItem->command()) {
        case MAV_CMD_DO_SET_ROI:
            // Only keep the last one
            if (foundROI) {
                resumeMission.removeAt(resumeIndex);
            }
            foundROI = true;
            break;
        case MAV_CMD_DO_SET_CAM_TRIGG_DIST:
            // Only keep the last one
            if (foundCamTrigDist) {
                resumeMission.removeAt(resumeIndex);
            }
            foundCamTrigDist = true;
            break;
        case MAV_CMD_IMAGE_START_CAPTURE:
        {
            // FIXME: Handle single image capture
            int cameraId = resumeItem->param6();

            if (resumeItem->param1() == 0) {
                // This is an individual image capture command, remove it
                resumeMission.removeAt(resumeIndex);
                break;
            }
            // If we already found an image stop, then all image start/stop commands are useless
            // De-dup repeated image start commands
            // Otherwise keep only the last image start
            if (imageStopCameraIds.contains(cameraId) || imageStartCameraIds.contains(cameraId)) {
                resumeMission.removeAt(resumeIndex);
            }
            if (!imageStopCameraIds.contains(cameraId)) {
                imageStopCameraIds.append(cameraId);
            }
        }
            break;
        case MAV_CMD_IMAGE_STOP_CAPTURE:
        {
            int cameraId = resumeItem->param1();
            // Image stop only matters to kill all previous image starts
            if (!imageStopCameraIds.contains(cameraId)) {
                imageStopCameraIds.append(cameraId);
            }
            resumeMission.removeAt(resumeIndex);
        }
            break;
        case MAV_CMD_VIDEO_START_CAPTURE:
        {
            int cameraId = resumeItem->param1();
            // If we already found an video stop, then all video start/stop commands are useless
            // De-dup repeated video start commands
            // Otherwise keep only the last video start
            if (videoStopCameraIds.contains(cameraId) || videoStopCameraIds.contains(cameraId)) {
                resumeMission.removeAt(resumeIndex);
            }
            if (!videoStopCameraIds.contains(cameraId)) {
                videoStopCameraIds.append(cameraId);
            }
        }
            break;
        case MAV_CMD_VIDEO_STOP_CAPTURE:
        {
            int cameraId = resumeItem->param1();
            // Video stop only matters to kill all previous video starts
            if (!videoStopCameraIds.contains(cameraId)) {
                videoStopCameraIds.append(cameraId);
            }
            resumeMission.removeAt(resumeIndex);
        }
            break;
        default:
            break;
        }

        resumeIndex--;
    }

    // Send to vehicle
    _clearAndDeleteMissionItems();
    for (int i=0; i<resumeMission.count(); i++) {
        _missionItems.append(new MissionItem(*resumeMission[i], this));
    }
    _resumeMission = true;
    _writeMissionItemsWorker();

    // Clean up no longer needed resume items
    for (int i=0; i<resumeMission.count(); i++) {
        resumeMission[i]->deleteLater();
    }
}

void MissionManager::_clearAndDeleteMissionItems(void)
{
    for (int i=0; i<_missionItems.count(); i++) {
        _missionItems[i]->deleteLater();
    }
    _missionItems.clear();
}
