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

const MockMavlinkFileServer::ErrorMode_t MockMavlinkFileServer::rgFailureModes[] = {
    MockMavlinkFileServer::errModeNoResponse,
    MockMavlinkFileServer::errModeNakResponse,
    MockMavlinkFileServer::errModeNoSecondResponse,
    MockMavlinkFileServer::errModeNakSecondResponse,
    MockMavlinkFileServer::errModeBadCRC,
};
const size_t MockMavlinkFileServer::cFailureModes = sizeof(MockMavlinkFileServer::rgFailureModes) / sizeof(MockMavlinkFileServer::rgFailureModes[0]);

const MockMavlinkFileServer::FileTestCase MockMavlinkFileServer::rgFileTestCases[MockMavlinkFileServer::cFileTestCases] = {
    // File fits one Read Ack packet, partially filling data
    { "partial.qgc",    sizeof(((QGCUASFileManager::Request*)0)->data) - 1,     false },
    // File fits one Read Ack packet, exactly filling all data
    { "exact.qgc",      sizeof(((QGCUASFileManager::Request*)0)->data),         true },
    // File is larger than a single Read Ack packets, requires multiple Reads
    { "multi.qgc",      sizeof(((QGCUASFileManager::Request*)0)->data) + 1,     true },
};

// We only support a single fixed session
const uint8_t MockMavlinkFileServer::_sessionId = 1;

MockMavlinkFileServer::MockMavlinkFileServer(void) :
    _errMode(errModeNone)
{

}

/// @brief Handles List command requests. Only supports root folder paths.
///         File list returned is set using the setFileList method.
void MockMavlinkFileServer::_listCommand(QGCUASFileManager::Request* request)
{
    // FIXME: Does not support directories that span multiple packets
    
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
    ackResponse.hdr.offset = request->hdr.offset;
    ackResponse.hdr.size = 0;

    if (request->hdr.offset == 0) {
        // Requesting first batch of file names
        Q_ASSERT(_fileList.size());
        char *bufPtr = (char *)&ackResponse.data[0];
        for (int i=0; i<_fileList.size(); i++) {
            strcpy(bufPtr, _fileList[i].toStdString().c_str());
            size_t cchFilename = strlen(bufPtr);
			Q_ASSERT(cchFilename);
            ackResponse.hdr.size += cchFilename + 1;
            bufPtr += cchFilename + 1;
        }

        _emitResponse(&ackResponse);
    } else if (_errMode == errModeNakSecondResponse) {
        // Nak error all subsequent requests
        _sendNak(QGCUASFileManager::kErrPerm);
        return;
    } else if (_errMode == errModeNoSecondResponse) {
        // No response for all subsequent requests
        return;
    } else {
        // FIXME: Does not support directories that span multiple packets
        _sendNak(QGCUASFileManager::kErrEOF);
    }
}

/// @brief Handles Open command requests.
void MockMavlinkFileServer::_openCommand(QGCUASFileManager::Request* request)
{
    QGCUASFileManager::Request  response;
    QString                     path;
    
    size_t cchPath = strnlen((char *)request->data, sizeof(request->data));
    Q_ASSERT(cchPath != sizeof(request->data));
    Q_UNUSED(cchPath); // Fix initialized-but-not-referenced warning on release builds
    path = (char *)request->data;
    
    // Check path against one of our known test cases

    bool found = false;
    for (size_t i=0; i<cFileTestCases; i++) {
        if (path == rgFileTestCases[i].filename) {
            found = true;
            _readFileLength = rgFileTestCases[i].length;
            break;
        }
    }
    if (!found) {
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
    QGCUASFileManager::Request response;

    if (request->hdr.session != _sessionId) {
        _sendNak(QGCUASFileManager::kErrNoSession);
        return;
    }
    
    uint32_t readOffset = request->hdr.offset;  // offset into file for reading
    uint8_t cDataBytes = 0;                     // current number of data bytes used
    
    if (readOffset != 0) {
        // If we get here it means the client is requesting additional data past the first request
        if (_errMode == errModeNakSecondResponse) {
            // Nak error all subsequent requests
            _sendNak(QGCUASFileManager::kErrPerm);
            return;
        } else if (_errMode == errModeNoSecondResponse) {
            // No rsponse for all subsequent requests
            return;
        }
    }
    
    if (readOffset >= _readFileLength) {
        _sendNak(QGCUASFileManager::kErrEOF);
        return;
    }
    
    // Write file bytes. Data is a repeating sequence of 0x00, 0x01, .. 0xFF.
    for (; cDataBytes < sizeof(response.data) && readOffset < _readFileLength; readOffset++, cDataBytes++) {
        response.data[cDataBytes] = readOffset & 0xFF;
    }
    
    // We should always have written something, otherwise there is something wrong with the code above
    Q_ASSERT(cDataBytes);
    
    response.hdr.magic = 'f';
    response.hdr.session = _sessionId;
    response.hdr.size = cDataBytes;
    response.hdr.offset = request->hdr.offset;
    response.hdr.opcode = QGCUASFileManager::kRspAck;
    
    _emitResponse(&response);
}

/// @brief Handles Terminate commands
void MockMavlinkFileServer::_terminateCommand(QGCUASFileManager::Request* request)
{
    if (request->hdr.session != _sessionId) {
        _sendNak(QGCUASFileManager::kErrNoSession);
        return;
    }
    
    _sendAck();
    
    // Let our test harness know that we got a terminate command. This is used to validate the a Terminate is correctly
    // sent after an Open.
    emit terminateCommandReceived();
}

/// @brief Handles messages sent to the FTP server.
void MockMavlinkFileServer::sendMessage(mavlink_message_t message)
{
    QGCUASFileManager::Request ackResponse;

    Q_ASSERT(message.msgid == MAVLINK_MSG_ID_ENCAPSULATED_DATA);
    
    if (_errMode == errModeNoResponse) {
        // Don't respond to any requests, this shold cause the client to eventually timeout waiting for the ack
        return;
    } else if (_errMode == errModeNakResponse) {
        // Nak all requests, the actual error send back doesn't really matter as long as it's an error
        _sendNak(QGCUASFileManager::kErrPerm);
        return;
    }

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

        case QGCUASFileManager::kCmdTerminate:
            _terminateCommand(request);
            break;

        // Remainder of commands are NYI

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

/// @brief Sends an Ack
void MockMavlinkFileServer::_sendAck(void)
{
    QGCUASFileManager::Request ackResponse;
    
    ackResponse.hdr.magic = 'f';
    ackResponse.hdr.opcode = QGCUASFileManager::kRspAck;
    ackResponse.hdr.session = 0;
    ackResponse.hdr.size = 0;
    
    _emitResponse(&ackResponse);
}

/// @brief Sends a Nak with the specified error code.
void MockMavlinkFileServer::_sendNak(QGCUASFileManager::ErrorCode error)
{
    QGCUASFileManager::Request nakResponse;

    nakResponse.hdr.magic = 'f';
    nakResponse.hdr.opcode = QGCUASFileManager::kRspNak;
    nakResponse.hdr.session = 0;
    nakResponse.hdr.size = 1;
    nakResponse.data[0] = error;
    
    _emitResponse(&nakResponse);
}

/// @brief Emits a Request through the messageReceived signal.
void MockMavlinkFileServer::_emitResponse(QGCUASFileManager::Request* request)
{
    mavlink_message_t   mavlinkMessage;
    
    request->hdr.crc32 = QGCUASFileManager::crc32(request);
    if (_errMode == errModeBadCRC) {
        // Return a bad CRC
        request->hdr.crc32++;
    }
    
    mavlink_msg_encapsulated_data_pack(250, MAV_COMP_ID_IMU, &mavlinkMessage, 0 /*_encdata_seq*/, (uint8_t*)request);
    
    emit messageReceived(NULL, mavlinkMessage);
}
