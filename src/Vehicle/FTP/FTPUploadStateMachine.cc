#include "FTPUploadStateMachine.h"
#include "QGCLoggingCategory.h"

Q_DECLARE_LOGGING_CATEGORY(FTPStateMachineLog)

FTPUploadStateMachine::FTPUploadStateMachine(Vehicle* vehicle, QObject* parent)
    : FTPOperationStateMachine("FTPUpload", vehicle, MAV_COMP_ID_AUTOPILOT1, parent)
{
    _buildStateMachine();
}

void FTPUploadStateMachine::_buildStateMachine()
{
    _idleState = new QGCState("Idle", this);
    _creatingState = new QGCState("Creating", this);
    _writingState = new QGCState("Writing", this);
    _resettingState = new QGCState("Resetting", this);
    _completeState = new QGCFinalState("Complete", this);
    _errorState = new QGCState("Error", this);

    setInitialState(_idleState);

    connect(_creatingState, &QAbstractState::entered, this, [this]() {
        qCDebug(FTPStateMachineLog) << "Creating remote file:" << _remotePath;
        _phase = Phase::CreatingFile;
        _createFile();
    });

    connect(_writingState, &QAbstractState::entered, this, [this]() {
        qCDebug(FTPStateMachineLog) << "Starting file write";
        _phase = Phase::WritingFile;
        _writeFile(true);
    });

    connect(_resettingState, &QAbstractState::entered, this, [this]() {
        qCDebug(FTPStateMachineLog) << "Resetting sessions";
        _phase = Phase::ResettingSessions;
        _resetSessions();
    });

    connect(_completeState, &QAbstractState::entered, this, [this]() {
        qCDebug(FTPStateMachineLog) << "Upload complete:" << _remotePath;
        _file.close();
        _phase = Phase::Complete;
        emit uploadComplete(_remotePath, QString());
        completeOperation();
    });

    connect(_errorState, &QAbstractState::entered, this, [this]() {
        qCWarning(FTPStateMachineLog) << "Upload failed:" << _remotePath;
        _file.close();
    });

    // Transitions
    _idleState->addTransition(new MachineEventTransition("start", _creatingState));
    _creatingState->addTransition(new MachineEventTransition("advance", _writingState));
    _creatingState->addTransition(new MachineEventTransition("error", _errorState));
    _writingState->addTransition(new MachineEventTransition("advance", _resettingState));
    _writingState->addTransition(new MachineEventTransition("error", _errorState));
    _resettingState->addTransition(new MachineEventTransition("advance", _completeState));
    _resettingState->addTransition(new MachineEventTransition("error", _completeState)); // Complete even on reset error
}

bool FTPUploadStateMachine::upload(uint8_t compId, const QString& localPath, const QString& remotePath)
{
    if (_operationInProgress) {
        qCWarning(FTPStateMachineLog) << "Cannot upload - operation already in progress";
        return false;
    }

    _compId = (compId == MAV_COMP_ID_ALL) ? MAV_COMP_ID_AUTOPILOT1 : compId;
    _localPath = localPath;
    _remotePath = remotePath;
    _cancelled = false;

    // Reset state
    _sessionId = 0;
    _fileSize = 0;
    _totalBytesSent = 0;
    _lastChunkSize = 0;
    _phase = Phase::Idle;

    // Open local file
    _file.setFileName(localPath);
    if (!_file.open(QIODevice::ReadOnly)) {
        qCWarning(FTPStateMachineLog) << "Cannot open local file:" << localPath;
        return false;
    }
    _fileSize = static_cast<uint32_t>(_file.size());

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

void FTPUploadStateMachine::cancel()
{
    if (!_operationInProgress) {
        return;
    }

    _cancelled = true;
    _file.close();
    completeOperation(tr("Aborted"));
    emit uploadComplete(_remotePath, tr("Aborted"));
}

void FTPUploadStateMachine::_createFile()
{
    MavlinkFTP::Request request{};
    request.hdr.session = 0;
    request.hdr.opcode = MavlinkFTP::kCmdCreateFile;
    request.hdr.offset = 0;
    request.hdr.size = 0;
    fillRequestDataWithString(&request, _remotePath);
    sendRequest(&request);
}

void FTPUploadStateMachine::_writeFile(bool firstRequest)
{
    if (_totalBytesSent >= _fileSize) {
        advanceToNextState();
        return;
    }

    qCDebug(FTPStateMachineLog) << "Writing file offset:" << _totalBytesSent << "of" << _fileSize;

    MavlinkFTP::Request request{};
    request.hdr.session = _sessionId;
    request.hdr.opcode = MavlinkFTP::kCmdWriteFile;
    request.hdr.offset = _totalBytesSent;

    // Read chunk from file
    if (!_file.seek(_totalBytesSent)) {
        _finishUpload(tr("Upload failed: seek error"));
        return;
    }

    qint64 bytesRead = _file.read(reinterpret_cast<char*>(request.data), sizeof(request.data));
    if (bytesRead < 0) {
        _finishUpload(tr("Upload failed: read error"));
        return;
    }

    request.hdr.size = static_cast<uint8_t>(bytesRead);
    _lastChunkSize = static_cast<uint32_t>(bytesRead);

    if (firstRequest) {
        resetRetryCount();
    } else {
        _expectedSeqNumber -= 2;
    }

    sendRequest(&request);
}

void FTPUploadStateMachine::_resetSessions()
{
    MavlinkFTP::Request request{};
    request.hdr.session = 0;
    request.hdr.opcode = MavlinkFTP::kCmdResetSessions;
    request.hdr.offset = 0;
    request.hdr.size = 0;
    sendRequest(&request);
}

void FTPUploadStateMachine::_finishUpload(const QString& errorMsg)
{
    _file.close();
    completeOperation(errorMsg);
    emit uploadComplete(_remotePath, errorMsg);
    postEvent("error");
}

void FTPUploadStateMachine::onAckReceived(const MavlinkFTP::Request* ack)
{
    if (_cancelled) {
        return;
    }

    MavlinkFTP::OpCode_t reqOpCode = static_cast<MavlinkFTP::OpCode_t>(ack->hdr.req_opcode);

    switch (_phase) {
    case Phase::CreatingFile:
        if (reqOpCode != MavlinkFTP::kCmdCreateFile) {
            return;
        }
        _sessionId = ack->hdr.session;
        qCDebug(FTPStateMachineLog) << "File created, session:" << _sessionId;
        advanceToNextState();
        break;

    case Phase::WritingFile:
        if (reqOpCode != MavlinkFTP::kCmdWriteFile) {
            return;
        }
        {
            _totalBytesSent += _lastChunkSize;
            qCDebug(FTPStateMachineLog) << "Write ACK, total sent:" << _totalBytesSent << "of" << _fileSize;

            // Emit progress
            if (_fileSize > 0) {
                emit progressChanged(static_cast<float>(_totalBytesSent) / static_cast<float>(_fileSize));
            }

            if (_totalBytesSent >= _fileSize) {
                advanceToNextState();
            } else {
                _writeFile(true);
            }
        }
        break;

    case Phase::ResettingSessions:
        if (reqOpCode != MavlinkFTP::kCmdResetSessions) {
            return;
        }
        advanceToNextState();
        break;

    default:
        break;
    }
}

void FTPUploadStateMachine::onNakReceived(const MavlinkFTP::Request* nak)
{
    if (_cancelled) {
        return;
    }

    MavlinkFTP::OpCode_t reqOpCode = static_cast<MavlinkFTP::OpCode_t>(nak->hdr.req_opcode);

    switch (_phase) {
    case Phase::CreatingFile:
        if (reqOpCode != MavlinkFTP::kCmdCreateFile) {
            return;
        }
        _finishUpload(tr("Upload failed: %1").arg(errorMsgFromNak(nak)));
        break;

    case Phase::WritingFile:
        if (reqOpCode != MavlinkFTP::kCmdWriteFile) {
            return;
        }
        _finishUpload(tr("Upload failed: %1").arg(errorMsgFromNak(nak)));
        break;

    case Phase::ResettingSessions:
        // Ignore reset session errors - proceed to complete
        qCDebug(FTPStateMachineLog) << "Reset sessions NAK (ignored)";
        advanceToNextState();
        break;

    default:
        break;
    }
}

void FTPUploadStateMachine::onTimeout()
{
    if (_cancelled) {
        return;
    }

    if (shouldRetry()) {
        qCDebug(FTPStateMachineLog) << "Upload timeout, phase:" << static_cast<int>(_phase) << "retrying:" << retryCount();

        switch (_phase) {
        case Phase::CreatingFile:
            _createFile();
            break;
        case Phase::WritingFile:
            _writeFile(false);
            break;
        case Phase::ResettingSessions:
            _resetSessions();
            break;
        default:
            break;
        }
    } else {
        _finishUpload(tr("Upload failed: timeout"));
    }
}
