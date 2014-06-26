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

#include "MockMavlinkFileServer.h"

const char* MockMavlinkFileServer::smallFilename = "small";
const char* MockMavlinkFileServer::largeFilename = "large";

// FIXME: -2 to avoid eof on full packet
const uint8_t MockMavlinkFileServer::smallFileLength = sizeof(((QGCUASFileManager::Request*)0)->data) - 2;
const uint8_t MockMavlinkFileServer::largeFileLength = sizeof(((QGCUASFileManager::Request*)0)->data) + 1;

const uint8_t MockMavlinkFileServer::_sessionId = 1;


/// @brief Handles List command requests. Only supports root folder paths.
///         File list returned is set using the setFileList method.
void MockMavlinkFileServer::_listCommand(QGCUASFileManager::Request* request)
{
    QGCUASFileManager::Request  ackResponse;
    QString                     path;

    // We only support root path
    path = (char *)&request->data[0];
    if (!path.isEmpty() && path != "/") {
        _sendNak(QGCUASFileManager::kErrNotDir);
        return;
    }
    
    // Offset requested is past the end of the list
    if (request->hdr.offset > (uint32_t)_fileList.size()) {
        _sendNak(QGCUASFileManager::kErrEOF);
        return;
    }
    
    ackResponse.hdr.magic = 'f';
    ackResponse.hdr.opcode = QGCUASFileManager::kRspAck;
    ackResponse.hdr.session = 0;
    ackResponse.hdr.size = 0;
    
    if (request->hdr.offset == 0) {
        // Requesting first batch of file names
        Q_ASSERT(_fileList.size());
        char *bufPtr = (char *)&ackResponse.data[0];
        for (int i=0; i<_fileList.size(); i++) {
            const char *filename = _fileList[i].toStdString().c_str();
            size_t cchFilename = strlen(filename);
            strcpy(bufPtr, filename);
            ackResponse.hdr.size += cchFilename + 1;
            bufPtr += cchFilename + 1;
        }
        
        // Final double termination
        *bufPtr = 0;
        ackResponse.hdr.size++;
        
    } else {
        // All filenames fit in first ack, send final null terminated ack
        ackResponse.data[0] = 0;
        ackResponse.hdr.size = 1;
    }
    
    _emitResponse(&ackResponse);
}

/// @brief Handles Open command requests. Two filenames are supported:
///         '/small' - file fits in a single packet
///         '/large' - file requires multiple packets to be sent
/// In all cases file contents are one byte data length, followed by a single
/// byte repeating increasing sequence (0x00, 0x01, .. 0xFF) for specified
/// number of bytes.
void MockMavlinkFileServer::_openCommand(QGCUASFileManager::Request* request)
{
    QGCUASFileManager::Request  response;
    QString                     path;
    
    // Make sure one byte of length is enough to overflow into two packets.
    Q_ASSERT((sizeof(request->data) & 0xFF) == sizeof(request->data));

    path = (char *)&request->data[0];
    if (path == smallFilename) {
        _readFileLength = smallFileLength;
        qDebug() << "Reading file length" << smallFileLength;
    } else if (path == largeFilename) {
        _readFileLength = largeFileLength;
        qDebug() << "Reading file length" << largeFileLength;
    } else {
        _sendNak(QGCUASFileManager::kErrNotFile);
        return;
    }
    
    response.hdr.magic = 'f';
    response.hdr.opcode = QGCUASFileManager::kRspAck;
    response.hdr.session = _sessionId;
    response.hdr.size = 0;
    
    _emitResponse(&response);
}

/// @brief Handles Read command requests.
void MockMavlinkFileServer::_readCommand(QGCUASFileManager::Request* request)
{
    qDebug() << "Read command:" << request->hdr.offset;
    
    QGCUASFileManager::Request response;

    if (request->hdr.session != _sessionId) {
        _sendNak(QGCUASFileManager::kErrNoSession);
        return;
    }
    
    if (request->hdr.size > sizeof(response.data)) {
        _sendNak(QGCUASFileManager::kErrPerm);
        return;
    }

    uint8_t size = 0;
    uint32_t offset = request->hdr.offset;
    
    // Offset check is > instead of >= to take into accoutn extra length byte at beginning of file
    if (offset > _readFileLength) {
        _sendNak(QGCUASFileManager::kErrIO);
        return;
    }
    
    // Write length byte if needed
    if (offset == 0) {
        response.data[0] = _readFileLength;
        offset++;
        size++;
    }
    
    // Write file bytes. Data is a repeating sequence of 0x00, 0x01, .. 0xFF.
    for (; size < sizeof(response.data) && offset <= _readFileLength; offset++, size++) {
        response.data[size] = (offset - 1) & 0xFF;
    }
    
    qDebug() << "_readCommand bytes written" << size;
    
    // If we didn't write any bytes it was a bad request
    if (size == 0) {
        _sendNak(QGCUASFileManager::kErrEOF);
        return;
    }
    
    response.hdr.magic = 'f';
    response.hdr.opcode = QGCUASFileManager::kRspAck;
    response.hdr.session = _sessionId;
    response.hdr.size = size;
    response.hdr.offset = request->hdr.offset;
    
    _emitResponse(&response);
}

/// @brief Handles messages sent to the FTP server.
void MockMavlinkFileServer::sendMessage(mavlink_message_t message)
{
    QGCUASFileManager::Request  ackResponse;

    Q_ASSERT(message.msgid == MAVLINK_MSG_ID_ENCAPSULATED_DATA);

    mavlink_encapsulated_data_t requestEncapsulatedData;
    mavlink_msg_encapsulated_data_decode(&message, &requestEncapsulatedData);
    QGCUASFileManager::Request* request = (QGCUASFileManager::Request*)&requestEncapsulatedData.data[0];
    
    // Validate CRC
    if (request->hdr.crc32 != QGCUASFileManager::crc32(request)) {
        _sendNak(QGCUASFileManager::kErrCrc);
    }

    switch (request->hdr.opcode) {
        case QGCUASFileManager::kCmdTestNoAck:
            // ignored, ack not sent back, for testing only
            break;
            
        case QGCUASFileManager::kCmdReset:
            // terminates all sessions
            // Fall through to send back Ack

        case QGCUASFileManager::kCmdNone:
            // ignored, always acked
            ackResponse.hdr.magic = 'f';
            ackResponse.hdr.opcode = QGCUASFileManager::kRspAck;
            ackResponse.hdr.session = 0;
            ackResponse.hdr.crc32 = 0;
            ackResponse.hdr.size = 0;
            _emitResponse(&ackResponse);
            break;

        case QGCUASFileManager::kCmdList:
            _listCommand(request);
            break;
            
        case QGCUASFileManager::kCmdOpen:
            _openCommand(request);
            break;

        case QGCUASFileManager::kCmdRead:
            _readCommand(request);
            break;

        // Remainder of commands are NYI

        case QGCUASFileManager::kCmdTerminate:
            // releases sessionID, closes file
        case QGCUASFileManager::kCmdCreate:
            // creates <path> for writing, returns <session>
        case QGCUASFileManager::kCmdWrite:
            // appends <size> bytes at <offset> in <session>
        case QGCUASFileManager::kCmdRemove:
            // remove file (only if created by server?)
        default:
            // nack for all NYI opcodes
            _sendNak(QGCUASFileManager::kErrUnknownCommand);
            break;
    }
}

/// @brief Sends a Nak with the specified error code.
void MockMavlinkFileServer::_sendNak(QGCUASFileManager::ErrorCode error)
{
    QGCUASFileManager::Request nakResponse;

    nakResponse.hdr.magic = 'f';
    nakResponse.hdr.opcode = QGCUASFileManager::kRspNak;
    nakResponse.hdr.session = 0;
    nakResponse.hdr.size = sizeof(nakResponse.data[0]);
    nakResponse.data[0] = error;
    
    _emitResponse(&nakResponse);
}

/// @brief Emits a Request through the messageReceived signal.
void MockMavlinkFileServer::_emitResponse(QGCUASFileManager::Request* request)
{
    mavlink_message_t   mavlinkMessage;
    
    request->hdr.crc32 = QGCUASFileManager::crc32(request);
    
    mavlink_msg_encapsulated_data_pack(250, 0, &mavlinkMessage, 0 /*_encdata_seq*/, (uint8_t*)request);
    
    emit messageReceived(NULL, mavlinkMessage);
}