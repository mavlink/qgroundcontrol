/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "FileManager.h"
#include "QGC.h"
#include "MAVLinkProtocol.h"
#include "Vehicle.h"
#include "QGCApplication.h"

#include <QFile>
#include <QDir>
#include <string>

QGC_LOGGING_CATEGORY(FileManagerLog, "FileManagerLog")

FileManager::FileManager(QObject* parent, Vehicle* vehicle)
    : QObject(parent)
    , _currentOperation(kCOIdle)
    , _vehicle(vehicle)
    , _dedicatedLink(nullptr)
    , _activeSession(0)
    , _missingDownloadedBytes(0)
    , _downloadingMissingParts(false)
    , _systemIdQGC(0)
{
    connect(&_ackTimer, &QTimer::timeout, this, &FileManager::_ackTimeout);
    
    _lastOutgoingRequest.hdr.seqNumber = 0;

    _systemIdServer = _vehicle->id();
    
    // Make sure we don't have bad structure packing
    Q_ASSERT(sizeof(RequestHeader) == 12);
}

/// Respond to the Ack associated with the Open command with the next read command.
void FileManager::_openAckResponse(Request* openAck)
{
    qCDebug(FileManagerLog) << QString("_openAckResponse: _currentOperation(%1) _readFileLength(%2)").arg(_currentOperation).arg(openAck->openFileLength);
    
	Q_ASSERT(_currentOperation == kCOOpenRead || _currentOperation == kCOOpenBurst);
	_currentOperation = _currentOperation == kCOOpenRead ? kCORead : kCOBurst;
    _activeSession = openAck->hdr.session;
    
    // File length comes back in data
    Q_ASSERT(openAck->hdr.size == sizeof(uint32_t));
    _downloadFileSize = openAck->openFileLength;
    
    // Start the sequence of read commands

    _downloadOffset = 0;            // Start reading at beginning of file
    _readFileAccumulator.clear();   // Start with an empty file
    _missingDownloadedBytes = 0;
    _downloadingMissingParts = false;
    _missingData.clear();

    Request request;
    request.hdr.session = _activeSession;
	Q_ASSERT(_currentOperation == kCORead || _currentOperation == kCOBurst);
	request.hdr.opcode = _currentOperation == kCORead ? kCmdReadFile : kCmdBurstReadFile;
    request.hdr.offset = _downloadOffset;
    request.hdr.size = sizeof(request.data);

    _sendRequest(&request);
}

/// request the next missing part of a (partially) downloaded file
void FileManager::_requestMissingData()
{
    if (_missingData.empty()) {
        _downloadingMissingParts = false;
        _missingDownloadedBytes = 0;
        // there might be more data missing at the end
        if ((uint32_t)_readFileAccumulator.length() != _downloadFileSize) {
            _downloadOffset = _readFileAccumulator.length();
            qCDebug(FileManagerLog) << QString("_requestMissingData: missing parts done, downloadOffset(%1) downloadFileSize(%2)")
                    .arg(_downloadOffset).arg(_downloadFileSize);
        } else {
            _closeDownloadSession(true);
            return;
        }
    } else {
        qCDebug(FileManagerLog) << QString("_requestMissingData: offset(%1) size(%2)").arg(_missingData.head().offset).arg(_missingData.head().size);

        _downloadOffset = _missingData.head().offset;
    }

    Request request;
    _currentOperation = kCORead;
    request.hdr.session = _activeSession;
    request.hdr.opcode = kCmdReadFile;
    request.hdr.offset = _downloadOffset;
    request.hdr.size = sizeof(request.data);

    _sendRequest(&request);
}

/// Closes out a download session by writing the file and doing cleanup.
///     @param success true: successful download completion, false: error during download
void FileManager::_closeDownloadSession(bool success)
{
    qCDebug(FileManagerLog) << QString("_closeDownloadSession: success(%1) missingBytes(%2)").arg(success).arg(_missingDownloadedBytes);
    
    _currentOperation = kCOIdle;
    
    if (success) {
        if (_missingDownloadedBytes > 0 || (uint32_t)_readFileAccumulator.length() < _downloadFileSize) {
            // we're not done yet: request the missing parts individually (either we had missing parts or
            // the last (few) packets right before the EOF got dropped)
            _downloadingMissingParts = true;
            _requestMissingData();
            return;
        }

        QString downloadFilePath = _readFileDownloadDir.absoluteFilePath(_readFileDownloadFilename);

        QFile file(downloadFilePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            _emitErrorMessage(tr("Unable to open local file for writing (%1)").arg(downloadFilePath));
            return;
        }

        qint64 bytesWritten = file.write((const char *)_readFileAccumulator, _readFileAccumulator.length());
        if (bytesWritten != _readFileAccumulator.length()) {
            file.close();
            _emitErrorMessage(tr("Unable to write data to local file (%1)").arg(downloadFilePath));
            return;
        }
        file.close();

        emit commandComplete();
    }
    
    _readFileAccumulator.clear();
    
    // Close the open session
    _sendResetCommand();
}

/// Closes out an upload session doing cleanup.
///     @param success true: successful upload completion, false: error during download
void FileManager::_closeUploadSession(bool success)
{
    qCDebug(FileManagerLog) << QString("_closeUploadSession: success(%1)").arg(success);
    
    _currentOperation = kCOIdle;
    _writeFileAccumulator.clear();
    _writeFileSize = 0;
    
    if (success) {
        emit commandComplete();
    }
    
    // Close the open session
    _sendResetCommand();
}

/// Respond to the Ack associated with the Read or Stream commands.
///		@param readFile: true: read file, false: stream file
void FileManager::_downloadAckResponse(Request* readAck, bool readFile)
{
    if (readAck->hdr.session != _activeSession) {
        _closeDownloadSession(false /* failure */);
        _emitErrorMessage(tr("Download: Incorrect session returned"));
        return;
    }

    if (readAck->hdr.offset != _downloadOffset) {
        if (readFile) {
            _closeDownloadSession(false /* failure */);
            _emitErrorMessage(tr("Download: Offset returned (%1) differs from offset requested/expected (%2)").arg(readAck->hdr.offset).arg(_downloadOffset));
            return;
        } else { // burst
            if (readAck->hdr.offset < _downloadOffset) { // old data: ignore it
                _setupAckTimeout();
                return;
            }
            // keep track of missing data chunks
            MissingData missingData;
            missingData.offset = _downloadOffset;
            missingData.size = readAck->hdr.offset - _downloadOffset;
            _missingData.push_back(missingData);
            _missingDownloadedBytes += readAck->hdr.offset - _downloadOffset;
            qCDebug(FileManagerLog) << QString("_downloadAckResponse: missing data: offset(%1) size(%2)").arg(missingData.offset).arg(missingData.size);
            _downloadOffset = readAck->hdr.offset;
            _readFileAccumulator.resize(_downloadOffset); // placeholder for the missing data
        }
    }
    
    qCDebug(FileManagerLog) << QString("_downloadAckResponse: offset(%1) size(%2) burstComplete(%3)").arg(readAck->hdr.offset).arg(readAck->hdr.size).arg(readAck->hdr.burstComplete);

    if (_downloadingMissingParts) {
        Q_ASSERT(_missingData.head().offset == _downloadOffset);
        _missingDownloadedBytes -= readAck->hdr.size;
        _readFileAccumulator.replace(_downloadOffset, readAck->hdr.size, (const char*)readAck->data, readAck->hdr.size);
        if (_missingData.head().size <= readAck->hdr.size) {
            _missingData.pop_front();
        } else {
            _missingData.head().size -= readAck->hdr.size;
            _missingData.head().offset += readAck->hdr.size;
        }
    } else {
        _downloadOffset += readAck->hdr.size;
        _readFileAccumulator.append((const char*)readAck->data, readAck->hdr.size);
    }
    
    if (_downloadFileSize != 0) {
        emit commandProgress(100 * ((float)(_readFileAccumulator.length() - _missingDownloadedBytes) / (float)_downloadFileSize));
    }

    if (_downloadingMissingParts) {
        _requestMissingData();
    } else if (readFile || readAck->hdr.burstComplete) {
        // Possibly still more data to read, send next read request

        Request request;
        request.hdr.session = _activeSession;
        request.hdr.opcode = readFile ? kCmdReadFile : kCmdBurstReadFile;
        request.hdr.offset = _downloadOffset;
        request.hdr.size = 0;

        _sendRequest(&request);
    } else if (!readFile) {
        // Streaming, so next ack should come automatically
        _setupAckTimeout();
    }
}

/// @brief Respond to the Ack associated with the List command.
void FileManager::_listAckResponse(Request* listAck)
{
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
        Q_ASSERT(listAck->hdr.opcode == kRspAck);
        _currentOperation = kCOIdle;
        emit commandComplete();
    } else {
        // Possibly more entries to come, need to keep trying till we get EOF
        _currentOperation = kCOList;
        _listOffset += cListEntries;
        _sendListCommand();
    }
}

/// @brief Respond to the Ack associated with the create command.
void FileManager::_createAckResponse(Request* createAck)
{
    qCDebug(FileManagerLog) << "_createAckResponse";
    
    _currentOperation = kCOWrite;
    _activeSession = createAck->hdr.session;

    // Start the sequence of write commands from the beginning of the file

    _writeOffset = 0;
    _writeSize = 0;
    
    _writeFileDatablock();
}

/// @brief Respond to the Ack associated with the write command.
void FileManager::_writeAckResponse(Request* writeAck)
{
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
}

/// @brief Send next write file data block.
void FileManager::_writeFileDatablock(void)
{
    if (_writeOffset + _writeSize >= _writeFileSize){
        _closeUploadSession(true /* success */);
        return;
    }

    _writeOffset += _writeSize;

    Request request;
    request.hdr.session = _activeSession;
    request.hdr.opcode = kCmdWriteFile;
    request.hdr.offset = _writeOffset;

    if(_writeFileSize -_writeOffset > sizeof(request.data) )
        _writeSize = sizeof(request.data);
    else
        _writeSize = _writeFileSize - _writeOffset;

    request.hdr.size = _writeSize;

    memcpy(request.data, &_writeFileAccumulator.data()[_writeOffset], _writeSize);

    _sendRequest(&request);
}

void FileManager::receiveMessage(mavlink_message_t message)
{
    // receiveMessage is signalled will all mavlink messages so we need to filter everything else out but ours.
    if (message.msgid != MAVLINK_MSG_ID_FILE_TRANSFER_PROTOCOL) {
        return;
    }

    mavlink_file_transfer_protocol_t data;
    mavlink_msg_file_transfer_protocol_decode(&message, &data);
	
    // Make sure we are the target system
    if (data.target_system != _systemIdQGC) {
        qDebug() << "Received MAVLINK_MSG_ID_FILE_TRANSFER_PROTOCOL with incorrect target_system:" <<  data.target_system << "expected:" << _systemIdQGC;
        return;
    }
    
    Request* request = (Request*)&data.payload[0];

    uint16_t incomingSeqNumber = request->hdr.seqNumber;
    
    // Make sure we have a good sequence number
    uint16_t expectedSeqNumber = _lastOutgoingRequest.hdr.seqNumber + 1;

    // ignore old/reordered packets (handle wrap-around properly)
    if ((uint16_t)((expectedSeqNumber - 1) - incomingSeqNumber) < (std::numeric_limits<uint16_t>::max()/2)) {
        qDebug() << "Received old packet: expected seq:" << expectedSeqNumber << "got:" << incomingSeqNumber;
        return;
    }

    _clearAckTimeout();
    
	qCDebug(FileManagerLog) << "receiveMessage" << request->hdr.opcode;
	
    if (incomingSeqNumber != expectedSeqNumber) {
        bool doAbort = true;
        switch (_currentOperation) {
            case kCOBurst: // burst download drops are handled in _downloadAckResponse()
                doAbort = false;
                break;
            case kCORead:
                _closeDownloadSession(false /* failure */);
                break;
            
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
                
            default:
                // Don't need to do anything special
                _currentOperation = kCOIdle;
                break;
        }
        
        if (doAbort) {
            _emitErrorMessage(tr("Bad sequence number on received message: expected(%1) received(%2)").arg(expectedSeqNumber).arg(incomingSeqNumber));
            return;
        }
    }
    
    // Move past the incoming sequence number for next request
    _lastOutgoingRequest.hdr.seqNumber = incomingSeqNumber;

    if (request->hdr.opcode == kRspAck) {
        switch (request->hdr.req_opcode) {
			case kCmdListDirectory:
				_listAckResponse(request);
				break;
				
			case kCmdOpenFileRO:
            case kCmdOpenFileWO:
				_openAckResponse(request);
				break;
				
			case kCmdReadFile:
				_downloadAckResponse(request, true /* read file */);
				break;
				
			case kCmdBurstReadFile:
				_downloadAckResponse(request, false /* stream file */);
				break;
				
            case kCmdCreateFile:
                _createAckResponse(request);
                break;
                
            case kCmdWriteFile:
                _writeAckResponse(request);
                break;
                
			default:
				// Ack back from operation which does not require additional work
				_currentOperation = kCOIdle;
				break;
		}
    } else if (request->hdr.opcode == kRspNak) {
        uint8_t errorCode = request->data[0];

        // Nak's normally have 1 byte of data for error code, except for kErrFailErrno which has additional byte for errno
        Q_ASSERT((errorCode == kErrFailErrno && request->hdr.size == 2) || request->hdr.size == 1);
        
        _currentOperation = kCOIdle;

        if (request->hdr.req_opcode == kCmdListDirectory && errorCode == kErrEOF) {
            // This is not an error, just the end of the list loop
            emit commandComplete();
            return;
        } else if ((request->hdr.req_opcode == kCmdReadFile || request->hdr.req_opcode == kCmdBurstReadFile) && errorCode == kErrEOF) {
            // This is not an error, just the end of the download loop
            _closeDownloadSession(true /* success */);
            return;
        } else if (request->hdr.req_opcode == kCmdCreateFile) {
            _emitErrorMessage(tr("Nak received creating file, error: %1").arg(errorString(request->data[0])));
            return;
        } else if (request->hdr.req_opcode == kCmdCreateDirectory) {
            _emitErrorMessage(tr("Nak received creating directory, error: %1").arg(errorString(request->data[0])));
            return;
        } else {
            // Generic Nak handling
            if (request->hdr.req_opcode == kCmdReadFile || request->hdr.req_opcode == kCmdBurstReadFile) {
                // Nak error during download loop, download failed
                _closeDownloadSession(false /* failure */);
            } else if (request->hdr.req_opcode == kCmdWriteFile) {
                // Nak error during upload loop, upload failed
                _closeUploadSession(false /* failure */);
            }
            _emitErrorMessage(tr("Nak received, error: %1").arg(errorString(request->data[0])));
        }
    } else {
        // Note that we don't change our operation state. If something goes wrong beyond this, the operation
        // will time out.
        _emitErrorMessage(tr("Unknown opcode returned from server: %1").arg(request->hdr.opcode));
    }
}

void FileManager::listDirectory(const QString& dirPath)
{
    if (_currentOperation != kCOIdle) {
        _emitErrorMessage(tr("Command not sent. Waiting for previous command to complete."));
        return;
    }

    _dedicatedLink = _vehicle->priorityLink();
    if (!_dedicatedLink) {
        _emitErrorMessage(tr("Command not sent. No Vehicle links."));
        return;
    }

    // initialise the lister
    _listPath = dirPath;
    _listOffset = 0;
    _currentOperation = kCOList;

    // and send the initial request
    _sendListCommand();
}

void FileManager::_fillRequestWithString(Request* request, const QString& str)
{
    strncpy((char *)&request->data[0], str.toStdString().c_str(), sizeof(request->data));
    request->hdr.size = static_cast<uint8_t>(strnlen((const char *)&request->data[0], sizeof(request->data)));
}

void FileManager::_sendListCommand(void)
{
    Request request;

    request.hdr.session = 0;
    request.hdr.opcode = kCmdListDirectory;
    request.hdr.offset = _listOffset;
    request.hdr.size = 0;

    _fillRequestWithString(&request, _listPath);

    qCDebug(FileManagerLog) << "listDirectory: path:" << _listPath << "offset:" <<  _listOffset;
    
    _sendRequest(&request);
}

void FileManager::downloadPath(const QString& from, const QDir& downloadDir)
{
    if (_currentOperation != kCOIdle) {
        _emitErrorMessage(tr("Command not sent. Waiting for previous command to complete."));
        return;
    }

    _dedicatedLink = _vehicle->priorityLink();
    if (!_dedicatedLink) {
        _emitErrorMessage(tr("Command not sent. No Vehicle links."));
        return;
    }
    
	qCDebug(FileManagerLog) << "downloadPath from:" << from << "to:" << downloadDir;
	_downloadWorker(from, downloadDir, true /* read file */);
}

void FileManager::streamPath(const QString& from, const QDir& downloadDir)
{
    if (_currentOperation != kCOIdle) {
        _emitErrorMessage(tr("Command not sent. Waiting for previous command to complete."));
        return;
    }

    _dedicatedLink = _vehicle->priorityLink();
    if (!_dedicatedLink) {
        _emitErrorMessage(tr("Command not sent. No Vehicle links."));
        return;
    }
    
	qCDebug(FileManagerLog) << "streamPath from:" << from << "to:" << downloadDir;
	_downloadWorker(from, downloadDir, false /* stream file */);
}

void FileManager::_downloadWorker(const QString& from, const QDir& downloadDir, bool readFile)
{
	if (from.isEmpty()) {
		return;
	}
	
	_readFileDownloadDir.setPath(downloadDir.absolutePath());
	
	// We need to strip off the file name from the fully qualified path. We can't use the usual QDir
	// routines because this path does not exist locally.
	int i;
	for (i=from.size()-1; i>=0; i--) {
		if (from[i] == '/') {
			break;
		}
	}
	i++; // move past slash
	_readFileDownloadFilename = from.right(from.size() - i);
	
	_currentOperation = readFile ? kCOOpenRead : kCOOpenBurst;
	
	Request request;
	request.hdr.session = 0;
	request.hdr.opcode = kCmdOpenFileRO;
	request.hdr.offset = 0;
	request.hdr.size = 0;
	_fillRequestWithString(&request, from);
	_sendRequest(&request);
}

/// @brief Uploads the specified file.
///     @param toPath File in UAS to upload to, fully qualified path
///     @param uploadFile Local file to upload from
void FileManager::uploadPath(const QString& toPath, const QFileInfo& uploadFile)
{
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

    Request request;
    request.hdr.session = 0;
    request.hdr.opcode = kCmdCreateFile;
    request.hdr.offset = 0;
    request.hdr.size = 0;
    _fillRequestWithString(&request, toPath + "/" + uploadFile.fileName());
    _sendRequest(&request);
}

void FileManager::createDirectory(const QString& directory)
{
    if(_currentOperation != kCOIdle){
        _emitErrorMessage(tr("UAS File manager busy. Try again later"));
        return;
    }

    _currentOperation = kCOCreateDir;

    Request request;
    request.hdr.session = 0;
    request.hdr.opcode = kCmdCreateDirectory;
    request.hdr.offset = 0;
    request.hdr.size = 0;
    _fillRequestWithString(&request, directory);
    _sendRequest(&request);
}

QString FileManager::errorString(uint8_t errorCode)
{
    switch(errorCode) {
        case kErrNone:
            return QString("no error");
        case kErrFail:
            return QString("unknown error");
        case kErrEOF:
            return QString("read beyond end of file");
        case kErrUnknownCommand:
            return QString("unknown command");
        case kErrFailErrno:
            return QString("command failed");
        case kErrInvalidDataSize:
            return QString("invalid data size");
        case kErrInvalidSession:
            return QString("invalid session");
        case kErrNoSessionsAvailable:
            return QString("no sessions available");
        case kErrFailFileExists:
            return QString("File already exists on target");
        case kErrFailFileProtected:
            return QString("File is write protected");
        default:
            return QString("unknown error code");
    }
}

/// @brief Sends a command which only requires an opcode and no additional data
///     @param opcode Opcode to send
///     @param newOpState State to put state machine into
/// @return TRUE: command sent, FALSE: command not sent, waiting for previous command to finish
bool FileManager::_sendOpcodeOnlyCmd(uint8_t opcode, OperationState newOpState)
{
    if (_currentOperation != kCOIdle) {
        // Can't have multiple commands in play at the same time
        return false;
    }

    Request request;
    request.hdr.session = 0;
    request.hdr.opcode = opcode;
    request.hdr.offset = 0;
    request.hdr.size = 0;

    _currentOperation = newOpState;

    _sendRequest(&request);

    return true;
}

/// @brief Starts the ack timeout timer
void FileManager::_setupAckTimeout(void)
{
	qCDebug(FileManagerLog) << "_setupAckTimeout";

    Q_ASSERT(!_ackTimer.isActive());

    _ackNumTries = 0;
    _ackTimer.setSingleShot(false);
    _ackTimer.start(ackTimerTimeoutMsecs);
}

/// @brief Clears the ack timeout timer
void FileManager::_clearAckTimeout(void)
{
	qCDebug(FileManagerLog) << "_clearAckTimeout";
    _ackTimer.stop();
}

/// @brief Called when ack timeout timer fires
void FileManager::_ackTimeout(void)
{
    qCDebug(FileManagerLog) << "_ackTimeout";
    
    if (++_ackNumTries <= ackTimerMaxRetries) {
        qCDebug(FileManagerLog) << "ack timeout - retrying";
        if (_currentOperation == kCOBurst) {
            // for burst downloads try to initiate a new burst
            Request request;
            request.hdr.session = _activeSession;
            request.hdr.opcode = kCmdBurstReadFile;
            request.hdr.offset = _downloadOffset;
            request.hdr.size = 0;
            request.hdr.seqNumber = ++_lastOutgoingRequest.hdr.seqNumber;

            _sendRequestNoAck(&request);
        } else {
            _sendRequestNoAck(&_lastOutgoingRequest);
        }
        return;
    }

    _clearAckTimeout();

    // Make sure to set _currentOperation state before emitting error message. Code may respond
    // to error message signal by sending another command, which will fail if state is not back
    // to idle. FileView UI works this way with the List command.

    switch (_currentOperation) {
        case kCORead:
        case kCOBurst:
            _closeDownloadSession(false /* failure */);
            _emitErrorMessage(tr("Timeout waiting for ack: Download failed"));
            break;
            
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
			
        default:
        {
            OperationState currentOperation = _currentOperation;
            _currentOperation = kCOIdle;
            _emitErrorMessage(QString("Timeout waiting for ack: Command failed (%1)").arg(currentOperation));
        }
            break;
    }
}

void FileManager::_sendResetCommand(void)
{
    Request request;
    request.hdr.opcode = kCmdResetSessions;
    request.hdr.size = 0;
    _sendRequest(&request);
}

void FileManager::_emitErrorMessage(const QString& msg)
{
	qCDebug(FileManagerLog) << "Error:" << msg;
    emit commandError(msg);
}

void FileManager::_emitListEntry(const QString& entry)
{
    qCDebug(FileManagerLog) << "_emitListEntry" << entry;
    emit listEntry(entry);
}

/// @brief Sends the specified Request out to the UAS.
void FileManager::_sendRequest(Request* request)
{

    _setupAckTimeout();
    
    request->hdr.seqNumber = ++_lastOutgoingRequest.hdr.seqNumber;
    // store the current request
    if (request->hdr.size <= sizeof(request->data)) {
        memcpy(&_lastOutgoingRequest, request, sizeof(RequestHeader) + request->hdr.size);
    } else {
        qCCritical(FileManagerLog) << "request length too long:" << request->hdr.size;
    }
    
    qCDebug(FileManagerLog) << "_sendRequest opcode:" << request->hdr.opcode << "seqNumber:" << request->hdr.seqNumber;
    
    if (_systemIdQGC == 0) {
        _systemIdQGC = qgcApp()->toolbox()->mavlinkProtocol()->getSystemId();
    }
    _sendRequestNoAck(request);
}

/// @brief Sends the specified Request out to the UAS, without ack timeout handling
void FileManager::_sendRequestNoAck(Request* request)
{
    mavlink_message_t message;

    // Unit testing code can end up here without _dedicateLink set since it tests inidividual commands.
    LinkInterface* link;
    if (_dedicatedLink) {
        link = _dedicatedLink;
    } else {
        link = _vehicle->priorityLink();
    }
    
    mavlink_msg_file_transfer_protocol_pack_chan(_systemIdQGC,       // QGC System ID
                                                 0,                  // QGC Component ID
                                                 link->mavlinkChannel(),
                                                 &message,           // Mavlink Message to pack into
                                                 0,                  // Target network
                                                 _systemIdServer,    // Target system
                                                 0,                  // Target component
                                                 (uint8_t*)request); // Payload
    
    _vehicle->sendMessageOnLink(link, message);
}
