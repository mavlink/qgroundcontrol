#include "FTPOperationStateMachine.h"
#include "Vehicle.h"
#include "MAVLinkProtocol.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(FTPStateMachineLog, "Vehicle.FTP.StateMachine")

FTPOperationStateMachine::FTPOperationStateMachine(const QString& operationName, Vehicle* vehicle,
                                                     uint8_t compId, QObject* parent)
    : QGCStateMachine(operationName, vehicle, parent)
    , _vehicle(vehicle)
    , _compId(compId == MAV_COMP_ID_ALL ? MAV_COMP_ID_AUTOPILOT1 : compId)
{
    _timeoutTimer.setSingleShot(true);
    _timeoutTimer.setInterval(TimeoutMs);
    connect(&_timeoutTimer, &QTimer::timeout, this, &FTPOperationStateMachine::_onTimeout);

    // Connect to vehicle's MAVLink message handling
    connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &FTPOperationStateMachine::_onMavlinkMessage);
}

void FTPOperationStateMachine::sendRequest(MavlinkFTP::Request* request)
{
    _timeoutTimer.start();

    SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qCWarning(FTPStateMachineLog) << "No primary link available";
        return;
    }

    request->hdr.seqNumber = _expectedSeqNumber + 1;
    _expectedSeqNumber += 2;

    qCDebug(FTPStateMachineLog) << "Sending FTP request opcode:"
                                 << MavlinkFTP::opCodeToString(static_cast<MavlinkFTP::OpCode_t>(request->hdr.opcode))
                                 << "seqNumber:" << request->hdr.seqNumber;

    mavlink_message_t message;
    mavlink_msg_file_transfer_protocol_pack_chan(
        MAVLinkProtocol::instance()->getSystemId(),
        MAVLinkProtocol::getComponentId(),
        sharedLink->mavlinkChannel(),
        &message,
        0,  // Target network
        _vehicle->id(),
        _compId,
        reinterpret_cast<uint8_t*>(request));

    _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), message);
}

void FTPOperationStateMachine::_onMavlinkMessage(const mavlink_message_t& message)
{
    if (message.msgid != MAVLINK_MSG_ID_FILE_TRANSFER_PROTOCOL) {
        return;
    }
    if (message.sysid != _vehicle->id() || message.compid != _compId) {
        return;
    }
    if (!_operationInProgress) {
        return;
    }

    handleFTPMessage(message);
}

void FTPOperationStateMachine::handleFTPMessage(const mavlink_message_t& message)
{
    mavlink_file_transfer_protocol_t data;
    mavlink_msg_file_transfer_protocol_decode(&message, &data);

    // Check target system
    int qgcId = MAVLinkProtocol::instance()->getSystemId();
    if (data.target_system != qgcId) {
        return;
    }

    MavlinkFTP::Request* request = reinterpret_cast<MavlinkFTP::Request*>(&data.payload[0]);

    // Check sequence number
    if (request->hdr.seqNumber != _expectedSeqNumber) {
        qCDebug(FTPStateMachineLog) << "Ignoring message with unexpected seqNumber:"
                                     << request->hdr.seqNumber << "expected:" << _expectedSeqNumber;
        return;
    }

    _timeoutTimer.stop();

    if (request->hdr.opcode == MavlinkFTP::kRspAck) {
        onAckReceived(request);
    } else if (request->hdr.opcode == MavlinkFTP::kRspNak) {
        onNakReceived(request);
    }
}

void FTPOperationStateMachine::_onTimeout()
{
    onTimeout();
}

QString FTPOperationStateMachine::errorMsgFromNak(const MavlinkFTP::Request* nak) const
{
    MavlinkFTP::ErrorCode_t errorCode = static_cast<MavlinkFTP::ErrorCode_t>(nak->data[0]);

    if ((errorCode == MavlinkFTP::kErrFailErrno && nak->hdr.size != 2) ||
        ((errorCode != MavlinkFTP::kErrFailErrno) && nak->hdr.size != 1)) {
        return tr("Invalid NAK format");
    }

    if (errorCode == MavlinkFTP::kErrFailErrno) {
        return tr("errno %1").arg(nak->data[1]);
    }

    return MavlinkFTP::errorCodeToString(errorCode);
}

void FTPOperationStateMachine::fillRequestDataWithString(MavlinkFTP::Request* request, const QString& str)
{
    strncpy(reinterpret_cast<char*>(&request->data[0]), str.toStdString().c_str(), sizeof(request->data));
    request->hdr.size = static_cast<uint8_t>(strnlen(reinterpret_cast<const char*>(&request->data[0]),
                                                      sizeof(request->data)));
}

void FTPOperationStateMachine::completeOperation(const QString& errorMsg)
{
    _timeoutTimer.stop();
    _operationInProgress = false;
    emit operationComplete(errorMsg);
}

void FTPOperationStateMachine::advanceToNextState()
{
    postEvent("advance");
}

bool FTPOperationStateMachine::shouldRetry()
{
    return ++_retryCount <= MaxRetries;
}
