#include "FTPListDirectoryStateMachine.h"
#include "QGCLoggingCategory.h"

Q_DECLARE_LOGGING_CATEGORY(FTPStateMachineLog)

FTPListDirectoryStateMachine::FTPListDirectoryStateMachine(Vehicle* vehicle, QObject* parent)
    : FTPOperationStateMachine("FTPListDirectory", vehicle, MAV_COMP_ID_AUTOPILOT1, parent)
{
    _buildStateMachine();
}

void FTPListDirectoryStateMachine::_buildStateMachine()
{
    _idleState = new QGCState("Idle", this);
    _listingState = new QGCState("Listing", this);
    _completeState = new QGCFinalState("Complete", this);
    _errorState = new QGCState("Error", this);

    setInitialState(_idleState);

    connect(_listingState, &QAbstractState::entered, this, [this]() {
        qCDebug(FTPStateMachineLog) << "Listing directory:" << _pathToList;
        _sendListRequest(true);
    });

    connect(_completeState, &QAbstractState::entered, this, [this]() {
        qCDebug(FTPStateMachineLog) << "List complete:" << _directoryEntries.size() << "entries";
        emit listDirectoryComplete(_directoryEntries, QString());
        completeOperation();
    });

    connect(_errorState, &QAbstractState::entered, this, [this]() {
        qCWarning(FTPStateMachineLog) << "List directory failed:" << _pathToList;
    });

    _idleState->addTransition(new MachineEventTransition("start", _listingState));
    _listingState->addTransition(new MachineEventTransition("advance", _completeState));
    _listingState->addTransition(new MachineEventTransition("error", _errorState));
}

bool FTPListDirectoryStateMachine::listDirectory(uint8_t compId, const QString& path)
{
    if (_operationInProgress) {
        qCWarning(FTPStateMachineLog) << "Cannot list - operation already in progress";
        return false;
    }

    _compId = (compId == MAV_COMP_ID_ALL) ? MAV_COMP_ID_AUTOPILOT1 : compId;
    _pathToList = path;
    _directoryEntries.clear();
    _expectedOffset = 0;
    _operationInProgress = true;
    _retryCount = 0;
    _expectedSeqNumber = 0;

    if (!isRunning()) {
        start();
    }

    QTimer::singleShot(0, this, [this]() {
        postEvent("start");
    });

    return true;
}

void FTPListDirectoryStateMachine::cancel()
{
    if (!_operationInProgress) {
        return;
    }

    completeOperation(tr("Aborted"));
    emit listDirectoryComplete(QStringList(), tr("Aborted"));
}

void FTPListDirectoryStateMachine::_sendListRequest(bool firstRequest)
{
    qCDebug(FTPStateMachineLog) << "Sending list request offset:" << _expectedOffset
                                 << "firstRequest:" << firstRequest << "retry:" << _retryCount;

    MavlinkFTP::Request request{};
    request.hdr.session = 0;
    request.hdr.opcode = MavlinkFTP::kCmdListDirectory;
    request.hdr.offset = _expectedOffset;
    request.hdr.size = sizeof(request.data);
    fillRequestDataWithString(&request, _pathToList);

    if (firstRequest) {
        resetRetryCount();
    } else {
        _expectedSeqNumber -= 2;
    }

    sendRequest(&request);
}

void FTPListDirectoryStateMachine::onAckReceived(const MavlinkFTP::Request* ack)
{
    MavlinkFTP::OpCode_t reqOpCode = static_cast<MavlinkFTP::OpCode_t>(ack->hdr.req_opcode);
    if (reqOpCode != MavlinkFTP::kCmdListDirectory) {
        return;
    }

    qCDebug(FTPStateMachineLog) << "List ACK received, size:" << ack->hdr.size;

    // Parse entries
    const char* dataPtr = reinterpret_cast<const char*>(ack->data);
    while (dataPtr < reinterpret_cast<const char*>(ack->data) + ack->hdr.size) {
        QString entry = dataPtr;
        dataPtr += entry.size() + 1;
        _directoryEntries.append(entry);
        _expectedOffset++;
    }

    // Request next batch
    _sendListRequest(true);
}

void FTPListDirectoryStateMachine::onNakReceived(const MavlinkFTP::Request* nak)
{
    MavlinkFTP::OpCode_t reqOpCode = static_cast<MavlinkFTP::OpCode_t>(nak->hdr.req_opcode);
    if (reqOpCode != MavlinkFTP::kCmdListDirectory) {
        return;
    }

    MavlinkFTP::ErrorCode_t errorCode = static_cast<MavlinkFTP::ErrorCode_t>(nak->data[0]);

    if (errorCode == MavlinkFTP::kErrEOF) {
        // All entries received
        qCDebug(FTPStateMachineLog) << "List EOF received";
        advanceToNextState();
    } else {
        QString error = tr("List directory failed: %1").arg(errorMsgFromNak(nak));
        completeOperation(error);
        emit listDirectoryComplete(QStringList(), error);
        postEvent("error");
    }
}

void FTPListDirectoryStateMachine::onTimeout()
{
    if (shouldRetry()) {
        qCDebug(FTPStateMachineLog) << "List timeout, retrying:" << retryCount();
        _sendListRequest(false);
    } else {
        QString error = tr("List directory failed: timeout");
        completeOperation(error);
        emit listDirectoryComplete(QStringList(), error);
        postEvent("error");
    }
}
