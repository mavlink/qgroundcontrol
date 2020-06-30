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
    _ackTimer.setSingleShot(true);
    if (qgcApp()->runningUnitTests()) {
        // Mock link responds immediately if at all
        _ackTimer.setInterval(10);
    } else {
        _ackTimer.setInterval(_ackTimerTimeoutMsecs);
    }
    connect(&_ackTimer, &QTimer::timeout, this, &FTPManager::_ackTimeout);
    
    _lastOutgoingRequest.hdr.seqNumber = 0;
    
    // Make sure we don't have bad structure packing
    Q_ASSERT(sizeof(MavlinkFTP::RequestHeader) == 12);
}

void FTPManager::_handlOpenFileROAck(MavlinkFTP::Request* ack)
{
    qCDebug(FTPManagerLog) << QString("_handlOpenFileROAck: _waitState(%1) _readFileLength(%3)").arg(MavlinkFTP::opCodeToString(_waitState)).arg(ack->openFileLength);
    
    if (_waitState != MavlinkFTP::kCmdOpenFileRO) {
        qCDebug(FTPManagerLog) << "Received OpenFileRO Ack while not waiting for it";
        return;
    }

    if (ack->hdr.size != sizeof(uint32_t)) {
        qCDebug(FTPManagerLog) << "_handlOpenFileROAck: ack->hdr.size != sizeof(uint32_t)" << ack->hdr.size << sizeof(uint32_t);
        _downloadComplete(tr("Download failed"));
        return;
    }

    _downloadState.reset();

    _waitState                          = MavlinkFTP::kCmdBurstReadFile;
    _activeSession                      = ack->hdr.session;
    _downloadState.fileSize             = ack->openFileLength;
    _downloadState.expectedBurstOffset  = 0;

    _downloadState.file.setFileName(_downloadState.toDir.filePath(_downloadState.fileName));
    if (!_downloadState.file.open(QFile::WriteOnly | QFile::Truncate)) {
        qCDebug(FTPManagerLog) << "_handlOpenFileROAck: _downloadState.file open failed" << _downloadState.file.errorString();
        _downloadComplete(tr("Download failed"));
        return;
    }

    MavlinkFTP::Request request;
    request.hdr.session = _activeSession;
    request.hdr.opcode  = MavlinkFTP::kCmdBurstReadFile;
    request.hdr.offset  = _downloadState.expectedBurstOffset;
    request.hdr.size    = sizeof(request.data);
    _sendRequestExpectAck(&request);
}

void FTPManager::_requestMissingBurstData()
{
    MavlinkFTP::Request request;

    if (_downloadState.missingData.count()) {
        MissingData_t& missingData = _downloadState.missingData.first();

        uint32_t cBytesToRead = qMin((uint32_t)sizeof(request.data), missingData.cBytes);

        qCDebug(FTPManagerLog) << "_requestMissingBurstData: offset:cBytesToRead" << missingData.offset << cBytesToRead;

        request.hdr.session                 = _activeSession;
        request.hdr.opcode                  = MavlinkFTP::kCmdReadFile;
        request.hdr.offset                  = missingData.offset;
        request.hdr.size                    = cBytesToRead;
        _waitState                          = MavlinkFTP::kCmdReadFile;
        _downloadState.retryCount           = 0;
        _downloadState.expectedReadOffset   = request.hdr.offset;

        if (cBytesToRead < missingData.cBytes) {
            missingData.offset += cBytesToRead;
            missingData.cBytes -= cBytesToRead;
        } else {
            _downloadState.missingData.takeFirst();
        }
    } else {
        qCDebug(FTPManagerLog) << "_requestMissingBurstData: starting next burst" << _downloadState.expectedBurstOffset;
        request.hdr.session = _activeSession;
        request.hdr.opcode  = MavlinkFTP::kCmdBurstReadFile;
        request.hdr.offset  = _downloadState.expectedBurstOffset;
        request.hdr.size    = sizeof(request.data);
        _waitState = MavlinkFTP::kCmdBurstReadFile;
    }

    _sendRequestExpectAck(&request);
}

/// Closes out a download session by writing the file and doing cleanup.
///     @param success true: successful download completion, false: error during download
void FTPManager::_downloadComplete(const QString& errorMsg)
{
    qCDebug(FTPManagerLog) << QString("_downloadComplete: errorMsg(%1)").arg(errorMsg);
    
    QString downloadFilePath    = _downloadState.toDir.absoluteFilePath(_downloadState.fileName);
    QString error               = errorMsg;

    _ackTimer.stop();
    _waitState = MavlinkFTP::kCmdNone;

    if (error.isEmpty()) {
        _downloadState.file.close();
    }
    
    _downloadState.reset();
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

// We only do read files to fill in holes from a burst read
void FTPManager::_handleReadFileAck(MavlinkFTP::Request* ack)
{
    if (ack->hdr.session != _activeSession) {
        return;
    }

    qCDebug(FTPManagerLog) << "_handleReadFileAck: offset:size" << ack->hdr.offset << ack->hdr.size;

    if (ack->hdr.offset != _downloadState.expectedReadOffset) {
        if (++_downloadState.retryCount > _maxRetry) {
            qCDebug(FTPManagerLog) << QString("_handleReadFileAck: retries exceeded");
            _downloadComplete(tr("Download failed: Unable to retrieve specified file contents"));
            return;
        }

        // Ask for current offset again
        qCDebug(FTPManagerLog) << QString("_handleReadFileAck: retry retryCount(%1) offset(%2)").arg(_downloadState.retryCount).arg(_downloadState.expectedReadOffset);
        MavlinkFTP::Request request;
        request.hdr.session = _activeSession;
        request.hdr.opcode  = _waitState;
        request.hdr.offset  = _downloadState.expectedReadOffset;
        request.hdr.size    = 0;
        _sendRequestExpectAck(&request);
        return;
    }

    _downloadState.file.seek(ack->hdr.offset);
    int bytesWritten = _downloadState.file.write((const char*)ack->data, ack->hdr.size);
    if (bytesWritten != ack->hdr.size) {
        _downloadComplete(tr("Download failed: Error saving file"));
        return;
    }
    _downloadState.bytesWritten += ack->hdr.size;
    
    if (_downloadState.fileSize != 0) {
        emit commandProgress(100 * ((float)(_downloadState.bytesWritten) / (float)_downloadState.fileSize));
    }

    // Move on to fill in possible next hole
    _requestMissingBurstData();
}

void FTPManager::_handleBurstReadFileAck(MavlinkFTP::Request* ack)
{
    if (ack->hdr.session != _activeSession) {
        return;
    }

    qCDebug(FTPManagerLog) << QString("_handleBurstReadFileAck: offset(%1) size(%2) burstComplete(%3)").arg(ack->hdr.offset).arg(ack->hdr.size).arg(ack->hdr.burstComplete);

    if (ack->hdr.offset != _downloadState.expectedBurstOffset) {
        if (ack->hdr.offset > _downloadState.expectedBurstOffset) {
            MissingData_t missingData;
            missingData.offset = _downloadState.expectedBurstOffset;
            missingData.cBytes = ack->hdr.offset - _downloadState.expectedBurstOffset;
            _downloadState.missingData.append(missingData);
            qCDebug(FTPManagerLog) << "_handleBurstReadFileAck: adding missing data offset:cBytes" << missingData.offset << missingData.cBytes;
        } else {
            qCDebug(FTPManagerLog) << "_handleBurstReadFileAck: received offset less than expected offset received:expected" << ack->hdr.offset << _downloadState.expectedBurstOffset;
            _ackTimer.start();
            return;
        }
    }

    _downloadState.file.seek(ack->hdr.offset);
    int bytesWritten = _downloadState.file.write((const char*)ack->data, ack->hdr.size);
    if (bytesWritten != ack->hdr.size) {
        _downloadComplete(tr("Download failed: Error saving file"));
        return;
    }
    _downloadState.bytesWritten += ack->hdr.size;
    _downloadState.expectedBurstOffset = ack->hdr.offset + ack->hdr.size;

    if (_downloadState.fileSize != 0) {
        emit commandProgress(100 * ((float)(_downloadState.bytesWritten) / (float)_downloadState.fileSize));
    }

    if (ack->hdr.burstComplete) {
        _requestMissingBurstData();
    } else {
        // Still within a burst, next ack should come automatically
        _ackTimer.start();
    }
}

void FTPManager::_listAckResponse(MavlinkFTP::Request* /*listAck*/)
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
void FTPManager::_createAckResponse(MavlinkFTP::Request* /*createAck*/)
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
void FTPManager::_writeAckResponse(MavlinkFTP::Request* /*writeAck*/)
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

void FTPManager::_handleAck(MavlinkFTP::Request* ack)
{

    switch (ack->hdr.req_opcode) {
    case MavlinkFTP::kCmdOpenFileRO:
        _handlOpenFileROAck(ack);
        break;
    case MavlinkFTP::kCmdReadFile:
        _handleReadFileAck(ack);
        break;
    case MavlinkFTP::kCmdBurstReadFile:
        _handleBurstReadFileAck(ack);
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
}

void FTPManager::_handleNak(MavlinkFTP::Request* nak)
{
    QString                 errorMsg;
    MavlinkFTP::OpCode_t    requestOpCode = static_cast<MavlinkFTP::OpCode_t>(nak->hdr.req_opcode);
    MavlinkFTP::ErrorCode_t errorCode = static_cast<MavlinkFTP::ErrorCode_t>(nak->data[0]);

    if (errorCode == MavlinkFTP::kErrEOF) {
        qCDebug(FTPManagerLog) << "_handleNak EOF";
        if (requestOpCode == MavlinkFTP::kCmdReadFile && _downloadState.bytesWritten == _downloadState.fileSize) {
            // This could be an EOF on a normal read sequence, or an EOF for a read to fill in holes from a burst read.
            // Either way it means we should be done.
            _downloadComplete(QString());
            return;
        } else if (requestOpCode == MavlinkFTP::kCmdBurstReadFile) {
            // This is an EOF during a burst read, we still have to check for filling in missing data
            if (_downloadState.missingData.count()) {
                // We only call _requestMissingBurstData if there are no missing blocks since _requestMissingBurstData will start a new
                // burst sequence if you call it with no missing blocks which would put us into an infinite loop on EOFs.
                _requestMissingBurstData();
                return;
            } else if (_downloadState.bytesWritten == _downloadState.fileSize) {
                _downloadComplete(QString());
                return;
            }
        }
    }

    // Nak's normally have 1 byte of data for error code, except for MavlinkFTP::kErrFailErrno which has additional byte for errno
    if ((errorCode == MavlinkFTP::kErrFailErrno && nak->hdr.size != 2) || ((errorCode != MavlinkFTP::kErrFailErrno) && nak->hdr.size != 1)) {
        errorMsg = tr("Invalid Nak format");
    } else if (errorCode == MavlinkFTP::kErrFailErrno) {
        errorMsg = tr("errno %1").arg(nak->data[1]);
    } else {
        errorMsg = MavlinkFTP::errorCodeToString(errorCode);
    }

    _waitState = MavlinkFTP::kCmdNone;

    switch (nak->hdr.req_opcode) {
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
        qCDebug(FTPManagerLog) << "Received old packet: expected seq:" << expectedSeqNumber << "got:" << incomingSeqNumber;
        return;
    }

    _ackTimer.stop();
    
    qCDebug(FTPManagerLog) << "mavlinkMessageReceived" << MavlinkFTP::opCodeToString(static_cast<MavlinkFTP::OpCode_t>(request->hdr.opcode)) <<  MavlinkFTP::opCodeToString(static_cast<MavlinkFTP::OpCode_t>(request->hdr.req_opcode));

    if (incomingSeqNumber != expectedSeqNumber) {
        switch (_waitState) {
        case MavlinkFTP::kCmdOpenFileRO:
            _downloadComplete(tr("Download failed: Unable to handle packet loss"));
            break;
        case MavlinkFTP::kCmdReadFile:
        case MavlinkFTP::kCmdBurstReadFile:
            // These can handle packet loss
            break;
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
        _handleAck(request);
    } else if (request->hdr.opcode == MavlinkFTP::kRspNak) {
        _handleNak(request);
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
    return _downloadWorker(from, toDir);
}

bool FTPManager::_downloadWorker(const QString& from, const QString& toDir)
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

    _downloadState.reset();
    _downloadState.toDir.setPath(toDir);

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

    _downloadState.fileName = strippedFrom.right(strippedFrom.size() - lastDirSlashIndex);
    _waitState              = MavlinkFTP::kCmdOpenFileRO;

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
void FTPManager::upload(const QString& /*toPath*/, const QFileInfo& /*uploadFile*/)
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

void FTPManager::createDirectory(const QString& /*directory*/)
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

void FTPManager::_ackTimeout(void)
{
    qCDebug(FTPManagerLog) << "_ackTimeout" << MavlinkFTP::opCodeToString(static_cast<MavlinkFTP::OpCode_t>(_waitState));

    switch (_waitState) {
    case MavlinkFTP::kCmdReadFile:
        // FIXME: retry count?
        // Resend last request
        _lastOutgoingRequest.hdr.seqNumber--;
        _sendRequestExpectAck(&_lastOutgoingRequest);
        return;
    default:
        break;
    }

#if 0
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

    // Make sure to set _currentOperation state before emitting error message. Code may respond
    // to error message signal by sending another command, which will fail if state is not back
    // to idle. FileView UI works this way with the List command.

    switch (_waitState) {
    case MavlinkFTP::kCmdOpenFileRO:
    case MavlinkFTP::kCmdBurstReadFile:
    case MavlinkFTP::kCmdReadFile:
        _downloadComplete(tr("Download failed: Vehicle did not respond to %1").arg(MavlinkFTP::opCodeToString(_waitState)));
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
    request.hdr.opcode  = MavlinkFTP::kCmdResetSessions;
    request.hdr.size    = 0;
    _waitState          = MavlinkFTP::kCmdResetSessions;
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
    _ackTimer.start();
    
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
