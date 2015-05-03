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
    MockMavlinkFileServer::errModeBadSequence,
};
const size_t MockMavlinkFileServer::cFailureModes = sizeof(MockMavlinkFileServer::rgFailureModes) / sizeof(MockMavlinkFileServer::rgFailureModes[0]);

const MockMavlinkFileServer::FileTestCase MockMavlinkFileServer::rgFileTestCases[MockMavlinkFileServer::cFileTestCases] = {
    // File fits one Read Ack packet, partially filling data
    { "partial.qgc",    sizeof(((FileManager::Request*)0)->data) - 1,     1,    false},
    // File fits one Read Ack packet, exactly filling all data
    { "exact.qgc",      sizeof(((FileManager::Request*)0)->data),         1,    true },
    // File is larger than a single Read Ack packets, requires multiple Reads
    { "multi.qgc",      sizeof(((FileManager::Request*)0)->data) + 1,     2,    false },
};

// We only support a single fixed session
const uint8_t MockMavlinkFileServer::_sessionId = 1;

MockMavlinkFileServer::MockMavlinkFileServer(uint8_t systemIdQGC, uint8_t systemIdServer) :
    _errMode(errModeNone),
    _systemIdServer(systemIdServer),
    _systemIdQGC(systemIdQGC)
{

}

/// @brief Handles List command requests. Only supports root folder paths.
///         File list returned is set using the setFileList method.
void MockMavlinkFileServer::_listCommand(FileManager::Request* request, uint16_t seqNumber)
{
    // FIXME: Does not support directories that span multiple packets
    
    FileManager::Request  ackResponse;
    QString                     path;
    uint16_t                    outgoingSeqNumber = _nextSeqNumber(seqNumber);

    // We only support root path
    path = (char *)&request->data[0];
    if (!path.isEmpty() && path != "/") {
		_sendNak(FileManager::kErrFail, outgoingSeqNumber, FileManager::kCmdListDirectory);
        return;
    }
    
    // Offset requested is past the end of the list
    if (request->hdr.offset > (uint32_t)_fileList.size()) {
        _sendNak(FileManager::kErrEOF, outgoingSeqNumber, FileManager::kCmdListDirectory);
        return;
    }
    
    ackResponse.hdr.opcode = FileManager::kRspAck;
    ackResponse.hdr.session = 0;
    ackResponse.hdr.offset = request->hdr.offset;
    ackResponse.hdr.size = 0;

    if (request->hdr.offset == 0) {
        // Requesting first batch of file names
        Q_ASSERT(_fileList.size());
        char *bufPtr = (char *)&ackResponse.data[0];
        for (int i=0; i<_fileList.size(); i++) {
            strcpy(bufPtr, _fileList[i].toStdString().c_str());
            uint8_t cchFilename = static_cast<uint8_t>(strlen(bufPtr));
            Q_ASSERT(cchFilename);
            ackResponse.hdr.size += cchFilename + 1;
            bufPtr += cchFilename + 1;
        }

        _emitResponse(&ackResponse, outgoingSeqNumber);
    } else if (_errMode == errModeNakSecondResponse) {
        // Nak error all subsequent requests
        _sendNak(FileManager::kErrFail, outgoingSeqNumber, FileManager::kCmdListDirectory);
        return;
    } else if (_errMode == errModeNoSecondResponse) {
        // No response for all subsequent requests
        return;
    } else {
        // FIXME: Does not support directories that span multiple packets
        _sendNak(FileManager::kErrEOF, outgoingSeqNumber, FileManager::kCmdListDirectory);
    }
}

/// @brief Handles Open command requests.
void MockMavlinkFileServer::_openCommand(FileManager::Request* request, uint16_t seqNumber)
{
    FileManager::Request  response;
    QString                     path;
    uint16_t                    outgoingSeqNumber = _nextSeqNumber(seqNumber);
    
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
        _sendNak(FileManager::kErrFail, outgoingSeqNumber, FileManager::kCmdOpenFileRO);
        return;
    }
    
    response.hdr.opcode = FileManager::kRspAck;
	response.hdr.req_opcode = FileManager::kCmdOpenFileRO;
    response.hdr.session = _sessionId;
    
    // Data contains file length
    response.hdr.size = sizeof(uint32_t);
    response.openFileLength = _readFileLength;
    
    _emitResponse(&response, outgoingSeqNumber);
}

void MockMavlinkFileServer::_readCommand(FileManager::Request* request, uint16_t seqNumber)
{
    FileManager::Request	response;
    uint16_t				outgoingSeqNumber = _nextSeqNumber(seqNumber);

    if (request->hdr.session != _sessionId) {
		_sendNak(FileManager::kErrFail, outgoingSeqNumber, FileManager::kCmdReadFile);
        return;
    }
    
    uint32_t readOffset = request->hdr.offset;  // offset into file for reading
    uint8_t cDataBytes = 0;                     // current number of data bytes used
    
    if (readOffset != 0) {
        // If we get here it means the client is requesting additional data past the first request
        if (_errMode == errModeNakSecondResponse) {
            // Nak error all subsequent requests
            _sendNak(FileManager::kErrFail, outgoingSeqNumber, FileManager::kCmdReadFile);
            return;
        } else if (_errMode == errModeNoSecondResponse) {
            // No rsponse for all subsequent requests
            return;
        }
    }
    
    if (readOffset >= _readFileLength) {
        _sendNak(FileManager::kErrEOF, outgoingSeqNumber, FileManager::kCmdReadFile);
        return;
    }
    
    // Write file bytes. Data is a repeating sequence of 0x00, 0x01, .. 0xFF.
    for (; cDataBytes < sizeof(response.data) && readOffset < _readFileLength; readOffset++, cDataBytes++) {
        response.data[cDataBytes] = readOffset & 0xFF;
    }
    
    // We should always have written something, otherwise there is something wrong with the code above
    Q_ASSERT(cDataBytes);
    
    response.hdr.session = _sessionId;
    response.hdr.size = cDataBytes;
    response.hdr.offset = request->hdr.offset;
    response.hdr.opcode = FileManager::kRspAck;
	response.hdr.req_opcode = FileManager::kCmdReadFile;

    _emitResponse(&response, outgoingSeqNumber);
}

void MockMavlinkFileServer::_streamCommand(FileManager::Request* request, uint16_t seqNumber)
{
    uint16_t                outgoingSeqNumber = _nextSeqNumber(seqNumber);
    FileManager::Request    response;

    if (request->hdr.session != _sessionId) {
		_sendNak(FileManager::kErrFail, outgoingSeqNumber, FileManager::kCmdBurstReadFile);
        return;
    }
    
    uint32_t readOffset = 0;	// offset into file for reading
    uint32_t ackOffset = 0;     // offset for ack
    uint8_t cDataAck;           // number of bytes in ack
    
    while (readOffset < _readFileLength) {
        cDataAck = 0;
        
        if (readOffset != 0) {
            // If we get here it means the client is requesting additional data past the first request
            if (_errMode == errModeNakSecondResponse) {
                // Nak error all subsequent requests
                _sendNak(FileManager::kErrFail, outgoingSeqNumber, FileManager::kCmdBurstReadFile);
                return;
            } else if (_errMode == errModeNoSecondResponse) {
                // No response for all subsequent requests
                return;
            }
        }
        
        // Write file bytes. Data is a repeating sequence of 0x00, 0x01, .. 0xFF.
        for (; cDataAck < sizeof(response.data) && readOffset < _readFileLength; readOffset++, cDataAck++) {
            response.data[cDataAck] = readOffset & 0xFF;
        }
        
        // We should always have written something, otherwise there is something wrong with the code above
        Q_ASSERT(cDataAck);
        
        response.hdr.session = _sessionId;
        response.hdr.size = cDataAck;
        response.hdr.offset = ackOffset;
        response.hdr.opcode = FileManager::kRspAck;
        response.hdr.req_opcode = FileManager::kCmdBurstReadFile;
        
        _emitResponse(&response, outgoingSeqNumber);
        
        outgoingSeqNumber = _nextSeqNumber(outgoingSeqNumber);
        ackOffset += cDataAck;
    }
	
    _sendNak(FileManager::kErrEOF, outgoingSeqNumber, FileManager::kCmdBurstReadFile);
}

/// @brief Handles Terminate commands
void MockMavlinkFileServer::_terminateCommand(FileManager::Request* request, uint16_t seqNumber)
{
    uint16_t outgoingSeqNumber = _nextSeqNumber(seqNumber);

    if (request->hdr.session != _sessionId) {
		_sendNak(FileManager::kErrInvalidSession, outgoingSeqNumber, FileManager::kCmdTerminateSession);
        return;
    }
    
	_sendAck(outgoingSeqNumber, FileManager::kCmdTerminateSession);
	
    // Let our test harness know that we got a terminate command. This is used to validate the a Terminate is correctly
    // sent after an Open.
    emit terminateCommandReceived();
}

/// @brief Handles messages sent to the FTP server.
void MockMavlinkFileServer::sendMessage(mavlink_message_t message)
{
    FileManager::Request  ackResponse;

    Q_ASSERT(message.msgid == MAVLINK_MSG_ID_FILE_TRANSFER_PROTOCOL);
    
    mavlink_file_transfer_protocol_t requestFileTransferProtocol;
    mavlink_msg_file_transfer_protocol_decode(&message, &requestFileTransferProtocol);
    FileManager::Request* request = (FileManager::Request*)&requestFileTransferProtocol.payload[0];

    Q_ASSERT(requestFileTransferProtocol.target_system == _systemIdServer);
    
    uint16_t incomingSeqNumber = request->hdr.seqNumber;
    uint16_t outgoingSeqNumber = _nextSeqNumber(incomingSeqNumber);
    
    if (_errMode == errModeNoResponse) {
        // Don't respond to any requests, this shold cause the client to eventually timeout waiting for the ack
        return;
    } else if (_errMode == errModeNakResponse) {
        // Nak all requests, the actual error send back doesn't really matter as long as it's an error
		_sendNak(FileManager::kErrFail, outgoingSeqNumber, (FileManager::Opcode)request->hdr.opcode);
        return;
    }

    switch (request->hdr.opcode) {
        case FileManager::kCmdTestNoAck:
            // ignored, ack not sent back, for testing only
            break;
            
        case FileManager::kCmdResetSessions:
            // terminates all sessions
            // Fall through to send back Ack

        case FileManager::kCmdNone:
            // ignored, always acked
            ackResponse.hdr.opcode = FileManager::kRspAck;
            ackResponse.hdr.session = 0;
            ackResponse.hdr.size = 0;
            _emitResponse(&ackResponse, outgoingSeqNumber);
            break;

        case FileManager::kCmdListDirectory:
            _listCommand(request, incomingSeqNumber);
            break;
            
        case FileManager::kCmdOpenFileRO:
            _openCommand(request, incomingSeqNumber);
            break;

        case FileManager::kCmdReadFile:
            _readCommand(request, incomingSeqNumber);
            break;

        case FileManager::kCmdBurstReadFile:
            _streamCommand(request, incomingSeqNumber);
            break;

        case FileManager::kCmdTerminateSession:
            _terminateCommand(request, incomingSeqNumber);
            break;

        default:
            // nack for all NYI opcodes
            _sendNak(FileManager::kErrUnknownCommand, outgoingSeqNumber, (FileManager::Opcode)request->hdr.opcode);
            break;
    }
}

/// @brief Sends an Ack
void MockMavlinkFileServer::_sendAck(uint16_t seqNumber, FileManager::Opcode reqOpcode)
{
    FileManager::Request ackResponse;
    
    ackResponse.hdr.opcode = FileManager::kRspAck;
	ackResponse.hdr.req_opcode = reqOpcode;
    ackResponse.hdr.session = 0;
    ackResponse.hdr.size = 0;
    
    _emitResponse(&ackResponse, seqNumber);
}

/// @brief Sends a Nak with the specified error code.
void MockMavlinkFileServer::_sendNak(FileManager::ErrorCode error, uint16_t seqNumber, FileManager::Opcode reqOpcode)
{
    FileManager::Request nakResponse;

    nakResponse.hdr.opcode = FileManager::kRspNak;
	nakResponse.hdr.req_opcode = reqOpcode;
    nakResponse.hdr.session = 0;
    nakResponse.hdr.size = 1;
    nakResponse.data[0] = error;
    
    _emitResponse(&nakResponse, seqNumber);
}

/// @brief Emits a Request through the messageReceived signal.
void MockMavlinkFileServer::_emitResponse(FileManager::Request* request, uint16_t seqNumber)
{
    mavlink_message_t   mavlinkMessage;
    
    request->hdr.seqNumber = seqNumber;
    
    mavlink_msg_file_transfer_protocol_pack(_systemIdServer,    // System ID
                                            0,                  // Component ID
                                            &mavlinkMessage,    // Mavlink Message to pack into
                                            0,                  // Target network
                                            _systemIdQGC,       // QGC Target System ID
                                            0,                  // Target component
                                            (uint8_t*)request); // Payload
    
    emit messageReceived(NULL, mavlinkMessage);
}

/// @brief Generates the next sequence number given an incoming sequence number. Handles generating
/// bad sequence numbers when errModeBadSequence is set.
uint16_t MockMavlinkFileServer::_nextSeqNumber(uint16_t seqNumber)
{
    uint16_t outgoingSeqNumber = seqNumber + 1;
    
    if (_errMode == errModeBadSequence) {
        outgoingSeqNumber++;
    }
    return outgoingSeqNumber;
}
