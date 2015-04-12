/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

#include "FileManager.h"
#include "QGC.h"
#include "MAVLinkProtocol.h"
#include "MainWindow.h"

#include <QFile>
#include <QDir>
#include <string>

QGC_LOGGING_CATEGORY(FileManagerLog, "FileManagerLog")

FileManager::FileManager(QObject* parent, UASInterface* uas, uint8_t unitTestSystemIdQGC) :
    QObject(parent),
    _currentOperation(kCOIdle),
    _mav(uas),
    _lastOutgoingSeqNumber(0),
    _activeSession(0),
    _systemIdQGC(unitTestSystemIdQGC)
{
    connect(&_ackTimer, &QTimer::timeout, this, &FileManager::_ackTimeout);
    
    _systemIdServer = _mav->getUASID();
    
    // Make sure we don't have bad structure packing
    Q_ASSERT(sizeof(RequestHeader) == 12);
}

/// @brief Respond to the Ack associated with the Open command with the next Read command.
void FileManager::_openAckResponse(Request* openAck)
{
	Q_ASSERT(_currentOperation == kCOOpenRead || _currentOperation == kCOOpenStream);
	_currentOperation = _currentOperation == kCOOpenRead ? kCORead : kCOStream;
    _activeSession = openAck->hdr.session;
    
    // File length comes back in data
    Q_ASSERT(openAck->hdr.size == sizeof(uint32_t));
    emit downloadFileLength(openAck->openFileLength);
    
    // Start the sequence of read commands

    _downloadOffset = 0;                // Start reading at beginning of file
    _readFileAccumulator.clear();   // Start with an empty file

    Request request;
    request.hdr.session = _activeSession;
	Q_ASSERT(_currentOperation == kCORead || _currentOperation == kCOStream);
	request.hdr.opcode = _currentOperation == kCORead ? kCmdReadFile : kCmdStreamFile;
    request.hdr.offset = _downloadOffset;
    request.hdr.size = sizeof(request.data);

    _sendRequest(&request);
}

/// @brief Closes out a read session by writing the file and doing cleanup.
///     @param success true: successful download completion, false: error during download
void FileManager::_closeDownloadSession(bool success)
{
    if (success) {
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

        emit downloadFileComplete();
    }

    // Close the open session
    _sendTerminateCommand();
}

/// Respond to the Ack associated with the Read or Stream commands.
///		@param readFile: true: read file, false: stream file
void FileManager::_downloadAckResponse(Request* readAck, bool readFile)
{
    if (readAck->hdr.session != _activeSession) {
        _currentOperation = kCOIdle;
        _readFileAccumulator.clear();
        _emitErrorMessage(tr("Download: Incorrect session returned"));
        return;
    }

    if (readAck->hdr.offset != _downloadOffset) {
        _currentOperation = kCOIdle;
        _readFileAccumulator.clear();
        _emitErrorMessage(tr("Download: Offset returned (%1) differs from offset requested/expected (%2)").arg(readAck->hdr.offset).arg(_downloadOffset));
        return;
    }

	_downloadOffset += readAck->hdr.size;
    _readFileAccumulator.append((const char*)readAck->data, readAck->hdr.size);
    emit downloadFileProgress(_readFileAccumulator.length());

    if (readAck->hdr.size == sizeof(readAck->data)) {
		if (readFile) {
			// Possibly still more data to read, send next read request

			Request request;
			request.hdr.session = _activeSession;
			request.hdr.opcode = kCmdReadFile;
			request.hdr.offset = _downloadOffset;
			request.hdr.size = 0;

			_sendRequest(&request);
		} else {
			// Streaming, so next ack should come automatically
			_currentOperation = kCOStream;
			_setupAckTimeout();
		}
    } else if (readFile) {
        // We only receieved a partial buffer back. These means we are at EOF
        _currentOperation = kCOIdle;
        _closeDownloadSession(true /* success */);
    }
}

/// @brief Respond to the Ack associated with the List command.
void FileManager::_listAckResponse(Request* listAck)
{
    if (listAck->hdr.offset != _listOffset) {
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

    if (listAck->hdr.size == 0) {
        // Directory is empty, we're done
        Q_ASSERT(listAck->hdr.opcode == kRspAck);
        _currentOperation = kCOIdle;
        emit listComplete();
    } else {
        // Possibly more entries to come, need to keep trying till we get EOF
        _currentOperation = kCOList;
        _listOffset += cListEntries;
        _sendListCommand();
    }
}

void FileManager::receiveMessage(LinkInterface* link, mavlink_message_t message)
{
    Q_UNUSED(link);

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
    
    _clearAckTimeout();
    
	qCDebug(FileManagerLog) << "receiveMessage" << request->hdr.opcode;
	
    uint16_t incomingSeqNumber = request->hdr.seqNumber;
    
    // Make sure we have a good sequence number
    uint16_t expectedSeqNumber = _lastOutgoingSeqNumber + 1;
    if (incomingSeqNumber != expectedSeqNumber) {
        _currentOperation = kCOIdle;
        _emitErrorMessage(tr("Bad sequence number on received message: expected(%1) received(%2)").arg(expectedSeqNumber).arg(incomingSeqNumber));
        return;
    }
    
    // Move past the incoming sequence number for next request
    _lastOutgoingSeqNumber = incomingSeqNumber;

    if (request->hdr.opcode == kRspAck) {

        switch (request->hdr.req_opcode) {
			case kCmdListDirectory:
				_listAckResponse(request);
				break;
				
			case kCmdOpenFileRO:
				_openAckResponse(request);
				break;
				
			case kCmdReadFile:
				_downloadAckResponse(request, true /* read file */);
				break;
				
			case kCmdStreamFile:
				_downloadAckResponse(request, false /* stream file */);
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
            emit listComplete();
            return;
        } else if ((request->hdr.req_opcode == kCmdReadFile || request->hdr.req_opcode == kCmdStreamFile) && errorCode == kErrEOF) {
            // This is not an error, just the end of the download loop
            _closeDownloadSession(true /* success */);
            return;
        } else {
            // Generic Nak handling
            if (request->hdr.req_opcode == kCmdReadFile || request->hdr.req_opcode == kCmdStreamFile) {
                // Nak error during download loop, download failed
                _closeDownloadSession(false /* failure */);
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

    _sendRequest(&request);
}

void FileManager::downloadPath(const QString& from, const QDir& downloadDir)
{
	qCDebug(FileManagerLog) << "downloadPath from:" << from << "to:" << downloadDir;
	_downloadWorker(from, downloadDir, true /* read file */);
}

void FileManager::streamPath(const QString& from, const QDir& downloadDir)
{
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
	
	_currentOperation = readFile ? kCOOpenRead : kCOOpenStream;
	
	Request request;
	request.hdr.session = 0;
	request.hdr.opcode = kCmdOpenFileRO;
	request.hdr.offset = 0;
	request.hdr.size = 0;
	_fillRequestWithString(&request, from);
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
            return QString("no sessions availble");
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

    _ackTimer.setSingleShot(true);
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
    // Make sure to set _currentOperation state before emitting error message. Code may respond
    // to error message signal by sending another command, which will fail if state is not back
    // to idle. FileView UI works this way with the List command.

    switch (_currentOperation) {
        case kCORead:
			// FIXME: This seems strage? Doesn't close download session
            _currentOperation = kCOAck;
            _emitErrorMessage(tr("Timeout waiting for ack: Sending Terminate command"));
            _sendTerminateCommand();
            break;
			
        case kCOStream:
			// FIXME: This seems strage? Doesn't close download session
            _currentOperation = kCOAck;
            _emitErrorMessage(tr("Timeout waiting for ack: Sending Terminate command"));
            _sendTerminateCommand();
            break;
			
        default:
            _currentOperation = kCOIdle;
            _emitErrorMessage(tr("Timeout waiting for ack"));
            break;
    }
}

void FileManager::_sendTerminateCommand(void)
{
    Request request;
    request.hdr.session = _activeSession;
    request.hdr.opcode = kCmdTerminateSession;
    request.hdr.size = 0;
    _sendRequest(&request);
}

void FileManager::_emitErrorMessage(const QString& msg)
{
	qCDebug(FileManagerLog) << "Error:" << msg;
    emit errorMessage(msg);
}

void FileManager::_emitListEntry(const QString& entry)
{
    qCDebug(FileManagerLog) << "_emitListEntry" << entry;
    emit listEntry(entry);
}

/// @brief Sends the specified Request out to the UAS.
void FileManager::_sendRequest(Request* request)
{
	qCDebug(FileManagerLog) << "_sendRequest opcode:" << request->hdr.opcode;

    mavlink_message_t message;

    _setupAckTimeout();
    
    _lastOutgoingSeqNumber++;

    request->hdr.seqNumber = _lastOutgoingSeqNumber;
    
    if (_systemIdQGC == 0) {
        _systemIdQGC = MAVLinkProtocol::instance()->getSystemId();
    }
    
    Q_ASSERT(_mav);
    mavlink_msg_file_transfer_protocol_pack(_systemIdQGC,       // QGC System ID
                                            0,                  // QGC Component ID
                                            &message,           // Mavlink Message to pack into
                                            0,                  // Target network
                                            _systemIdServer,    // Target system
                                            0,                  // Target component
                                            (uint8_t*)request); // Payload
    
    _mav->sendMessage(message);
}
