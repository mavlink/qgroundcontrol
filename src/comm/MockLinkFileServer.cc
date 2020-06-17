/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "MockLinkFileServer.h"
#include "MockLink.h"

const MockLinkFileServer::ErrorMode_t MockLinkFileServer::rgFailureModes[] = {
    MockLinkFileServer::errModeNoResponse,
    MockLinkFileServer::errModeNakResponse,
    MockLinkFileServer::errModeNoSecondResponse,
    MockLinkFileServer::errModeNakSecondResponse,
    MockLinkFileServer::errModeBadSequence,
};
const size_t MockLinkFileServer::cFailureModes = sizeof(MockLinkFileServer::rgFailureModes) / sizeof(MockLinkFileServer::rgFailureModes[0]);

const MockLinkFileServer::FileTestCase MockLinkFileServer::rgFileTestCases[MockLinkFileServer::cFileTestCases] = {
    // File fits one Read Ack packet, partially filling data
    { "partial.qgc",    sizeof(((FileManager::Request*)0)->data) - 1,     1,    false},
    // File fits one Read Ack packet, exactly filling all data
    { "exact.qgc",      sizeof(((FileManager::Request*)0)->data),         1,    true },
    // File is larger than a single Read Ack packets, requires multiple Reads
    { "multi.qgc",      sizeof(((FileManager::Request*)0)->data) + 1,     2,    false },
};

// We only support a single fixed session
const uint8_t MockLinkFileServer::_sessionId = 1;

MockLinkFileServer::MockLinkFileServer(uint8_t systemIdServer, uint8_t componentIdServer, MockLink* mockLink) :
    _errMode(errModeNone),
    _systemIdServer(systemIdServer),
    _componentIdServer(componentIdServer),
    _mockLink(mockLink),
    _lastReplyValid(false),
    _lastReplySequence(0),
    _randomDropsEnabled(false)
{
    srand(0); // make sure unit tests are deterministic
}

void MockLinkFileServer::ensureNullTemination(FileManager::Request* request)
{
    if (request->hdr.size < sizeof(request->data)) {
        request->data[request->hdr.size] = '\0';

    } else {
        request->data[sizeof(request->data)-1] = '\0';
    }
}

/// @brief Handles List command requests. Only supports root folder paths.
///         File list returned is set using the setFileList method.
void MockLinkFileServer::_listCommand(uint8_t senderSystemId, uint8_t senderComponentId, FileManager::Request* request, uint16_t seqNumber)
{
    // FIXME: Does not support directories that span multiple packets
    
    FileManager::Request  ackResponse;
    QString                     path;
    uint16_t                    outgoingSeqNumber = _nextSeqNumber(seqNumber);

    ensureNullTemination(request);

    // We only support root path
    path = (char *)&request->data[0];
    if (!path.isEmpty() && path != "/") {
		_sendNak(senderSystemId, senderComponentId, FileManager::kErrFail, outgoingSeqNumber, FileManager::kCmdListDirectory);
        return;
    }
    
    // Offset requested is past the end of the list
    if (request->hdr.offset > (uint32_t)_fileList.size()) {
        _sendNak(senderSystemId, senderComponentId, FileManager::kErrEOF, outgoingSeqNumber, FileManager::kCmdListDirectory);
        return;
    }
    
    ackResponse.hdr.opcode = FileManager::kRspAck;
    ackResponse.hdr.req_opcode = FileManager::kCmdListDirectory;
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

        _sendResponse(senderSystemId, senderComponentId, &ackResponse, outgoingSeqNumber);
    } else if (_errMode == errModeNakSecondResponse) {
        // Nak error all subsequent requests
        _sendNak(senderSystemId, senderComponentId, FileManager::kErrFail, outgoingSeqNumber, FileManager::kCmdListDirectory);
        return;
    } else if (_errMode == errModeNoSecondResponse) {
        // No response for all subsequent requests
        return;
    } else {
        // FIXME: Does not support directories that span multiple packets
        _sendNak(senderSystemId, senderComponentId, FileManager::kErrEOF, outgoingSeqNumber, FileManager::kCmdListDirectory);
    }
}

/// @brief Handles Open command requests.
void MockLinkFileServer::_openCommand(uint8_t senderSystemId, uint8_t senderComponentId, FileManager::Request* request, uint16_t seqNumber)
{
    FileManager::Request  response;
    QString                     path;
    uint16_t                    outgoingSeqNumber = _nextSeqNumber(seqNumber);
    
    ensureNullTemination(request);

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
        _sendNak(senderSystemId, senderComponentId, FileManager::kErrFail, outgoingSeqNumber, FileManager::kCmdOpenFileRO);
        return;
    }
    
    response.hdr.opcode = FileManager::kRspAck;
	response.hdr.req_opcode = FileManager::kCmdOpenFileRO;
    response.hdr.session = _sessionId;
    
    // Data contains file length
    response.hdr.size = sizeof(uint32_t);
    response.openFileLength = _readFileLength;
    
    _sendResponse(senderSystemId, senderComponentId, &response, outgoingSeqNumber);
}

void MockLinkFileServer::_readCommand(uint8_t senderSystemId, uint8_t senderComponentId, FileManager::Request* request, uint16_t seqNumber)
{
    FileManager::Request	response;
    uint16_t				outgoingSeqNumber = _nextSeqNumber(seqNumber);

    if (request->hdr.session != _sessionId) {
		_sendNak(senderSystemId, senderComponentId, FileManager::kErrFail, outgoingSeqNumber, FileManager::kCmdReadFile);
        return;
    }
    
    uint32_t readOffset = request->hdr.offset;  // offset into file for reading
    uint8_t cDataBytes = 0;                     // current number of data bytes used
    
    if (readOffset != 0) {
        // If we get here it means the client is requesting additional data past the first request
        if (_errMode == errModeNakSecondResponse) {
            // Nak error all subsequent requests
            _sendNak(senderSystemId, senderComponentId, FileManager::kErrFail, outgoingSeqNumber, FileManager::kCmdReadFile);
            return;
        } else if (_errMode == errModeNoSecondResponse) {
            // No rsponse for all subsequent requests
            return;
        }
    }
    
    if (readOffset >= _readFileLength) {
        _sendNak(senderSystemId, senderComponentId, FileManager::kErrEOF, outgoingSeqNumber, FileManager::kCmdReadFile);
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

    _sendResponse(senderSystemId, senderComponentId, &response, outgoingSeqNumber);
}

void MockLinkFileServer::_streamCommand(uint8_t senderSystemId, uint8_t senderComponentId, FileManager::Request* request, uint16_t seqNumber)
{
    uint16_t                outgoingSeqNumber = _nextSeqNumber(seqNumber);
    FileManager::Request    response;

    if (request->hdr.session != _sessionId) {
		_sendNak(senderSystemId, senderComponentId, FileManager::kErrFail, outgoingSeqNumber, FileManager::kCmdBurstReadFile);
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
                _sendNak(senderSystemId, senderComponentId, FileManager::kErrFail, outgoingSeqNumber, FileManager::kCmdBurstReadFile);
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
        
        _sendResponse(senderSystemId, senderComponentId, &response, outgoingSeqNumber);
        
        outgoingSeqNumber = _nextSeqNumber(outgoingSeqNumber);
        ackOffset += cDataAck;
    }
	
    _sendNak(senderSystemId, senderComponentId, FileManager::kErrEOF, outgoingSeqNumber, FileManager::kCmdBurstReadFile);
}

void MockLinkFileServer::_terminateCommand(uint8_t senderSystemId, uint8_t senderComponentId, FileManager::Request* request, uint16_t seqNumber)
{
    uint16_t outgoingSeqNumber = _nextSeqNumber(seqNumber);

    if (request->hdr.session != _sessionId) {
		_sendNak(senderSystemId, senderComponentId, FileManager::kErrInvalidSession, outgoingSeqNumber, FileManager::kCmdTerminateSession);
        return;
    }
    
	_sendAck(senderSystemId, senderComponentId, outgoingSeqNumber, FileManager::kCmdTerminateSession);
	
    emit terminateCommandReceived();
}

void MockLinkFileServer::_resetCommand(uint8_t senderSystemId, uint8_t senderComponentId, uint16_t seqNumber)
{
    uint16_t outgoingSeqNumber = _nextSeqNumber(seqNumber);
    
    _sendAck(senderSystemId, senderComponentId, outgoingSeqNumber, FileManager::kCmdResetSessions);
    
    emit resetCommandReceived();
}

void MockLinkFileServer::handleFTPMessage(const mavlink_message_t& message)
{
    if (message.msgid != MAVLINK_MSG_ID_FILE_TRANSFER_PROTOCOL) {
        return;
    }
    
    FileManager::Request  ackResponse;

    mavlink_file_transfer_protocol_t requestFTP;
    mavlink_msg_file_transfer_protocol_decode(&message, &requestFTP);
    
    if (requestFTP.target_system != _systemIdServer) {
        return;
    }

    FileManager::Request* request = (FileManager::Request*)&requestFTP.payload[0];

	if (_randomDropsEnabled) {
	    if (rand() % 3 == 0) {
	        qDebug() << "FileServer: Random drop of incoming packet";
	        return;
	    }
	}

	if (_lastReplyValid && request->hdr.seqNumber + 1 == _lastReplySequence) {
	    // this is the same request as the one we replied to last. It means the (n)ack got lost, and the GCS
	    // resent the request
	    qDebug() << "FileServer: resending response";
	    _mockLink->respondWithMavlinkMessage(_lastReply);
	    return;
	}

    uint16_t incomingSeqNumber = request->hdr.seqNumber;
    uint16_t outgoingSeqNumber = _nextSeqNumber(incomingSeqNumber);
    
    if (request->hdr.opcode != FileManager::kCmdResetSessions && request->hdr.opcode != FileManager::kCmdTerminateSession) {
        if (_errMode == errModeNoResponse) {
            // Don't respond to any requests, this shold cause the client to eventually timeout waiting for the ack
            return;
        } else if (_errMode == errModeNakResponse) {
            // Nak all requests, the actual error send back doesn't really matter as long as it's an error
            _sendNak(message.sysid, message.compid, FileManager::kErrFail, outgoingSeqNumber, (FileManager::Opcode)request->hdr.opcode);
            return;
        }
    }

    switch (request->hdr.opcode) {
        case FileManager::kCmdTestNoAck:
            // ignored, ack not sent back, for testing only
            break;
            
        case FileManager::kCmdNone:
            // ignored, always acked
            ackResponse.hdr.opcode = FileManager::kRspAck;
            ackResponse.hdr.session = 0;
            ackResponse.hdr.size = 0;
            _sendResponse(message.sysid, message.compid, &ackResponse, outgoingSeqNumber);
            break;

        case FileManager::kCmdListDirectory:
            _listCommand(message.sysid, message.compid, request, incomingSeqNumber);
            break;
            
        case FileManager::kCmdOpenFileRO:
            _openCommand(message.sysid, message.compid, request, incomingSeqNumber);
            break;

        case FileManager::kCmdReadFile:
            _readCommand(message.sysid, message.compid, request, incomingSeqNumber);
            break;

        case FileManager::kCmdBurstReadFile:
            _streamCommand(message.sysid, message.compid, request, incomingSeqNumber);
            break;

        case FileManager::kCmdTerminateSession:
            _terminateCommand(message.sysid, message.compid, request, incomingSeqNumber);
            break;

        case FileManager::kCmdResetSessions:
            _resetCommand(message.sysid, message.compid, incomingSeqNumber);
            break;
            
        default:
            // nack for all NYI opcodes
            _sendNak(message.sysid, message.compid, FileManager::kErrUnknownCommand, outgoingSeqNumber, (FileManager::Opcode)request->hdr.opcode);
            break;
    }
}

/// @brief Sends an Ack
void MockLinkFileServer::_sendAck(uint8_t targetSystemId, uint8_t targetComponentId, uint16_t seqNumber, FileManager::Opcode reqOpcode)
{
    FileManager::Request ackResponse;
    
    ackResponse.hdr.opcode = FileManager::kRspAck;
	ackResponse.hdr.req_opcode = reqOpcode;
    ackResponse.hdr.session = 0;
    ackResponse.hdr.size = 0;
    
    _sendResponse(targetSystemId, targetComponentId, &ackResponse, seqNumber);
}

/// @brief Sends a Nak with the specified error code.
void MockLinkFileServer::_sendNak(uint8_t targetSystemId, uint8_t targetComponentId, FileManager::ErrorCode error, uint16_t seqNumber, FileManager::Opcode reqOpcode)
{
    FileManager::Request nakResponse;

    nakResponse.hdr.opcode = FileManager::kRspNak;
	nakResponse.hdr.req_opcode = reqOpcode;
    nakResponse.hdr.session = 0;
    nakResponse.hdr.size = 1;
    nakResponse.data[0] = error;
    
    _sendResponse(targetSystemId, targetComponentId, &nakResponse, seqNumber);
}

/// @brief Emits a Request through the messageReceived signal.
void MockLinkFileServer::_sendResponse(uint8_t targetSystemId, uint8_t targetComponentId, FileManager::Request* request, uint16_t seqNumber)
{
    request->hdr.seqNumber = seqNumber;
    _lastReplySequence = seqNumber;
    _lastReplyValid = true;
    
    mavlink_msg_file_transfer_protocol_pack_chan(_systemIdServer,    // System ID
                                                 0,                  // Component ID
                                                 _mockLink->mavlinkChannel(),
                                                 &_lastReply,    // Mavlink Message to pack into
                                                 0,                  // Target network
                                                 targetSystemId,
                                                 targetComponentId,
                                                 (uint8_t*)request); // Payload

	if (_randomDropsEnabled) {
	    if (rand() % 3 == 0) {
	        qDebug() << "FileServer: Random drop of outgoing packet";
	        return;
	    }
	}
    
    _mockLink->respondWithMavlinkMessage(_lastReply);
}

/// @brief Generates the next sequence number given an incoming sequence number. Handles generating
/// bad sequence numbers when errModeBadSequence is set.
uint16_t MockLinkFileServer::_nextSeqNumber(uint16_t seqNumber)
{
    uint16_t outgoingSeqNumber = seqNumber + 1;
    
    if (_errMode == errModeBadSequence) {
        outgoingSeqNumber++;
    }
    return outgoingSeqNumber;
}
