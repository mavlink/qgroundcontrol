/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "FTPManager.h"
#include "QGC.h"
#include "MAVLinkProtocol.h"
#include "Vehicle.h"
#include "QGCApplication.h"

#include <QFile>
#include <QDir>
#include <string>

QGC_LOGGING_CATEGORY(FTPManagerLog, "FTPManagerLog")

FTPManager::FTPManager(Vehicle* vehicle)
    : QObject   (vehicle)
    , _vehicle  (vehicle)
{
    connect(&_ackTimer, &QTimer::timeout, this, &FTPManager::_ackTimeout);
    
    _lastOutgoingRequest.hdr.seqNumber = 0;
    
    // Make sure we don't have bad structure packing
    Q_ASSERT(sizeof(MavlinkFTP::RequestHeader) == 12);
}

void FTPManager::_handlOpenFileROAck(MavlinkFTP::Request* ack)
{
    qCDebug(FTPManagerLog) << QString("_openAckResponse: _waitState(%1) _openFileType(%2) _readFileLength(%3)").arg(MavlinkFTP::opCodeToString(_waitState)).arg(MavlinkFTP::opCodeToString(_openFileType)).arg(ack->openFileLength);
    
    if (_waitState != MavlinkFTP::kCmdOpenFileRO && _waitState != MavlinkFTP::kCmdBurstReadFile) {
        qCDebug(FTPManagerLog) << "Received OpenFileRO Ack while not waiting for it. _waitState" << MavlinkFTP::opCodeToString(_waitState);
        return;
    }

    if (ack->hdr.size != sizeof(uint32_t)) {
        _downloadComplete(tr("Download failed: Invalid response to OpenFileRO command."));
        return;
    }

    _waitState                  = _openFileType;
    _activeSession              = ack->hdr.session;
    _downloadFileSize           = ack->openFileLength;
    _requestedDownloadOffset    = 0;
    _readFileAccumulator.clear();

    MavlinkFTP::Request request;
    request.hdr.session = _activeSession;
    request.hdr.opcode  = _waitState;
    request.hdr.offset  = _requestedDownloadOffset;
    request.hdr.size    = sizeof(request.data);
    _sendRequestExpectAck(&request);
}

/// request the next missing part of a (partially) downloaded file
void FTPManager::_requestMissingData()
{
#if 0
    if (_missingData.empty()) {
        _downloadingMissingParts = false;
        _missingDownloadedBytes = 0;
        // there might be more data missing at the end
        if ((uint32_t)_readFileAccumulator.length() != _downloadFileSize) {
            _downloadOffset = _readFileAccumulator.length();
            qCDebug(FTPManagerLog) << QString("_requestMissingData: missing parts done, downloadOffset(%1) downloadFileSize(%2)")
                                      .arg(_downloadOffset).arg(_downloadFileSize);
        } else {
            _closeDownloadSession(true);
            return;
        }
    } else {
        qCDebug(FTPManagerLog) << QString("_requestMissingData: offset(%1) size(%2)").arg(_missingData.head().offset).arg(_missingData.head().size);

        _downloadOffset = _missingData.head().offset;
    }

    MavlinkFTP::Request request;
    _currentOperation = kCORead;
    request.hdr.session = _activeSession;
    request.hdr.opcode = MavlinkFTP::kCmdReadFile;
    request.hdr.offset = _downloadOffset;
    request.hdr.size = sizeof(request.data);

    _sendRequestExpectAck(&request);
#endif
}

/// Closes out a download session by writing the file and doing cleanup.
///     @param success true: successful download completion, false: error during download
void FTPManager::_downloadComplete(const QString& errorMsg)
{
    qCDebug(FTPManagerLog) << QString("_downloadComplete: errorMsg(%1)").arg(errorMsg);
    
    QString downloadFilePath    = _readFileDownloadDir.absoluteFilePath(_readFileDownloadFilename);
    QString error               = errorMsg;

    _waitState = MavlinkFTP::kCmdNone;

    if (error.isEmpty()) {
        QFile file(downloadFilePath);

        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qint64 bytesWritten = file.write((const char *)_readFileAccumulator, _readFileAccumulator.length());
            if (bytesWritten != _readFileAccumulator.length()) {
                error = tr("Download failed: Unable to write data to local file '%1'.").arg(downloadFilePath);
            }
        } else {
            error = tr("Download failed: Unable to open local file '%1' for writing. Error: '%2'.").arg(downloadFilePath).arg(file.errorString());
        }
        file.close();
    }
    
    _readFileAccumulator.clear();
    _sendResetCommand(); // Close the open session
    emit downloadComplete(downloadFilePath, error);
}

/// Closes out an upload session doing cleanup.
///     @param success true: successful upload completion, false: error during download
void FTPManager::_uploadComplete(const QString& errorMsg)
{
    qCDebug(FTPManagerLog) << QString("_uploadComplete: errorMsg(%1)").arg(errorMsg);

    _waitState      = MavlinkFTP::kCmdNone;
    _writeFileSize  = 0;
    _writeFileAccumulator.clear();
    _sendResetCommand();
    emit uploadComplete(errorMsg);
}

void FTPManager::_handleReadFileAck(MavlinkFTP::Request* ack, bool burstReadFile)
{
    if (ack->hdr.session != _activeSession) {
        return;
    }

    qCDebug(FTPManagerLog) << QString("_handleReadFileAck: burstReadFile(%1) offset(%2) size(%3) burstComplete(%4)").arg(burstReadFile).arg(ack->hdr.offset).arg(ack->hdr.size).arg(ack->hdr.burstComplete);

    if (ack->hdr.offset != _requestedDownloadOffset) {
        // FIXME: NYI deal with missing packets
        _downloadComplete(tr("Download failed: Received incorrect offset: received:expected %1/%2").arg(ack->hdr.offset).arg(_requestedDownloadOffset));
        return;
    }

    _readFileAccumulator.append((const char*)ack->data, ack->hdr.size);

    _requestedDownloadOffset += ack->hdr.size;
    
    if (_downloadFileSize != 0) {
        emit commandProgress(100 * ((float)(_readFileAccumulator.length()) / (float)_downloadFileSize));
    }

    if (!burstReadFile || ack->hdr.burstComplete) {
        MavlinkFTP::Request request;
        request.hdr.session = _activeSession;
        request.hdr.opcode  = _waitState;
        request.hdr.offset  = _requestedDownloadOffset;
        request.hdr.size    = 0;
        _sendRequestExpectAck(&request);
    } else if (burstReadFile) {
        // Burst read, next ack should come without having to request it
        _setupAckTimeout();
    }
}

/// @brief Respond to the Ack associated with the List command.
void FTPManager::_listAckResponse(MavlinkFTP::Request* listAck)
{
#if 0
    if (listAck->hdr.offset != _listOffset) {
        // this is a real error (directory listing is synchronous), no need to retransmit
        _currentOperation = kCOIdle;
        _emitErrorMessage(tr("List: Offset returned (%1) differs from offset requested (%2)").arg(listAck->hdr.offset).arg(_listOffset));
        return;
    }

    uint8_t offset = 0;
    uint8_t cListEntries = 0;
    uint8_t cBytes = listAck->hdr.size;

    // parse filenames out of the buffer
    while (offset < cBytes) {
        const char * ptr = ((const char *)listAck->data) + offset;

        // get the length of the name
        uint8_t cBytesLeft = cBytes - offset;
        uint8_t nlen = static_cast<uint8_t>(strnlen(ptr, cBytesLeft));
        if ((*ptr == 'S' && nlen > 1) || (*ptr != 'S' && nlen < 2)) {
            _currentOperation = kCOIdle;
            _emitErrorMessage(tr("Incorrectly formed list entry: '%1'").arg(ptr));
            return;
        } else if (nlen == cBytesLeft) {
            _currentOperation = kCOIdle;
            _emitErrorMessage(tr("Missing NULL termination in list entry"));
            return;
        }

        // Returned names are prepended with D for directory, F for file, S for skip
        if (*ptr == 'F' || *ptr == 'D') {
            // put it in the view
            _emitListEntry(ptr);
        } else if (*ptr == 'S') {
            // do nothing
        } else {
            qDebug() << "unknown entry" << ptr;
        }

        // account for the name + NUL
        offset += nlen + 1;

        cListEntries++;
    }

    if (listAck->hdr.size == 0 || cListEntries == 0) {
        // Directory is empty, we're done
        Q_ASSERT(listAck->hdr.opcode == MavlinkFTP::kRspAck);
        _currentOperation = kCOIdle;
        emit commandComplete();
    } else {
        // Possibly more entries to come, need to keep trying till we get EOF
        _currentOperation = kCOList;
        _listOffset += cListEntries;
        _sendListCommand();
    }
#endif
}

/// @brief Respond to the Ack associated with the create command.
void FTPManager::_createAckResponse(MavlinkFTP::Request* createAck)
{
#if 0
    qCDebug(FTPManagerLog) << "_createAckResponse";
    
    _currentOperation = kCOWrite;
    _activeSession = createAck->hdr.session;

    // Start the sequence of write commands from the beginning of the file

    _writeOffset = 0;
    _writeSize = 0;
    
    _writeFileDatablock();
#endif
}

/// @brief Respond to the Ack associated with the write command.
void FTPManager::_writeAckResponse(MavlinkFTP::Request* writeAck)
{
#if 0
    if(_writeOffset + _writeSize >= _writeFileSize){
        _closeUploadSession(true /* success */);
        return;
    }

    if (writeAck->hdr.session != _activeSession) {
        _closeUploadSession(false /* failure */);
        _emitErrorMessage(tr("Write: Incorrect session returned"));
        return;
    }

    if (writeAck->hdr.offset != _writeOffset) {
        _closeUploadSession(false /* failure */);
        _emitErrorMessage(tr("Write: Offset returned (%1) differs from offset requested (%2)").arg(writeAck->hdr.offset).arg(_writeOffset));
        return;
    }

    if (writeAck->hdr.size != sizeof(uint32_t)) {
        _closeUploadSession(false /* failure */);
        _emitErrorMessage(tr("Write: Returned invalid size of write size data"));
        return;
    }


    if( writeAck->writeFileLength !=_writeSize) {
        _closeUploadSession(false /* failure */);
        _emitErrorMessage(tr("Write: Size returned (%1) differs from size requested (%2)").arg(writeAck->writeFileLength).arg(_writeSize));
        return;
    }

    _writeFileDatablock();
#endif
}

/// @brief Send next write file data block.
void FTPManager::_writeFileDatablock(void)
{
#if 0
    if (_writeOffset + _writeSize >= _writeFileSize){
        _closeUploadSession(true /* success */);
        return;
    }

    _writeOffset += _writeSize;

    MavlinkFTP::Request request;
    request.hdr.session = _activeSession;
    request.hdr.opcode = MavlinkFTP::kCmdWriteFile;
    request.hdr.offset = _writeOffset;

    if(_writeFileSize -_writeOffset > sizeof(request.data) )
        _writeSize = sizeof(request.data);
    else
        _writeSize = _writeFileSize - _writeOffset;

    request.hdr.size = _writeSize;

    memcpy(request.data, &_writeFileAccumulator.data()[_writeOffset], _writeSize);

    _sendRequestExpectAck(&request);
#endif
}

void FTPManager::mavlinkMessageReceived(mavlink_message_t message)
{
    if (message.msgid != MAVLINK_MSG_ID_FILE_TRANSFER_PROTOCOL) {
        return;
    }

    mavlink_file_transfer_protocol_t data;
    mavlink_msg_file_transfer_protocol_decode(&message, &data);

    // Make sure we are the target system
    int qgcId = qgcApp()->toolbox()->mavlinkProtocol()->getSystemId();
    if (data.target_system != qgcId) {
        return;
    }
    
    MavlinkFTP::Request* request = (MavlinkFTP::Request*)&data.payload[0];

    uint16_t incomingSeqNumber = request->hdr.seqNumber;
    uint16_t expectedSeqNumber = _lastOutgoingRequest.hdr.seqNumber + 1;

    // ignore old/reordered packets (handle wrap-around properly)
    if ((uint16_t)((expectedSeqNumber - 1) - incomingSeqNumber) < (std::numeric_limits<uint16_t>::max()/2)) {
        qDebug() << "Received old packet: expected seq:" << expectedSeqNumber << "got:" << incomingSeqNumber;
        return;
    }

    _clearAckTimeout();
    
    qCDebug(FTPManagerLog) << "mavlinkMessageReceived" << MavlinkFTP::opCodeToString(static_cast<MavlinkFTP::OpCode_t>(request->hdr.opcode)) <<  MavlinkFTP::opCodeToString(static_cast<MavlinkFTP::OpCode_t>(request->hdr.req_opcode));

    if (incomingSeqNumber != expectedSeqNumber) {
        switch (_waitState) {
        case MavlinkFTP::kCmdOpenFileRO:
        case MavlinkFTP::kCmdReadFile:
        case MavlinkFTP::kCmdBurstReadFile:
            _downloadComplete(tr("Download failed: Unable to handle packet loss"));
            return;
#if 0
        case kCOWrite:
            _closeUploadSession(false /* failure */);
            break;

        case kCOOpenRead:
        case kCOOpenBurst:
        case kCOCreate:
            // We could have an open session hanging around
            _currentOperation = kCOIdle;
            _sendResetCommand();
            break;
#endif
        default:
            // Don't need to do anything special
            _waitState = MavlinkFTP::kCmdNone;
            break;
        }
    }
    
    // Move past the incoming sequence number for next request
    _lastOutgoingRequest.hdr.seqNumber = incomingSeqNumber;

    if (request->hdr.opcode == MavlinkFTP::kRspAck) {
        switch (request->hdr.req_opcode) {
        case MavlinkFTP::kCmdOpenFileRO:
            _handlOpenFileROAck(request);
            break;
        case MavlinkFTP::kCmdReadFile:
            _handleReadFileAck(request, false /* burstReadFile */);
            break;
        case MavlinkFTP::kCmdBurstReadFile:
            _handleReadFileAck(request, true /*burstReadFile */);
            break;

#if 0
        case MavlinkFTP::kCmdListDirectory:
            _listAckResponse(request);
            break;

        case MavlinkFTP::kCmdOpenFileRO:
        case MavlinkFTP::kCmdOpenFileWO:
            _handlOpenFileROAck(request);
            break;

        case MavlinkFTP::kCmdCreateFile:
            _createAckResponse(request);
            break;

        case MavlinkFTP::kCmdWriteFile:
            _writeAckResponse(request);
            break;
#endif
        default:
            // Ack back from operation which does not require additional work
            _waitState = MavlinkFTP::kCmdNone;
            break;
        }
    } else if (request->hdr.opcode == MavlinkFTP::kRspNak) {
        QString                 errorMsg;
        MavlinkFTP::OpCode_t    requestOpCode = static_cast<MavlinkFTP::OpCode_t>(request->hdr.req_opcode);
        MavlinkFTP::ErrorCode_t errorCode = static_cast<MavlinkFTP::ErrorCode_t>(request->data[0]);

        if (requestOpCode == MavlinkFTP::kCmdReadFile && errorCode == MavlinkFTP::kErrEOF && _readFileAccumulator.size() == _downloadFileSize) {
            _downloadComplete(QString());
        } else {
            // Nak's normally have 1 byte of data for error code, except for MavlinkFTP::kErrFailErrno which has additional byte for errno
            if ((errorCode == MavlinkFTP::kErrFailErrno && request->hdr.size != 2) || ((errorCode != MavlinkFTP::kErrFailErrno) && request->hdr.size != 1)) {
                errorMsg = tr("Invalid Nak format");
            } else if (errorCode == MavlinkFTP::kErrFailErrno) {
                errorMsg = tr("errno %1").arg(request->data[1]);
            } else {
                errorMsg = MavlinkFTP::errorCodeToString(errorCode);
            }

            _waitState = MavlinkFTP::kCmdNone;

            switch (request->hdr.req_opcode) {
            case MavlinkFTP::kCmdOpenFileRO:
            case MavlinkFTP::kCmdReadFile:
            case MavlinkFTP::kCmdBurstReadFile:
                _downloadComplete(tr("Download failed: %1").arg(errorMsg));
                break;
            default:
                // FIXME: Rest is NYI
                break;
            }
        }
    }
}

void FTPManager::listDirectory(const QString& dirPath)
{
    if (_waitState != MavlinkFTP::kCmdNone) {
        _emitErrorMessage(tr("Command not sent. Waiting for previous command to complete."));
        return;
    }

    _dedicatedLink = _vehicle->priorityLink();
    if (!_dedicatedLink) {
        _emitErrorMessage(tr("Command not sent. No Vehicle links."));
        return;
    }

    // initialise the lister
    _listPath           = dirPath;
    _listOffset         = 0;
    _waitState   =      MavlinkFTP::kCmdListDirectory;

    // and send the initial request
    _sendListCommand();
}

void FTPManager::_fillRequestWithString(MavlinkFTP::Request* request, const QString& str)
{
    strncpy((char *)&request->data[0], str.toStdString().c_str(), sizeof(request->data));
    request->hdr.size = static_cast<uint8_t>(strnlen((const char *)&request->data[0], sizeof(request->data)));
}

void FTPManager::_sendListCommand(void)
{
    MavlinkFTP::Request request;

    request.hdr.session = 0;
    request.hdr.opcode = MavlinkFTP::kCmdListDirectory;
    request.hdr.offset = _listOffset;
    request.hdr.size = 0;

    _fillRequestWithString(&request, _listPath);

    qCDebug(FTPManagerLog) << "listDirectory: path:" << _listPath << "offset:" <<  _listOffset;
    
    _sendRequestExpectAck(&request);
}

bool FTPManager::download(const QString& from, const QString& toDir)
{
    qCDebug(FTPManagerLog) << "download from:" << from << "to:" << toDir;
    return _downloadWorker(from, toDir, false /* burstReadFile */);
}

bool FTPManager::burstDownload(const QString& from, const QString& toDir)
{
    qCDebug(FTPManagerLog) << "burstDownload from:" << from << "to:" << toDir;
    return _downloadWorker(from, toDir, true /* burstReadFile */);
}

bool FTPManager::_downloadWorker(const QString& from, const QString& toDir, bool burstReadFile)
{
    if (_waitState != MavlinkFTP::kCmdNone) {
        qCDebug(FTPManagerLog) << "Cannot download. Already in another operation";
        return false;
    }

    _dedicatedLink = _vehicle->priorityLink();
    if (!_dedicatedLink) {
        qCDebug(FTPManagerLog) << "Cannot download. Vehicle has no priority link";
        return false;
    }

    _readFileDownloadDir.setPath(toDir);

    QString strippedFrom;
    QString ftpPrefix("mavlinkftp://");
    if (from.startsWith(ftpPrefix, Qt::CaseInsensitive)) {
        strippedFrom = from.right(from.length() - ftpPrefix.length() + 1);
    } else {
        strippedFrom = from;
    }

    // We need to strip off the file name from the fully qualified path. We can't use the usual QDir
    // routines because this path does not exist locally.
    int lastDirSlashIndex;
    for (lastDirSlashIndex=strippedFrom.size()-1; lastDirSlashIndex>=0; lastDirSlashIndex--) {
        if (strippedFrom[lastDirSlashIndex] == '/') {
            break;
        }
    }
    lastDirSlashIndex++; // move past slash
    _readFileDownloadFilename = strippedFrom.right(strippedFrom.size() - lastDirSlashIndex);

    _waitState      = MavlinkFTP::kCmdOpenFileRO;
    _openFileType   = burstReadFile ? MavlinkFTP::kCmdBurstReadFile : MavlinkFTP::kCmdReadFile;

    MavlinkFTP::Request request;
    request.hdr.session = 0;
    request.hdr.opcode  = MavlinkFTP::kCmdOpenFileRO;
    request.hdr.offset  = 0;
    request.hdr.size    = 0;
    _fillRequestWithString(&request, strippedFrom);
    _sendRequestExpectAck(&request);

    return true;
}

/// @brief Uploads the specified file.
///     @param toPath File in UAS to upload to, fully qualified path
///     @param uploadFile Local file to upload from
void FTPManager::upload(const QString& toPath, const QFileInfo& uploadFile)
{
#if 0
    if(_currentOperation != kCOIdle){
        _emitErrorMessage(tr("UAS File manager busy. Try again later"));
        return;
    }

    _dedicatedLink = _vehicle->priorityLink();
    if (!_dedicatedLink) {
        _emitErrorMessage(tr("Command not sent. No Vehicle links."));
        return;
    }

    if (toPath.isEmpty()) {
        return;
    }

    if (!uploadFile.isReadable()){
        _emitErrorMessage(tr("File (%1) is not readable for upload").arg(uploadFile.path()));
        return;
    }

    QFile file(uploadFile.absoluteFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        _emitErrorMessage(tr("Unable to open local file for upload (%1)").arg(uploadFile.absoluteFilePath()));
        return;
    }

    _writeFileAccumulator = file.readAll();
    _writeFileSize = _writeFileAccumulator.size();

    file.close();

    if (_writeFileAccumulator.size() == 0) {
        _emitErrorMessage(tr("Unable to read data from local file (%1)").arg(uploadFile.absoluteFilePath()));
        return;
    }

    _currentOperation = kCOCreate;

    MavlinkFTP::Request request;
    request.hdr.session = 0;
    request.hdr.opcode = MavlinkFTP::kCmdCreateFile;
    request.hdr.offset = 0;
    request.hdr.size = 0;
    _fillRequestWithString(&request, toPath + "/" + uploadFile.fileName());
    _sendRequestExpectAck(&request);
#endif
}

void FTPManager::createDirectory(const QString& directory)
{
#if 0
    if(_currentOperation != kCOIdle){
        _emitErrorMessage(tr("UAS File manager busy. Try again later"));
        return;
    }

    _currentOperation = kCOCreateDir;

    MavlinkFTP::Request request;
    request.hdr.session = 0;
    request.hdr.opcode = MavlinkFTP::kCmdCreateDirectory;
    request.hdr.offset = 0;
    request.hdr.size = 0;
    _fillRequestWithString(&request, directory);
    _sendRequestExpectAck(&request);
#endif
}

/// @brief Sends a command which only requires an opcode and no additional data
///     @param opcode Opcode to send
///     @param newOpState State to put state machine into
/// @return TRUE: command sent, FALSE: command not sent, waiting for previous command to finish
bool FTPManager::_sendOpcodeOnlyCmd(MavlinkFTP::OpCode_t opcode, MavlinkFTP::OpCode_t newWaitState)
{
    if (_waitState != MavlinkFTP::kCmdNone) {
        // Can't have multiple commands in play at the same time
        return false;
    }

    _waitState = newWaitState;

    MavlinkFTP::Request request;
    request.hdr.session = 0;
    request.hdr.opcode  = opcode;
    request.hdr.offset  = 0;
    request.hdr.size    = 0;
    _sendRequestExpectAck(&request);

    return true;
}

/// @brief Starts the ack timeout timer
void FTPManager::_setupAckTimeout(void)
{
    qCDebug(FTPManagerLog) << "_setupAckTimeout";

    Q_ASSERT(!_ackTimer.isActive());

    _ackNumTries = 0;
    _ackTimer.setSingleShot(false);
    _ackTimer.start(_ackTimerTimeoutMsecs);
}

/// @brief Clears the ack timeout timer
void FTPManager::_clearAckTimeout(void)
{
    qCDebug(FTPManagerLog) << "_clearAckTimeout";
    _ackTimer.stop();
}

void FTPManager::_ackTimeout(void)
{
    qCDebug(FTPManagerLog) << "_ackTimeout";
    
#if 0
    // FIXME: retry NYI
    if (++_ackNumTries <= _ackTimerMaxRetries) {
        qCDebug(FTPManagerLog) << "ack timeout - retrying";
        if (_currentOperation == kCOBurst) {
            // for burst downloads try to initiate a new burst
            MavlinkFTP::Request request;
            request.hdr.session = _activeSession;
            request.hdr.opcode = MavlinkFTP::kCmdBurstReadFile;
            request.hdr.offset = _downloadOffset;
            request.hdr.size = 0;
            request.hdr.seqNumber = ++_lastOutgoingRequest.hdr.seqNumber;

            _sendRequestNoAck(&request);
        } else {
            _sendRequestNoAck(&_lastOutgoingRequest);
        }
        return;
    }
#endif

    _clearAckTimeout();

    // Make sure to set _currentOperation state before emitting error message. Code may respond
    // to error message signal by sending another command, which will fail if state is not back
    // to idle. FileView UI works this way with the List command.

    switch (_waitState) {
    case MavlinkFTP::kCmdOpenFileRO:
    case MavlinkFTP::kCmdReadFile:
    case MavlinkFTP::kCmdBurstReadFile:
        _downloadComplete(tr("Download failed: Vehicle did not response to %1").arg(MavlinkFTP::opCodeToString(_waitState)));
        break;
#if 0
        // FIXME: NYI
    case kCOOpenRead:
    case kCOOpenBurst:
        _currentOperation = kCOIdle;
        _emitErrorMessage(tr("Timeout waiting for ack: Download failed"));
        _sendResetCommand();
        break;

    case kCOCreate:
        _currentOperation = kCOIdle;
        _emitErrorMessage(tr("Timeout waiting for ack: Upload failed"));
        _sendResetCommand();
        break;

    case kCOWrite:
        _closeUploadSession(false /* failure */);
        _emitErrorMessage(tr("Timeout waiting for ack: Upload failed"));
        break;
#endif
    default:
    {
        MavlinkFTP::OpCode_t    _lastCommand = _waitState;
        _waitState = MavlinkFTP::kCmdNone;
        _emitErrorMessage(QString("Timeout waiting for ack: Command failed (%1)").arg(MavlinkFTP::opCodeToString(_lastCommand)));
    }
        break;
    }
}

void FTPManager::_sendResetCommand(void)
{
    MavlinkFTP::Request request;
    request.hdr.opcode = MavlinkFTP::kCmdResetSessions;
    request.hdr.size = 0;
    _sendRequestExpectAck(&request);
}

void FTPManager::_emitErrorMessage(const QString& msg)
{
    qCDebug(FTPManagerLog) << "Error:" << msg;
    emit commandError(msg);
}

void FTPManager::_emitListEntry(const QString& entry)
{
    qCDebug(FTPManagerLog) << "_emitListEntry" << entry;
    emit listEntry(entry);
}

void FTPManager::_sendRequestExpectAck(MavlinkFTP::Request* request)
{
    _setupAckTimeout();
    
    request->hdr.seqNumber = ++_lastOutgoingRequest.hdr.seqNumber;
    qCDebug(FTPManagerLog) << "_sendRequestExpectAck opcode:" << MavlinkFTP::opCodeToString(static_cast<MavlinkFTP::OpCode_t>(request->hdr.opcode)) << "seqNumber:" << request->hdr.seqNumber;

    if (request->hdr.size <= sizeof(request->data)) {
        memcpy(&_lastOutgoingRequest, request, sizeof(MavlinkFTP::RequestHeader) + request->hdr.size);
    } else {
        // FIXME: Shouldn't this fail something?
        qCCritical(FTPManagerLog) << "request length too long:" << request->hdr.size;
    }
    
    _sendRequestNoAck(request);
}

void FTPManager::_sendRequestNoAck(MavlinkFTP::Request* request)
{
    qCDebug(FTPManagerLog) << "_sendRequestNoAck opcode:" << MavlinkFTP::opCodeToString(static_cast<MavlinkFTP::OpCode_t>(request->hdr.opcode));

    mavlink_message_t message;
    mavlink_msg_file_transfer_protocol_pack_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                                 qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                                 _dedicatedLink->mavlinkChannel(),
                                                 &message,
                                                 0,                                                     // Target network, 0=broadcast?
                                                 _vehicle->id(),
                                                 MAV_COMP_ID_AUTOPILOT1,
                                                 (uint8_t*)request);                                    // Payload
    _vehicle->sendMessageOnLinkThreadSafe(_dedicatedLink, message);
}
