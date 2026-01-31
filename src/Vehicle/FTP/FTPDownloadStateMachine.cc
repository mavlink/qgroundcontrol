#include "FTPDownloadStateMachine.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QFileInfo>

Q_DECLARE_LOGGING_CATEGORY(FTPStateMachineLog)

FTPDownloadStateMachine::FTPDownloadStateMachine(Vehicle* vehicle, QObject* parent)
    : FTPOperationStateMachine("FTPDownload", vehicle, MAV_COMP_ID_AUTOPILOT1, parent)
{
    _buildStateMachine();
}

void FTPDownloadStateMachine::_buildStateMachine()
{
    _idleState = new QGCState("Idle", this);
    _openingState = new QGCState("Opening", this);
    _burstReadingState = new QGCState("BurstReading", this);
    _fillingBlocksState = new QGCState("FillingBlocks", this);
    _resettingState = new QGCState("Resetting", this);
    _completeState = new QGCFinalState("Complete", this);
    _errorState = new QGCState("Error", this);

    setInitialState(_idleState);

    connect(_openingState, &QAbstractState::entered, this, [this]() {
        qCDebug(FTPStateMachineLog) << "Opening file:" << _remotePath;
        _phase = Phase::OpeningFile;
        _openFile();
    });

    connect(_burstReadingState, &QAbstractState::entered, this, [this]() {
        qCDebug(FTPStateMachineLog) << "Starting burst read";
        _phase = Phase::BurstReading;
        _startBurstRead(true);
    });

    connect(_fillingBlocksState, &QAbstractState::entered, this, [this]() {
        qCDebug(FTPStateMachineLog) << "Filling missing blocks:" << _missingBlocks.size();
        _phase = Phase::FillingMissingBlocks;
        _fillMissingBlocks(true);
    });

    connect(_resettingState, &QAbstractState::entered, this, [this]() {
        qCDebug(FTPStateMachineLog) << "Resetting sessions";
        _phase = Phase::ResettingSessions;
        _resetSessions();
    });

    connect(_completeState, &QAbstractState::entered, this, [this]() {
        qCDebug(FTPStateMachineLog) << "Download complete:" << _file.fileName();
        _file.close();
        _phase = Phase::Complete;
        emit downloadComplete(_file.fileName(), QString());
        completeOperation();
    });

    connect(_errorState, &QAbstractState::entered, this, [this]() {
        qCWarning(FTPStateMachineLog) << "Download failed:" << _remotePath;
        _file.close();
    });

    // Transitions
    _idleState->addTransition(new MachineEventTransition("start", _openingState));
    _openingState->addTransition(new MachineEventTransition("advance", _burstReadingState));
    _openingState->addTransition(new MachineEventTransition("error", _errorState));
    _burstReadingState->addTransition(new MachineEventTransition("advance", _fillingBlocksState));
    _burstReadingState->addTransition(new MachineEventTransition("complete", _resettingState));
    _burstReadingState->addTransition(new MachineEventTransition("error", _errorState));
    _fillingBlocksState->addTransition(new MachineEventTransition("advance", _resettingState));
    _fillingBlocksState->addTransition(new MachineEventTransition("error", _errorState));
    _resettingState->addTransition(new MachineEventTransition("advance", _completeState));
    _resettingState->addTransition(new MachineEventTransition("error", _completeState)); // Complete even on reset error
}

bool FTPDownloadStateMachine::download(uint8_t compId, const QString& remotePath, const QString& localDir,
                                        const QString& fileName, bool checkSize)
{
    if (_operationInProgress) {
        qCWarning(FTPStateMachineLog) << "Cannot download - operation already in progress";
        return false;
    }

    _compId = (compId == MAV_COMP_ID_ALL) ? MAV_COMP_ID_AUTOPILOT1 : compId;
    _remotePath = remotePath;
    _localDir = QDir(localDir);
    _fileName = fileName.isEmpty() ? QFileInfo(remotePath).fileName() : fileName;
    _checkSize = checkSize;
    _cancelled = false;

    // Reset state
    _sessionId = 0;
    _fileSize = 0;
    _expectedOffset = 0;
    _bytesWritten = 0;
    _missingBlocks.clear();
    _phase = Phase::Idle;

    _operationInProgress = true;
    _retryCount = 0;
    _expectedSeqNumber = 0;

    // Create local file
    QString localPath = _localDir.filePath(_fileName);
    _file.setFileName(localPath);
    if (!_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qCWarning(FTPStateMachineLog) << "Cannot create local file:" << localPath;
        _operationInProgress = false;
        return false;
    }

    if (!isRunning()) {
        start();
    }

    QTimer::singleShot(0, this, [this]() {
        postEvent("start");
    });

    return true;
}

void FTPDownloadStateMachine::cancel()
{
    if (!_operationInProgress) {
        return;
    }

    _cancelled = true;
    _file.close();
    QString localPath = _file.fileName();
    completeOperation(tr("Aborted"));
    emit downloadComplete(localPath, tr("Aborted"));
}

void FTPDownloadStateMachine::_openFile()
{
    MavlinkFTP::Request request{};
    request.hdr.session = 0;
    request.hdr.opcode = MavlinkFTP::kCmdOpenFileRO;
    request.hdr.offset = 0;
    request.hdr.size = 0;
    fillRequestDataWithString(&request, _remotePath);
    sendRequest(&request);
}

void FTPDownloadStateMachine::_startBurstRead(bool firstRequest)
{
    qCDebug(FTPStateMachineLog) << "Burst read offset:" << _expectedOffset << "firstRequest:" << firstRequest;

    MavlinkFTP::Request request{};
    request.hdr.session = _sessionId;
    request.hdr.opcode = MavlinkFTP::kCmdBurstReadFile;
    request.hdr.offset = _expectedOffset;
    request.hdr.size = sizeof(request.data);

    if (firstRequest) {
        resetRetryCount();
    } else {
        _expectedSeqNumber -= 2;
    }

    sendRequest(&request);
}

void FTPDownloadStateMachine::_fillMissingBlocks(bool firstRequest)
{
    if (_missingBlocks.isEmpty()) {
        advanceToNextState();
        return;
    }

    MissingBlock& block = _missingBlocks.first();
    qCDebug(FTPStateMachineLog) << "Reading missing block offset:" << block.offset << "size:" << block.size;

    MavlinkFTP::Request request{};
    request.hdr.session = _sessionId;
    request.hdr.opcode = MavlinkFTP::kCmdReadFile;
    request.hdr.offset = block.offset;
    request.hdr.size = static_cast<uint8_t>(qMin(static_cast<uint32_t>(sizeof(request.data)), block.size));

    if (firstRequest) {
        resetRetryCount();
    } else {
        _expectedSeqNumber -= 2;
    }

    sendRequest(&request);
}

void FTPDownloadStateMachine::_resetSessions()
{
    MavlinkFTP::Request request{};
    request.hdr.session = 0;
    request.hdr.opcode = MavlinkFTP::kCmdResetSessions;
    request.hdr.offset = 0;
    request.hdr.size = 0;
    sendRequest(&request);
}

void FTPDownloadStateMachine::_finishDownload(const QString& errorMsg)
{
    _file.close();
    QString localPath = _file.fileName();
    completeOperation(errorMsg);
    emit downloadComplete(localPath, errorMsg);
    postEvent("error");
}

void FTPDownloadStateMachine::onAckReceived(const MavlinkFTP::Request* ack)
{
    if (_cancelled) {
        return;
    }

    MavlinkFTP::OpCode_t reqOpCode = static_cast<MavlinkFTP::OpCode_t>(ack->hdr.req_opcode);

    switch (_phase) {
    case Phase::OpeningFile:
        if (reqOpCode != MavlinkFTP::kCmdOpenFileRO) {
            return;
        }
        _sessionId = ack->hdr.session;
        _fileSize = *reinterpret_cast<const uint32_t*>(&ack->data[0]);
        qCDebug(FTPStateMachineLog) << "File opened, session:" << _sessionId << "size:" << _fileSize;
        advanceToNextState();
        break;

    case Phase::BurstReading:
        if (reqOpCode != MavlinkFTP::kCmdBurstReadFile) {
            return;
        }
        {
            uint32_t offset = ack->hdr.offset;
            uint32_t dataSize = ack->hdr.size;

            qCDebug(FTPStateMachineLog) << "Burst data received offset:" << offset << "size:" << dataSize;

            // Track missing blocks if data arrives out of order
            if (offset > _expectedOffset) {
                MissingBlock missing;
                missing.offset = _expectedOffset;
                missing.size = offset - _expectedOffset;
                _missingBlocks.append(missing);
                qCDebug(FTPStateMachineLog) << "Missing block detected:" << missing.offset << "size:" << missing.size;
            }

            // Write data at correct position
            if (!_file.seek(offset)) {
                _finishDownload(tr("Download failed: seek error"));
                return;
            }
            qint64 written = _file.write(reinterpret_cast<const char*>(ack->data), dataSize);
            if (written != dataSize) {
                _finishDownload(tr("Download failed: write error"));
                return;
            }

            _bytesWritten += dataSize;
            _expectedOffset = offset + dataSize;

            // Emit progress
            if (_fileSize > 0) {
                emit progressChanged(static_cast<float>(_bytesWritten) / static_cast<float>(_fileSize));
            }

            // More data coming via burst
            resetRetryCount();
        }
        break;

    case Phase::FillingMissingBlocks:
        if (reqOpCode != MavlinkFTP::kCmdReadFile) {
            return;
        }
        {
            if (_missingBlocks.isEmpty()) {
                advanceToNextState();
                return;
            }

            MissingBlock& block = _missingBlocks.first();
            uint32_t offset = ack->hdr.offset;
            uint32_t dataSize = ack->hdr.size;

            // Write data at correct position
            if (!_file.seek(offset)) {
                _finishDownload(tr("Download failed: seek error"));
                return;
            }
            _file.write(reinterpret_cast<const char*>(ack->data), dataSize);

            block.offset += dataSize;
            block.size -= dataSize;

            if (block.size == 0) {
                _missingBlocks.removeFirst();
            }

            if (_missingBlocks.isEmpty()) {
                advanceToNextState();
            } else {
                _fillMissingBlocks(true);
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

void FTPDownloadStateMachine::onNakReceived(const MavlinkFTP::Request* nak)
{
    if (_cancelled) {
        return;
    }

    MavlinkFTP::OpCode_t reqOpCode = static_cast<MavlinkFTP::OpCode_t>(nak->hdr.req_opcode);
    MavlinkFTP::ErrorCode_t errorCode = static_cast<MavlinkFTP::ErrorCode_t>(nak->data[0]);

    switch (_phase) {
    case Phase::OpeningFile:
        if (reqOpCode != MavlinkFTP::kCmdOpenFileRO) {
            return;
        }
        _finishDownload(tr("Download failed: %1").arg(errorMsgFromNak(nak)));
        break;

    case Phase::BurstReading:
        if (reqOpCode != MavlinkFTP::kCmdBurstReadFile) {
            return;
        }
        if (errorCode == MavlinkFTP::kErrEOF) {
            qCDebug(FTPStateMachineLog) << "Burst read EOF";
            // Check if we have missing blocks
            if (_missingBlocks.isEmpty()) {
                postEvent("complete");
            } else {
                advanceToNextState(); // Go to filling blocks state
            }
        } else {
            _finishDownload(tr("Download failed: %1").arg(errorMsgFromNak(nak)));
        }
        break;

    case Phase::FillingMissingBlocks:
        if (reqOpCode != MavlinkFTP::kCmdReadFile) {
            return;
        }
        _finishDownload(tr("Download failed: %1").arg(errorMsgFromNak(nak)));
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

void FTPDownloadStateMachine::onTimeout()
{
    if (_cancelled) {
        return;
    }

    if (shouldRetry()) {
        qCDebug(FTPStateMachineLog) << "Download timeout, phase:" << static_cast<int>(_phase) << "retrying:" << retryCount();

        switch (_phase) {
        case Phase::OpeningFile:
            _openFile();
            break;
        case Phase::BurstReading:
            _startBurstRead(false);
            break;
        case Phase::FillingMissingBlocks:
            _fillMissingBlocks(false);
            break;
        case Phase::ResettingSessions:
            _resetSessions();
            break;
        default:
            break;
        }
    } else {
        _finishDownload(tr("Download failed: timeout"));
    }
}
