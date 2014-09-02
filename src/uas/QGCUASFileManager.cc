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

#include "QGCUASFileManager.h"
#include "QGC.h"
#include "MAVLinkProtocol.h"
#include "MainWindow.h"

#include <QFile>
#include <QDir>
#include <string>

static const quint32 crctab[] =
{
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};


QGCUASFileManager::QGCUASFileManager(QObject* parent, UASInterface* uas, uint8_t unitTestSystemIdQGC) :
    QObject(parent),
    _currentOperation(kCOIdle),
    _mav(uas),
    _lastOutgoingSeqNumber(0),
    _activeSession(0),
    _systemIdQGC(unitTestSystemIdQGC)
{
    connect(&_ackTimer, &QTimer::timeout, this, &QGCUASFileManager::_ackTimeout);
    
    _systemIdServer = _mav->getUASID();
    
    // Make sure we don't have bad structure packing
    Q_ASSERT(sizeof(RequestHeader) == 16);
}

/// @brief Calculates a 32 bit CRC for the specified request.
///     @param request Request to calculate CRC for. request->size must be set correctly.
///     @param state previous crc state
/// @return Calculated CRC
quint32 QGCUASFileManager::crc32(Request* request, unsigned state)
{
    uint8_t* data = (uint8_t*)request;
    size_t cbData = sizeof(RequestHeader) + request->hdr.size;

    // Always calculate CRC with 0 initial CRC value
    quint32 crcSave = request->hdr.crc32;
    request->hdr.crc32 = 0;
    request->hdr.padding[0] = 0;
    request->hdr.padding[1] = 0;
    request->hdr.padding[2] = 0;

    for (size_t i=0; i < cbData; i++)
        state = crctab[(state ^ data[i]) & 0xff] ^ (state >> 8);

    request->hdr.crc32 = crcSave;

    return state;
}

/// @brief Respond to the Ack associated with the Open command with the next Read command.
void QGCUASFileManager::_openAckResponse(Request* openAck)
{
    _currentOperation = kCORead;
    _activeSession = openAck->hdr.session;
    
    // File length comes back in data
    Q_ASSERT(openAck->hdr.size == sizeof(uint32_t));
    emit downloadFileLength(openAck->openFileLength);
    
    // Start the sequence of read commands

    _readOffset = 0;                // Start reading at beginning of file
    _readFileAccumulator.clear();   // Start with an empty file

    Request request;
    request.hdr.session = _activeSession;
    request.hdr.opcode = kCmdReadFile;
    request.hdr.offset = _readOffset;
    request.hdr.size = sizeof(request.data);

    _sendRequest(&request);
}

/// @brief Closes out a read session by writing the file and doing cleanup.
///     @param success true: successful download completion, false: error during download
void QGCUASFileManager::_closeReadSession(bool success)
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

/// @brief Respond to the Ack associated with the Read command.
void QGCUASFileManager::_readAckResponse(Request* readAck)
{
    if (readAck->hdr.session != _activeSession) {
        _currentOperation = kCOIdle;
        _readFileAccumulator.clear();
        _emitErrorMessage(tr("Read: Incorrect session returned"));
        return;
    }

    if (readAck->hdr.offset != _readOffset) {
        _currentOperation = kCOIdle;
        _readFileAccumulator.clear();
        _emitErrorMessage(tr("Read: Offset returned (%1) differs from offset requested (%2)").arg(readAck->hdr.offset).arg(_readOffset));
        return;
    }

    _readFileAccumulator.append((const char*)readAck->data, readAck->hdr.size);
    emit downloadFileProgress(_readFileAccumulator.length());

    if (readAck->hdr.size == sizeof(readAck->data)) {
        // Possibly still more data to read, send next read request

        _currentOperation = kCORead;

        _readOffset += readAck->hdr.size;

        Request request;
        request.hdr.session = _activeSession;
        request.hdr.opcode = kCmdReadFile;
        request.hdr.offset = _readOffset;
        request.hdr.size = 0;

        _sendRequest(&request);
    } else {
        // We only receieved a partial buffer back. These means we are at EOF
        _currentOperation = kCOIdle;
        _closeReadSession(true /* success */);
    }
}

/// @brief Respond to the Ack associated with the List command.
void QGCUASFileManager::_listAckResponse(Request* listAck)
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
        size_t nlen = strnlen(ptr, cBytesLeft);
        if (nlen < 2) {
            _currentOperation = kCOIdle;
            _emitErrorMessage(tr("Incorrectly formed list entry: '%1'").arg(ptr));
            return;
        } else if (nlen == cBytesLeft) {
            _currentOperation = kCOIdle;
            _emitErrorMessage(tr("Missing NULL termination in list entry"));
            return;
        }

        // Returned names are prepended with D for directory, F for file, U for unknown
        if (*ptr == 'F' || *ptr == 'D') {
            // put it in the view
            _emitListEntry(ptr);
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

void QGCUASFileManager::receiveMessage(LinkInterface* link, mavlink_message_t message)
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
        qDebug() << "Received MAVLINK_MSG_ID_FILE_TRANSFER_PROTOCOL with possibly incorrect system id" << _systemIdQGC;
        return;
    }
    
    Request* request = (Request*)&data.payload[0];
    
    _clearAckTimeout();
    
    uint16_t incomingSeqNumber = request->hdr.seqNumber;
    
    // Make sure we have a good CRC
    quint32 expectedCRC = crc32(request);
    quint32 receivedCRC = request->hdr.crc32;
    if (receivedCRC != expectedCRC) {
        _currentOperation = kCOIdle;
        _emitErrorMessage(tr("Bad CRC on received message: expected(%1) received(%2)").arg(expectedCRC).arg(receivedCRC));
        return;
    }
    
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

        switch (_currentOperation) {
            case kCOIdle:
                // we should not be seeing anything here.. shut the other guy up
                _sendCmdReset();
                break;

            case kCOAck:
                // We are expecting an ack back
                _currentOperation = kCOIdle;
                break;

            case kCOList:
                _listAckResponse(request);
                break;

            case kCOOpen:
                _openAckResponse(request);
                break;

            case kCORead:
                _readAckResponse(request);
                break;

            default:
                _emitErrorMessage(tr("Ack received in unexpected state"));
                break;
        }
    } else if (request->hdr.opcode == kRspNak) {
        Q_ASSERT(request->hdr.size == 1); // Should only have one byte of error code

        OperationState previousOperation = _currentOperation;
        uint8_t errorCode = request->data[0];

        _currentOperation = kCOIdle;

        if (previousOperation == kCOList && errorCode == kErrEOF) {
            // This is not an error, just the end of the read loop
            emit listComplete();
            return;
        } else if (previousOperation == kCORead && errorCode == kErrEOF) {
            // This is not an error, just the end of the read loop
            _closeReadSession(true /* success */);
            return;
        } else {
            // Generic Nak handling
            if (previousOperation == kCORead) {
                // Nak error during read loop, download failed
                _closeReadSession(false /* failure */);
            }
            _emitErrorMessage(tr("Nak received, error: %1").arg(errorString(request->data[0])));
        }
    } else {
        // Note that we don't change our operation state. If something goes wrong beyond this, the operation
        // will time out.
        _emitErrorMessage(tr("Unknown opcode returned from server: %1").arg(request->hdr.opcode));
    }
}

void QGCUASFileManager::listDirectory(const QString& dirPath)
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

void QGCUASFileManager::_fillRequestWithString(Request* request, const QString& str)
{
    strncpy((char *)&request->data[0], str.toStdString().c_str(), sizeof(request->data));
    request->hdr.size = strnlen((const char *)&request->data[0], sizeof(request->data));
}

void QGCUASFileManager::_sendListCommand(void)
{
    Request request;

    request.hdr.session = 0;
    request.hdr.opcode = kCmdListDirectory;
    request.hdr.offset = _listOffset;
    request.hdr.size = 0;

    _fillRequestWithString(&request, _listPath);

    _sendRequest(&request);
}

/// @brief Downloads the specified file.
///     @param from File to download from UAS, fully qualified path
///     @param downloadDir Local directory to download file to
void QGCUASFileManager::downloadPath(const QString& from, const QDir& downloadDir)
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

    _currentOperation = kCOOpen;

    Request request;
    request.hdr.session = 0;
    request.hdr.opcode = kCmdOpenFile;
    request.hdr.offset = 0;
    request.hdr.size = 0;
    _fillRequestWithString(&request, from);
    _sendRequest(&request);
}

QString QGCUASFileManager::errorString(uint8_t errorCode)
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
        case kErrCrc:
            return QString("bad crc");
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
bool QGCUASFileManager::_sendOpcodeOnlyCmd(uint8_t opcode, OperationState newOpState)
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
void QGCUASFileManager::_setupAckTimeout(void)
{
    Q_ASSERT(!_ackTimer.isActive());

    _ackTimer.setSingleShot(true);
    _ackTimer.start(ackTimerTimeoutMsecs);
}

/// @brief Clears the ack timeout timer
void QGCUASFileManager::_clearAckTimeout(void)
{
    _ackTimer.stop();
}

/// @brief Called when ack timeout timer fires
void QGCUASFileManager::_ackTimeout(void)
{
    // Make sure to set _currentOperation state before emitting error message. Code may respond
    // to error message signal by sending another command, which will fail if state is not back
    // to idle. FileView UI works this way with the List command.

    switch (_currentOperation) {
        case kCORead:
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

void QGCUASFileManager::_sendTerminateCommand(void)
{
    Request request;
    request.hdr.session = _activeSession;
    request.hdr.opcode = kCmdTerminateSession;
    request.hdr.size = 0;
    _sendRequest(&request);
}

void QGCUASFileManager::_emitErrorMessage(const QString& msg)
{
    qDebug() << "QGCUASFileManager: Error" << msg;
    emit errorMessage(msg);
}

void QGCUASFileManager::_emitListEntry(const QString& entry)
{
    qDebug() << "QGCUASFileManager: list entry" << entry;
    emit listEntry(entry);
}

/// @brief Sends the specified Request out to the UAS.
void QGCUASFileManager::_sendRequest(Request* request)
{
    mavlink_message_t message;

    _setupAckTimeout();
    
    _lastOutgoingSeqNumber++;

    request->hdr.seqNumber = _lastOutgoingSeqNumber;
    
    request->hdr.crc32 = crc32(request);
    
    if (_systemIdQGC == 0) {
        _systemIdQGC = MainWindow::instance()->getMAVLink()->getSystemId();
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
