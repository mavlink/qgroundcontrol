#include "FTPDeleteStateMachine.h"
#include "QGCLoggingCategory.h"

Q_DECLARE_LOGGING_CATEGORY(FTPStateMachineLog)

FTPDeleteStateMachine::FTPDeleteStateMachine(Vehicle* vehicle, QObject* parent)
    : FTPOperationStateMachine("FTPDelete", vehicle, MAV_COMP_ID_AUTOPILOT1, parent)
{
    _buildStateMachine();
}

void FTPDeleteStateMachine::_buildStateMachine()
{
    _idleState = new QGCState("Idle", this);
    _deletingState = new QGCState("Deleting", this);
    _completeState = new QGCFinalState("Complete", this);
    _errorState = new QGCState("Error", this);

    setInitialState(_idleState);

    // State entry handlers
    connect(_deletingState, &QAbstractState::entered, this, [this]() {
        qCDebug(FTPStateMachineLog) << "Deleting file:" << _pathToDelete;
        _startDelete();
    });

    connect(_completeState, &QAbstractState::entered, this, [this]() {
        qCDebug(FTPStateMachineLog) << "Delete complete:" << _pathToDelete;
        emit deleteComplete(_pathToDelete, QString());
        completeOperation();
    });

    connect(_errorState, &QAbstractState::entered, this, [this]() {
        qCWarning(FTPStateMachineLog) << "Delete failed:" << _pathToDelete;
    });

    // Transitions
    _idleState->addTransition(new MachineEventTransition("start", _deletingState));
    _deletingState->addTransition(new MachineEventTransition("advance", _completeState));
    _deletingState->addTransition(new MachineEventTransition("error", _errorState));
}

bool FTPDeleteStateMachine::deleteFile(uint8_t compId, const QString& path)
{
    if (_operationInProgress) {
        qCWarning(FTPStateMachineLog) << "Cannot delete - operation already in progress";
        return false;
    }

    _compId = (compId == MAV_COMP_ID_ALL) ? MAV_COMP_ID_AUTOPILOT1 : compId;
    _pathToDelete = path;
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

void FTPDeleteStateMachine::cancel()
{
    if (!_operationInProgress) {
        return;
    }

    QString path = _pathToDelete;
    completeOperation(tr("Aborted"));
    emit deleteComplete(path, tr("Aborted"));
}

void FTPDeleteStateMachine::_startDelete()
{
    MavlinkFTP::Request request{};
    request.hdr.session = 0;
    request.hdr.opcode = MavlinkFTP::kCmdRemoveFile;
    request.hdr.offset = 0;
    request.hdr.size = 0;
    fillRequestDataWithString(&request, _pathToDelete);
    sendRequest(&request);
}

void FTPDeleteStateMachine::onAckReceived(const MavlinkFTP::Request* ack)
{
    MavlinkFTP::OpCode_t reqOpCode = static_cast<MavlinkFTP::OpCode_t>(ack->hdr.req_opcode);
    if (reqOpCode != MavlinkFTP::kCmdRemoveFile) {
        qCDebug(FTPStateMachineLog) << "Ignoring ACK for wrong opcode:" << MavlinkFTP::opCodeToString(reqOpCode);
        return;
    }

    advanceToNextState();
}

void FTPDeleteStateMachine::onNakReceived(const MavlinkFTP::Request* nak)
{
    MavlinkFTP::OpCode_t reqOpCode = static_cast<MavlinkFTP::OpCode_t>(nak->hdr.req_opcode);
    if (reqOpCode != MavlinkFTP::kCmdRemoveFile) {
        return;
    }

    QString error = tr("Delete failed: %1").arg(errorMsgFromNak(nak));
    completeOperation(error);
    emit deleteComplete(_pathToDelete, error);
    postEvent("error");
}

void FTPDeleteStateMachine::onTimeout()
{
    if (shouldRetry()) {
        qCDebug(FTPStateMachineLog) << "Delete timeout, retrying:" << _retryCount;
        _startDelete();
    } else {
        QString error = tr("Delete failed: timeout");
        completeOperation(error);
        emit deleteComplete(_pathToDelete, error);
        postEvent("error");
    }
}
