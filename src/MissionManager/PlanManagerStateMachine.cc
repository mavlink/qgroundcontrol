#include "PlanManagerStateMachine.h"
#include "PlanManager.h"
#include "Vehicle.h"
#include "FirmwarePlugin.h"
#include "MAVLinkProtocol.h"
#include "MissionItem.h"
#include "MissionCommandTree.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(PlanManagerStateMachineLog, "PlanManagerStateMachine")

PlanManagerStateMachine::PlanManagerStateMachine(PlanManager* planManager, QObject* parent)
    : QGCStateMachine("PlanManager", planManager->_vehicle, parent)
    , _planManager(planManager)
{
    _ackTimeoutTimer = new QTimer(this);
    _ackTimeoutTimer->setSingleShot(true);
    connect(_ackTimeoutTimer, &QTimer::timeout, this, &PlanManagerStateMachine::_ackTimeout);

    _buildStateMachine();
    _setupTransitions();
}

PlanManagerStateMachine::~PlanManagerStateMachine()
{
    _clearMissionItems();
    _clearWriteMissionItems();
}

void PlanManagerStateMachine::_buildStateMachine()
{
    // Create idle state
    _idleState = new QGCState("Idle", this);
    setInitialState(_idleState);

    // Read transaction states
    _requestListState = new QGCState("RequestList", this);
    _waitingForCountState = new QGCState("WaitingForCount", this);
    _requestItemsState = new QGCState("RequestItems", this);
    _waitingForItemState = new QGCState("WaitingForItem", this);
    _sendReadAckState = new QGCState("SendReadAck", this);

    // Write transaction states
    _sendCountState = new QGCState("SendCount", this);
    _waitingForRequestState = new QGCState("WaitingForRequest", this);
    _sendItemState = new QGCState("SendItem", this);
    _waitingForWriteAckState = new QGCState("WaitingForWriteAck", this);

    // Remove all states
    _sendClearAllState = new QGCState("SendClearAll", this);
    _waitingForClearAckState = new QGCState("WaitingForClearAck", this);

    // Final states
    _readCompleteState = new QGCFinalState("ReadComplete", this);
    _writeCompleteState = new QGCFinalState("WriteComplete", this);
    _clearCompleteState = new QGCFinalState("ClearComplete", this);
    _errorState = new QGCFinalState("Error", this);

    // Configure state entry callbacks
    connect(_idleState, &QAbstractState::entered, this, [this]() {
        qCDebug(PlanManagerStateMachineLog) << "Entered Idle state";
        _stopAckTimeout();
        _currentTransaction = TransactionType::None;
    });

    connect(_requestListState, &QAbstractState::entered, this, [this]() {
        qCDebug(PlanManagerStateMachineLog) << "Entered RequestList state";
        _sendRequestList();
    });

    connect(_waitingForCountState, &QAbstractState::entered, this, [this]() {
        qCDebug(PlanManagerStateMachineLog) << "Entered WaitingForCount state";
        _startAckTimeout(_ackTimeoutMilliseconds);
    });

    connect(_requestItemsState, &QAbstractState::entered, this, [this]() {
        qCDebug(PlanManagerStateMachineLog) << "Entered RequestItems state";
        if (_itemIndicesToRead.isEmpty()) {
            // No items to read, complete immediately
            postEvent("all_items_read");
        } else {
            _sendMissionRequest(_itemIndicesToRead.first());
        }
    });

    connect(_waitingForItemState, &QAbstractState::entered, this, [this]() {
        qCDebug(PlanManagerStateMachineLog) << "Entered WaitingForItem state";
        _startAckTimeout(_retryTimeoutMilliseconds);
    });

    connect(_sendReadAckState, &QAbstractState::entered, this, [this]() {
        qCDebug(PlanManagerStateMachineLog) << "Entered SendReadAck state";
        _sendMissionAck();
        _finishTransaction(true);
        postEvent("ack_sent");
    });

    connect(_sendCountState, &QAbstractState::entered, this, [this]() {
        qCDebug(PlanManagerStateMachineLog) << "Entered SendCount state";
        _sendMissionCount();
    });

    connect(_waitingForRequestState, &QAbstractState::entered, this, [this]() {
        qCDebug(PlanManagerStateMachineLog) << "Entered WaitingForRequest state";
        _startAckTimeout(_ackTimeoutMilliseconds);
    });

    connect(_sendItemState, &QAbstractState::entered, this, [this]() {
        qCDebug(PlanManagerStateMachineLog) << "Entered SendItem state - seq:" << _lastMissionRequest;
        if (_lastMissionRequest >= 0 && _lastMissionRequest < _writeMissionItems.count()) {
            _sendMissionItem(_lastMissionRequest);
        }
    });

    connect(_waitingForWriteAckState, &QAbstractState::entered, this, [this]() {
        qCDebug(PlanManagerStateMachineLog) << "Entered WaitingForWriteAck state";
        _startAckTimeout(_ackTimeoutMilliseconds);
    });

    connect(_sendClearAllState, &QAbstractState::entered, this, [this]() {
        qCDebug(PlanManagerStateMachineLog) << "Entered SendClearAll state";
        _sendClearAll();
    });

    connect(_waitingForClearAckState, &QAbstractState::entered, this, [this]() {
        qCDebug(PlanManagerStateMachineLog) << "Entered WaitingForClearAck state";
        _startAckTimeout(_ackTimeoutMilliseconds);
    });

    connect(_readCompleteState, &QAbstractState::entered, this, [this]() {
        qCDebug(PlanManagerStateMachineLog) << "Read transaction complete";
        emit readComplete(true);
        emit transactionComplete(true);
    });

    connect(_writeCompleteState, &QAbstractState::entered, this, [this]() {
        qCDebug(PlanManagerStateMachineLog) << "Write transaction complete";
        emit writeComplete(true);
        emit transactionComplete(true);
    });

    connect(_clearCompleteState, &QAbstractState::entered, this, [this]() {
        qCDebug(PlanManagerStateMachineLog) << "Clear transaction complete";
        emit removeAllComplete(true);
        emit transactionComplete(true);
    });

    connect(_errorState, &QAbstractState::entered, this, [this]() {
        qCDebug(PlanManagerStateMachineLog) << "Transaction failed";
        emit transactionComplete(false);
    });
}

void PlanManagerStateMachine::_setupTransitions()
{
    // Start read transaction: Idle -> RequestList
    _idleState->addTransition(new MachineEventTransition("start_read", _requestListState));

    // RequestList -> WaitingForCount (sent request)
    _requestListState->addTransition(new MachineEventTransition("request_sent", _waitingForCountState));

    // WaitingForCount -> RequestItems (received count)
    _waitingForCountState->addTransition(new MachineEventTransition("count_received", _requestItemsState));

    // WaitingForCount -> SendReadAck (count is zero)
    _waitingForCountState->addTransition(new MachineEventTransition("empty_count", _sendReadAckState));

    // RequestItems -> WaitingForItem (request sent)
    _requestItemsState->addTransition(new MachineEventTransition("request_sent", _waitingForItemState));

    // RequestItems -> SendReadAck (no more items)
    _requestItemsState->addTransition(new MachineEventTransition("all_items_read", _sendReadAckState));

    // WaitingForItem -> RequestItems (item received, more to go)
    _waitingForItemState->addTransition(new MachineEventTransition("item_received", _requestItemsState));

    // SendReadAck -> ReadComplete
    _sendReadAckState->addTransition(new MachineEventTransition("ack_sent", _readCompleteState));

    // Start write transaction: Idle -> SendCount
    _idleState->addTransition(new MachineEventTransition("start_write", _sendCountState));

    // SendCount -> WaitingForRequest (count sent)
    _sendCountState->addTransition(new MachineEventTransition("count_sent", _waitingForRequestState));

    // WaitingForRequest -> SendItem (request received)
    _waitingForRequestState->addTransition(new MachineEventTransition("request_received", _sendItemState));

    // WaitingForRequest -> WriteComplete (ack received, all items sent)
    _waitingForRequestState->addTransition(new MachineEventTransition("write_ack_received", _writeCompleteState));

    // SendItem -> WaitingForRequest (item sent)
    _sendItemState->addTransition(new MachineEventTransition("item_sent", _waitingForRequestState));

    // Start remove all transaction: Idle -> SendClearAll
    _idleState->addTransition(new MachineEventTransition("start_clear", _sendClearAllState));

    // SendClearAll -> WaitingForClearAck (clear sent)
    _sendClearAllState->addTransition(new MachineEventTransition("clear_sent", _waitingForClearAckState));

    // WaitingForClearAck -> ClearComplete (ack received)
    _waitingForClearAckState->addTransition(new MachineEventTransition("clear_ack_received", _clearCompleteState));

    // Error transitions from any waiting state
    _waitingForCountState->addTransition(new MachineEventTransition("error", _errorState));
    _waitingForItemState->addTransition(new MachineEventTransition("error", _errorState));
    _waitingForRequestState->addTransition(new MachineEventTransition("error", _errorState));
    _waitingForWriteAckState->addTransition(new MachineEventTransition("error", _errorState));
    _waitingForClearAckState->addTransition(new MachineEventTransition("error", _errorState));

    // Cancel transitions
    _waitingForCountState->addTransition(new MachineEventTransition("cancel", _idleState));
    _waitingForItemState->addTransition(new MachineEventTransition("cancel", _idleState));
    _requestItemsState->addTransition(new MachineEventTransition("cancel", _idleState));
    _waitingForRequestState->addTransition(new MachineEventTransition("cancel", _idleState));
    _sendItemState->addTransition(new MachineEventTransition("cancel", _idleState));
    _waitingForClearAckState->addTransition(new MachineEventTransition("cancel", _idleState));
}

void PlanManagerStateMachine::startRead()
{
    if (inProgress()) {
        qCDebug(PlanManagerStateMachineLog) << "Cannot start read, transaction in progress";
        return;
    }

    qCDebug(PlanManagerStateMachineLog) << "Starting read transaction";

    _retryCount = 0;
    _currentTransaction = TransactionType::Read;
    _itemIndicesToRead.clear();
    _clearMissionItems();

    emit progressChanged(0);

    if (!isRunning()) {
        start();
    }

    postEvent("start_read");
}

void PlanManagerStateMachine::startWrite(const QList<MissionItem*>& missionItems)
{
    if (inProgress()) {
        qCDebug(PlanManagerStateMachineLog) << "Cannot start write, transaction in progress";
        return;
    }

    qCDebug(PlanManagerStateMachineLog) << "Starting write transaction, items:" << missionItems.count();

    _retryCount = 0;
    _lastMissionRequest = -1;
    _currentTransaction = TransactionType::Write;

    _clearWriteMissionItems();

    // Take ownership of mission items
    for (MissionItem* item : missionItems) {
        _writeMissionItems.append(item);
    }

    // Prime write list
    _itemIndicesToWrite.clear();
    for (int i = 0; i < _writeMissionItems.count(); i++) {
        _itemIndicesToWrite << i;
    }

    emit progressChanged(0);

    if (!isRunning()) {
        start();
    }

    postEvent("start_write");
}

void PlanManagerStateMachine::startRemoveAll()
{
    if (inProgress()) {
        qCDebug(PlanManagerStateMachineLog) << "Cannot start remove all, transaction in progress";
        return;
    }

    qCDebug(PlanManagerStateMachineLog) << "Starting remove all transaction";

    _retryCount = 0;
    _currentTransaction = TransactionType::RemoveAll;

    emit progressChanged(0);

    if (!isRunning()) {
        start();
    }

    postEvent("start_clear");
}

void PlanManagerStateMachine::cancel()
{
    qCDebug(PlanManagerStateMachineLog) << "Cancelling transaction";
    _stopAckTimeout();
    postEvent("cancel");
}

bool PlanManagerStateMachine::inProgress() const
{
    return _currentTransaction != TransactionType::None;
}

void PlanManagerStateMachine::handleMessage(const mavlink_message_t& message)
{
    if (!inProgress()) {
        return;
    }

    switch (message.msgid) {
    case MAVLINK_MSG_ID_MISSION_COUNT:
        _handleMissionCount(message);
        break;
    case MAVLINK_MSG_ID_MISSION_ITEM_INT:
        _handleMissionItem(message);
        break;
    case MAVLINK_MSG_ID_MISSION_REQUEST:
    case MAVLINK_MSG_ID_MISSION_REQUEST_INT:
        _handleMissionRequest(message);
        break;
    case MAVLINK_MSG_ID_MISSION_ACK:
        _handleMissionAck(message);
        break;
    }
}

void PlanManagerStateMachine::_handleMissionCount(const mavlink_message_t& message)
{
    mavlink_mission_count_t missionCount;
    mavlink_msg_mission_count_decode(&message, &missionCount);

    if (missionCount.mission_type != _planManager->_planType) {
        qCDebug(PlanManagerStateMachineLog) << "Ignoring MISSION_COUNT with wrong type:" << missionCount.mission_type;
        return;
    }

    if (!isStateActive(_waitingForCountState)) {
        qCDebug(PlanManagerStateMachineLog) << "Ignoring unexpected MISSION_COUNT";
        return;
    }

    _stopAckTimeout();

    qCDebug(PlanManagerStateMachineLog) << "Received MISSION_COUNT:" << missionCount.count;

    _retryCount = 0;

    if (missionCount.count == 0) {
        postEvent("empty_count");
    } else {
        // Prime read list
        for (int i = 0; i < missionCount.count; i++) {
            _itemIndicesToRead << i;
        }
        _missionItemCountToRead = missionCount.count;
        postEvent("count_received");
    }
}

void PlanManagerStateMachine::_handleMissionItem(const mavlink_message_t& message)
{
    mavlink_mission_item_int_t missionItem;
    mavlink_msg_mission_item_int_decode(&message, &missionItem);

    if (static_cast<MAV_MISSION_TYPE>(missionItem.mission_type) != _planManager->_planType) {
        qCDebug(PlanManagerStateMachineLog) << "Ignoring MISSION_ITEM with wrong type";
        return;
    }

    if (!isStateActive(_waitingForItemState)) {
        // Handle ArduPilot home position updates
        if (_planManager->_vehicle->apmFirmware() && missionItem.seq == 0 &&
            _planManager->_planType == MAV_MISSION_TYPE_MISSION) {
            QGeoCoordinate newHomePosition(
                missionItem.frame == MAV_FRAME_MISSION ? missionItem.x : missionItem.x * 1e-7,
                missionItem.frame == MAV_FRAME_MISSION ? missionItem.y : missionItem.y * 1e-7,
                missionItem.z);
            _planManager->_vehicle->_setHomePosition(newHomePosition);
        }
        return;
    }

    _stopAckTimeout();

    int seq = missionItem.seq;

    qCDebug(PlanManagerStateMachineLog) << "Received MISSION_ITEM seq:" << seq;

    if (_itemIndicesToRead.contains(seq)) {
        _itemIndicesToRead.removeOne(seq);

        // Convert frame types
        MAV_FRAME frame = static_cast<MAV_FRAME>(missionItem.frame);
        if (frame == MAV_FRAME_GLOBAL_INT) {
            frame = MAV_FRAME_GLOBAL;
        } else if (frame == MAV_FRAME_GLOBAL_RELATIVE_ALT_INT) {
            frame = MAV_FRAME_GLOBAL_RELATIVE_ALT;
        }

        double param5 = missionItem.frame == MAV_FRAME_MISSION ? missionItem.x : missionItem.x * 1e-7;
        double param6 = missionItem.frame == MAV_FRAME_MISSION ? missionItem.y : missionItem.y * 1e-7;

        MissionItem* item = new MissionItem(
            seq,
            static_cast<MAV_CMD>(missionItem.command),
            frame,
            missionItem.param1,
            missionItem.param2,
            missionItem.param3,
            missionItem.param4,
            param5,
            param6,
            missionItem.z,
            missionItem.autocontinue,
            missionItem.current,
            _planManager);

        // Adjust DO_JUMP if home position not sent
        if (item->command() == MAV_CMD_DO_JUMP &&
            !_planManager->_vehicle->firmwarePlugin()->sendHomePositionToVehicle()) {
            item->setParam1(static_cast<int>(item->param1()) + 1);
        }

        _missionItems.append(item);

        emit progressChanged(static_cast<double>(seq) / static_cast<double>(_missionItemCountToRead));
    } else {
        qCDebug(PlanManagerStateMachineLog) << "Ignoring duplicate MISSION_ITEM seq:" << seq;
        _startAckTimeout(_retryTimeoutMilliseconds);
        return;
    }

    _retryCount = 0;
    postEvent("item_received");
}

void PlanManagerStateMachine::_handleMissionRequest(const mavlink_message_t& message)
{
    mavlink_mission_request_int_t missionRequest;
    mavlink_msg_mission_request_int_decode(&message, &missionRequest);

    if (static_cast<MAV_MISSION_TYPE>(missionRequest.mission_type) != _planManager->_planType) {
        qCDebug(PlanManagerStateMachineLog) << "Ignoring MISSION_REQUEST with wrong type";
        return;
    }

    if (!isStateActive(_waitingForRequestState)) {
        qCDebug(PlanManagerStateMachineLog) << "Ignoring unexpected MISSION_REQUEST";
        return;
    }

    _stopAckTimeout();

    int seq = missionRequest.seq;

    qCDebug(PlanManagerStateMachineLog) << "Received MISSION_REQUEST seq:" << seq;

    if (seq > _writeMissionItems.count() - 1) {
        emit errorOccurred(PlanManager::RequestRangeError,
                          tr("Vehicle requested item outside range, count:request %1:%2")
                              .arg(_writeMissionItems.count()).arg(seq));
        postEvent("error");
        return;
    }

    emit progressChanged(static_cast<double>(seq) / static_cast<double>(_writeMissionItems.count()));

    _lastMissionRequest = seq;

    if (_itemIndicesToWrite.contains(seq)) {
        _itemIndicesToWrite.removeOne(seq);
    } else {
        qCDebug(PlanManagerStateMachineLog) << "Re-sending item:" << seq;
    }

    postEvent("request_received");
}

void PlanManagerStateMachine::_handleMissionAck(const mavlink_message_t& message)
{
    mavlink_mission_ack_t missionAck;
    mavlink_msg_mission_ack_decode(&message, &missionAck);

    if (missionAck.mission_type != _planManager->_planType) {
        qCDebug(PlanManagerStateMachineLog) << "Ignoring MISSION_ACK with wrong type";
        return;
    }

    // ArduPilot can send bogus MAV_MISSION_INVALID_SEQUENCE
    if (_planManager->_vehicle->apmFirmware() && missionAck.type == MAV_MISSION_INVALID_SEQUENCE) {
        qCDebug(PlanManagerStateMachineLog) << "Ignoring ArduPilot MAV_MISSION_INVALID_SEQUENCE";
        return;
    }

    _stopAckTimeout();

    qCDebug(PlanManagerStateMachineLog) << "Received MISSION_ACK type:" << missionAck.type;

    if (isStateActive(_waitingForClearAckState)) {
        if (missionAck.type == MAV_MISSION_ACCEPTED) {
            _finishTransaction(true);
            postEvent("clear_ack_received");
        } else {
            emit errorOccurred(PlanManager::VehicleAckError,
                              tr("Vehicle remove all failed: %1").arg(missionAck.type));
            _finishTransaction(false);
            postEvent("error");
        }
    } else if (isStateActive(_waitingForRequestState)) {
        // Write complete
        if (missionAck.type == MAV_MISSION_ACCEPTED) {
            if (_itemIndicesToWrite.isEmpty()) {
                _finishTransaction(true);
                postEvent("write_ack_received");
            } else {
                emit errorOccurred(PlanManager::MissingRequestsError,
                                  tr("Vehicle did not request all items"));
                _finishTransaction(false);
                postEvent("error");
            }
        } else {
            emit errorOccurred(PlanManager::VehicleAckError,
                              tr("Vehicle returned error: %1").arg(missionAck.type));
            _finishTransaction(false);
            postEvent("error");
        }
    }
}

void PlanManagerStateMachine::_sendRequestList()
{
    qCDebug(PlanManagerStateMachineLog) << "Sending MISSION_REQUEST_LIST retry:" << _retryCount;

    SharedLinkInterfacePtr sharedLink = _planManager->_vehicle->vehicleLinkManager()->primaryLink().lock();
    if (sharedLink) {
        mavlink_message_t message;
        mavlink_msg_mission_request_list_pack_chan(
            MAVLinkProtocol::instance()->getSystemId(),
            MAVLinkProtocol::getComponentId(),
            sharedLink->mavlinkChannel(),
            &message,
            _planManager->_vehicle->id(),
            MAV_COMP_ID_AUTOPILOT1,
            _planManager->_planType);
        _planManager->_vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), message);
    }

    postEvent("request_sent");
}

void PlanManagerStateMachine::_sendMissionCount()
{
    qCDebug(PlanManagerStateMachineLog) << "Sending MISSION_COUNT:" << _writeMissionItems.count() << "retry:" << _retryCount;

    SharedLinkInterfacePtr sharedLink = _planManager->_vehicle->vehicleLinkManager()->primaryLink().lock();
    if (sharedLink) {
        mavlink_message_t message;
        mavlink_msg_mission_count_pack_chan(
            MAVLinkProtocol::instance()->getSystemId(),
            MAVLinkProtocol::getComponentId(),
            sharedLink->mavlinkChannel(),
            &message,
            _planManager->_vehicle->id(),
            MAV_COMP_ID_AUTOPILOT1,
            _writeMissionItems.count(),
            _planManager->_planType,
            0);
        _planManager->_vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), message);
    }

    postEvent("count_sent");
}

void PlanManagerStateMachine::_sendMissionItem(int seq)
{
    if (seq < 0 || seq >= _writeMissionItems.count()) {
        qCWarning(PlanManagerStateMachineLog) << "Invalid sequence for send:" << seq;
        return;
    }

    MissionItem* item = _writeMissionItems[seq];

    qCDebug(PlanManagerStateMachineLog) << "Sending MISSION_ITEM seq:" << seq << "cmd:" << item->command();

    SharedLinkInterfacePtr sharedLink = _planManager->_vehicle->vehicleLinkManager()->primaryLink().lock();
    if (sharedLink) {
        mavlink_message_t message;
        mavlink_msg_mission_item_int_pack_chan(
            MAVLinkProtocol::instance()->getSystemId(),
            MAVLinkProtocol::getComponentId(),
            sharedLink->mavlinkChannel(),
            &message,
            _planManager->_vehicle->id(),
            MAV_COMP_ID_AUTOPILOT1,
            seq,
            item->frame(),
            item->command(),
            seq == 0,
            item->autoContinue(),
            item->param1(),
            item->param2(),
            item->param3(),
            item->param4(),
            item->frame() == MAV_FRAME_MISSION ? item->param5() : item->param5() * 1e7,
            item->frame() == MAV_FRAME_MISSION ? item->param6() : item->param6() * 1e7,
            item->param7(),
            _planManager->_planType);
        _planManager->_vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), message);
    }

    postEvent("item_sent");
}

void PlanManagerStateMachine::_sendMissionRequest(int seq)
{
    qCDebug(PlanManagerStateMachineLog) << "Sending MISSION_REQUEST_INT seq:" << seq << "retry:" << _retryCount;

    SharedLinkInterfacePtr sharedLink = _planManager->_vehicle->vehicleLinkManager()->primaryLink().lock();
    if (sharedLink) {
        mavlink_message_t message;
        mavlink_msg_mission_request_int_pack_chan(
            MAVLinkProtocol::instance()->getSystemId(),
            MAVLinkProtocol::getComponentId(),
            sharedLink->mavlinkChannel(),
            &message,
            _planManager->_vehicle->id(),
            MAV_COMP_ID_AUTOPILOT1,
            seq,
            _planManager->_planType);
        _planManager->_vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), message);
    }

    postEvent("request_sent");
}

void PlanManagerStateMachine::_sendMissionAck()
{
    qCDebug(PlanManagerStateMachineLog) << "Sending MISSION_ACK";

    SharedLinkInterfacePtr sharedLink = _planManager->_vehicle->vehicleLinkManager()->primaryLink().lock();
    if (sharedLink) {
        mavlink_message_t message;
        mavlink_msg_mission_ack_pack_chan(
            MAVLinkProtocol::instance()->getSystemId(),
            MAVLinkProtocol::getComponentId(),
            sharedLink->mavlinkChannel(),
            &message,
            _planManager->_vehicle->id(),
            MAV_COMP_ID_AUTOPILOT1,
            MAV_MISSION_ACCEPTED,
            _planManager->_planType,
            0);
        _planManager->_vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), message);
    }
}

void PlanManagerStateMachine::_sendClearAll()
{
    qCDebug(PlanManagerStateMachineLog) << "Sending MISSION_CLEAR_ALL retry:" << _retryCount;

    SharedLinkInterfacePtr sharedLink = _planManager->_vehicle->vehicleLinkManager()->primaryLink().lock();
    if (sharedLink) {
        mavlink_message_t message;
        mavlink_msg_mission_clear_all_pack_chan(
            MAVLinkProtocol::instance()->getSystemId(),
            MAVLinkProtocol::getComponentId(),
            sharedLink->mavlinkChannel(),
            &message,
            _planManager->_vehicle->id(),
            MAV_COMP_ID_AUTOPILOT1,
            _planManager->_planType);
        _planManager->_vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), message);
    }

    postEvent("clear_sent");
}

void PlanManagerStateMachine::_startAckTimeout(int timeoutMs)
{
    _ackTimeoutTimer->setInterval(timeoutMs);
    _ackTimeoutTimer->start();
}

void PlanManagerStateMachine::_stopAckTimeout()
{
    _ackTimeoutTimer->stop();
}

void PlanManagerStateMachine::_ackTimeout()
{
    qCDebug(PlanManagerStateMachineLog) << "Ack timeout, retry:" << _retryCount;

    if (_retryCount >= _maxRetryCount) {
        emit errorOccurred(PlanManager::MaxRetryExceeded, tr("Maximum retries exceeded"));
        _finishTransaction(false);
        postEvent("error");
        return;
    }

    _retryCount++;
    _retryCurrentAction();
}

void PlanManagerStateMachine::_retryCurrentAction()
{
    if (isStateActive(_waitingForCountState)) {
        qCDebug(PlanManagerStateMachineLog) << "Retrying REQUEST_LIST";
        _sendRequestList();
        _startAckTimeout(_ackTimeoutMilliseconds);
    } else if (isStateActive(_waitingForItemState)) {
        qCDebug(PlanManagerStateMachineLog) << "Retrying MISSION_REQUEST";
        if (!_itemIndicesToRead.isEmpty()) {
            _sendMissionRequest(_itemIndicesToRead.first());
            _startAckTimeout(_retryTimeoutMilliseconds);
        }
    } else if (isStateActive(_waitingForRequestState)) {
        if (_itemIndicesToWrite.count() == _writeMissionItems.count()) {
            // No items have been requested yet, retry MISSION_COUNT
            qCDebug(PlanManagerStateMachineLog) << "Retrying MISSION_COUNT";
            _sendMissionCount();
            _startAckTimeout(_ackTimeoutMilliseconds);
        } else {
            // Vehicle didn't request all items
            emit errorOccurred(PlanManager::MissingRequestsError,
                              tr("Vehicle did not request all items from ground station"));
            _finishTransaction(false);
            postEvent("error");
        }
    } else if (isStateActive(_waitingForClearAckState)) {
        qCDebug(PlanManagerStateMachineLog) << "Retrying MISSION_CLEAR_ALL";
        _sendClearAll();
        _startAckTimeout(_ackTimeoutMilliseconds);
    }
}

void PlanManagerStateMachine::_finishTransaction(bool success)
{
    _stopAckTimeout();
    emit progressChanged(1.0);

    _itemIndicesToRead.clear();
    _itemIndicesToWrite.clear();

    TransactionType finishedTransaction = _currentTransaction;
    _currentTransaction = TransactionType::None;

    if (!success) {
        if (finishedTransaction == TransactionType::Read) {
            _clearMissionItems();
            emit readComplete(false);
        } else if (finishedTransaction == TransactionType::Write) {
            _clearWriteMissionItems();
            emit writeComplete(false);
        } else if (finishedTransaction == TransactionType::RemoveAll) {
            emit removeAllComplete(false);
        }
    }
}

QList<MissionItem*> PlanManagerStateMachine::takeMissionItems()
{
    QList<MissionItem*> items = _missionItems;
    _missionItems.clear();
    return items;
}

QList<MissionItem*> PlanManagerStateMachine::takeWriteMissionItems()
{
    QList<MissionItem*> items = _writeMissionItems;
    _writeMissionItems.clear();
    return items;
}

void PlanManagerStateMachine::_clearMissionItems()
{
    for (MissionItem* item : _missionItems) {
        delete item;
    }
    _missionItems.clear();
}

void PlanManagerStateMachine::_clearWriteMissionItems()
{
    for (MissionItem* item : _writeMissionItems) {
        delete item;
    }
    _writeMissionItems.clear();
}
