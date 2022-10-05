/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "MockLinkFTP.h"
#include "MockLink.h"

const MockLinkFTP::ErrorMode_t MockLinkFTP::rgFailureModes[] = {
    MockLinkFTP::errModeNoResponse,
    MockLinkFTP::errModeNakResponse,
    MockLinkFTP::errModeNoSecondResponse,
    MockLinkFTP::errModeNakSecondResponse,
    MockLinkFTP::errModeBadSequence,
};
const size_t MockLinkFTP::cFailureModes = sizeof(MockLinkFTP::rgFailureModes) / sizeof(MockLinkFTP::rgFailureModes[0]);

const char* MockLinkFTP::sizeFilenamePrefix = "mocklink-size-";

MockLinkFTP::MockLinkFTP(uint8_t systemIdServer, uint8_t componentIdServer, MockLink* mockLink)
    : _systemIdServer   (systemIdServer)
    , _componentIdServer(componentIdServer)
    , _mockLink         (mockLink)
{
    srand(0); // make sure unit tests are deterministic
}

void MockLinkFTP::ensureNullTemination(MavlinkFTP::Request* request)
{
    if (request->hdr.size < sizeof(request->data)) {
        request->data[request->hdr.size] = '\0';

    } else {
        request->data[sizeof(request->data)-1] = '\0';
    }
}

/// @brief Handles List command requests. Only supports root folder paths.
///         File list returned is set using the setFileList method.
void MockLinkFTP::_listCommand(uint8_t senderSystemId, uint8_t senderComponentId, MavlinkFTP::Request* request, uint16_t seqNumber)
{
    // FIXME: Does not support directories that span multiple packets
    
    MavlinkFTP::Request  ackResponse{};
    QString                     path;
    uint16_t                    outgoingSeqNumber = _nextSeqNumber(seqNumber);

    ensureNullTemination(request);

    // We only support root path
    path = (char *)&request->data[0];
    if (!path.isEmpty() && path != "/") {
        _sendNak(senderSystemId, senderComponentId, MavlinkFTP::kErrFail, outgoingSeqNumber, MavlinkFTP::kCmdListDirectory);
        return;
    }
    
    // Offset requested is past the end of the list
    if (request->hdr.offset > (uint32_t)_fileList.size()) {
        _sendNak(senderSystemId, senderComponentId, MavlinkFTP::kErrEOF, outgoingSeqNumber, MavlinkFTP::kCmdListDirectory);
        return;
    }
    
    ackResponse.hdr.opcode = MavlinkFTP::kRspAck;
    ackResponse.hdr.req_opcode = MavlinkFTP::kCmdListDirectory;
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
        _sendNak(senderSystemId, senderComponentId, MavlinkFTP::kErrFail, outgoingSeqNumber, MavlinkFTP::kCmdListDirectory);
        return;
    } else if (_errMode == errModeNoSecondResponse) {
        // No response for all subsequent requests
        return;
    } else {
        // FIXME: Does not support directories that span multiple packets
        _sendNak(senderSystemId, senderComponentId, MavlinkFTP::kErrEOF, outgoingSeqNumber, MavlinkFTP::kCmdListDirectory);
    }
}

void MockLinkFTP::_openCommand(uint8_t senderSystemId, uint8_t senderComponentId, MavlinkFTP::Request* request, uint16_t seqNumber)
{
    MavlinkFTP::Request response{};
    QString             path;
    uint16_t            outgoingSeqNumber = _nextSeqNumber(seqNumber);
    QString             tmpFilename;
    
    ensureNullTemination(request);

    size_t cchPath = strnlen((char *)request->data, sizeof(request->data));
    Q_ASSERT(cchPath != sizeof(request->data));
    Q_UNUSED(cchPath); // Fix initialized-but-not-referenced warning on release builds
    path = (char *)request->data;

    _currentFile.close();

    QString sizePrefix = sizeFilenamePrefix;
    if (path.startsWith(sizePrefix)) {
        QString sizeString = path.right(path.length() - sizePrefix.length());
        tmpFilename = _createTestTempFile(sizeString.toInt());
    } else if (path == "/general.json") {
        tmpFilename = ":MockLink/General.MetaData.json";
    } else if (path == "/general.json.xz") {
        tmpFilename = ":MockLink/General.MetaData.json.xz";
    } else if (path == "/parameter.json") {
        tmpFilename = ":MockLink/Parameter.MetaData.json";
    } else if (path == "/parameter.json.xz") {
        tmpFilename = ":MockLink/Parameter.MetaData.json.xz";
    }

    if (!tmpFilename.isEmpty()) {
        _currentFile.setFileName(tmpFilename);
        if (!_currentFile.open(QIODevice::ReadOnly)) {
            _sendNakErrno(senderSystemId, senderComponentId, _currentFile.error(), outgoingSeqNumber, MavlinkFTP::kCmdOpenFileRO);
            return;
        }
    } else {
        _sendNak(senderSystemId, senderComponentId, MavlinkFTP::kErrFailFileNotFound, outgoingSeqNumber, MavlinkFTP::kCmdOpenFileRO);
        return;
    }
    
    response.hdr.opcode     = MavlinkFTP::kRspAck;
    response.hdr.req_opcode = MavlinkFTP::kCmdOpenFileRO;
    response.hdr.session    = _sessionId;
    
    // Data contains file length
    response.hdr.size = sizeof(uint32_t);
    response.openFileLength = _currentFile.size();
    
    _sendResponse(senderSystemId, senderComponentId, &response, outgoingSeqNumber);
}

void MockLinkFTP::_readCommand(uint8_t senderSystemId, uint8_t senderComponentId, MavlinkFTP::Request* request, uint16_t seqNumber)
{
    MavlinkFTP::Request	response{};
    uint16_t			outgoingSeqNumber = _nextSeqNumber(seqNumber);

    if (request->hdr.session != _sessionId) {
        _sendNak(senderSystemId, senderComponentId, MavlinkFTP::kErrInvalidSession, outgoingSeqNumber, MavlinkFTP::kCmdReadFile);
        return;
    }
    
    uint32_t readOffset = request->hdr.offset;  // offset into file for reading

    if (readOffset != 0) {
        // If we get here it means the client is requesting additional data past the first request
        if (_errMode == errModeNakSecondResponse) {
            // Nak error all subsequent requests
            _sendNak(senderSystemId, senderComponentId, MavlinkFTP::kErrFail, outgoingSeqNumber, MavlinkFTP::kCmdReadFile);
            return;
        } else if (_errMode == errModeNoSecondResponse) {
            // No rsponse for all subsequent requests
            return;
        }
    }
    
    if (readOffset >= _currentFile.size()) {
        _sendNak(senderSystemId, senderComponentId, MavlinkFTP::kErrEOF, outgoingSeqNumber, MavlinkFTP::kCmdReadFile);
        return;
    }
    
    uint8_t cBytesToRead = (uint8_t)qMin((qint64)sizeof(response.data), _currentFile.size() - readOffset);
    _currentFile.seek(readOffset);
    QByteArray bytes = _currentFile.read(cBytesToRead);
    memcpy(response.data, bytes.constData(), cBytesToRead);
    
    // We should always have written something, otherwise there is something wrong with the code above
    Q_ASSERT(cBytesToRead);
    
    response.hdr.session    = _sessionId;
    response.hdr.size       = cBytesToRead;
    response.hdr.offset     = request->hdr.offset;
    response.hdr.opcode     = MavlinkFTP::kRspAck;
    response.hdr.req_opcode = MavlinkFTP::kCmdReadFile;

    _sendResponse(senderSystemId, senderComponentId, &response, outgoingSeqNumber);
}

void MockLinkFTP::_burstReadCommand(uint8_t senderSystemId, uint8_t senderComponentId, MavlinkFTP::Request* request, uint16_t seqNumber)
{
    uint16_t            outgoingSeqNumber = _nextSeqNumber(seqNumber);
    MavlinkFTP::Request response{};

    if (request->hdr.session != _sessionId) {
        _sendNak(senderSystemId, senderComponentId, MavlinkFTP::kErrFail, outgoingSeqNumber, MavlinkFTP::kCmdBurstReadFile);
        return;
    }
    
    int         burstMax    = 10;
    int         burstCount  = 1;
    uint32_t    burstOffset = request->hdr.offset;

    while (burstOffset < _currentFile.size() && burstCount++ < burstMax) {
        _currentFile.seek(burstOffset);

        uint8_t     cBytes  = (uint8_t)qMin((qint64)sizeof(response.data), _currentFile.size() - burstOffset);
        QByteArray  bytes   = _currentFile.read(cBytes);

        // We should always have written something, otherwise there is something wrong with the code above
        Q_ASSERT(cBytes);

        memcpy(response.data, bytes.constData(), cBytes);

        response.hdr.session        = _sessionId;
        response.hdr.size           = cBytes;
        response.hdr.offset         = burstOffset;
        response.hdr.opcode         = MavlinkFTP::kRspAck;
        response.hdr.req_opcode     = MavlinkFTP::kCmdBurstReadFile;
        response.hdr.burstComplete  = burstCount == burstMax ? 1 : 0;

        _sendResponse(senderSystemId, senderComponentId, &response, outgoingSeqNumber);
        
        outgoingSeqNumber = _nextSeqNumber(outgoingSeqNumber);
        burstOffset += cBytes;
    }

    if (burstOffset >= _currentFile.size()) {
        // Burst is fully complete
        _sendNak(senderSystemId, senderComponentId, MavlinkFTP::kErrEOF, outgoingSeqNumber, MavlinkFTP::kCmdBurstReadFile);
    }
}

void MockLinkFTP::_terminateCommand(uint8_t senderSystemId, uint8_t senderComponentId, MavlinkFTP::Request* request, uint16_t seqNumber)
{
    uint16_t outgoingSeqNumber = _nextSeqNumber(seqNumber);

    if (request->hdr.session != _sessionId) {
        _sendNak(senderSystemId, senderComponentId, MavlinkFTP::kErrInvalidSession, outgoingSeqNumber, MavlinkFTP::kCmdTerminateSession);
        return;
    }
    
    _sendAck(senderSystemId, senderComponentId, outgoingSeqNumber, MavlinkFTP::kCmdTerminateSession);

    emit terminateCommandReceived();
}

void MockLinkFTP::_resetCommand(uint8_t senderSystemId, uint8_t senderComponentId, uint16_t seqNumber)
{
    uint16_t outgoingSeqNumber = _nextSeqNumber(seqNumber);
    
    _currentFile.close();
    _currentFile.remove();
    _sendAck(senderSystemId, senderComponentId, outgoingSeqNumber, MavlinkFTP::kCmdResetSessions);
    
    emit resetCommandReceived();
}

void MockLinkFTP::mavlinkMessageReceived(const mavlink_message_t& message)
{
    if (message.msgid != MAVLINK_MSG_ID_FILE_TRANSFER_PROTOCOL) {
        return;
    }
    
    MavlinkFTP::Request  ackResponse{};

    mavlink_file_transfer_protocol_t requestFTP;
    mavlink_msg_file_transfer_protocol_decode(&message, &requestFTP);
    
    if (requestFTP.target_system != _systemIdServer) {
        return;
    }

    MavlinkFTP::Request* request = (MavlinkFTP::Request*)&requestFTP.payload[0];

    // kCmdOpenFileRO and kCmdResetSessions don't support retry so we can't drop those
    if (_randomDropsEnabled && request->hdr.opcode != MavlinkFTP::kCmdOpenFileRO && request->hdr.opcode != MavlinkFTP::kCmdResetSessions) {
        if ((rand() % 5) == 0) {
            qDebug() << "MockLinkFTP: Random drop of incoming packet";
            return;
        }
    }

    if (_lastReplyValid && request->hdr.seqNumber == _lastReplySequence - 1) {
        // This is the same request as the one we replied to last. It means the (n)ack got lost, and the GCS
        // resent the request
        qDebug() << "MockLinkFTP: resending response";
        _mockLink->respondWithMavlinkMessage(_lastReply);
        return;
    }

    uint16_t incomingSeqNumber = request->hdr.seqNumber;
    uint16_t outgoingSeqNumber = _nextSeqNumber(incomingSeqNumber);
    
    if (request->hdr.opcode != MavlinkFTP::kCmdResetSessions && request->hdr.opcode != MavlinkFTP::kCmdTerminateSession) {
        if (_errMode == errModeNoResponse) {
            // Don't respond to any requests, this shold cause the client to eventually timeout waiting for the ack
            return;
        } else if (_errMode == errModeNakResponse) {
            // Nak all requests, the actual error send back doesn't really matter as long as it's an error
            _sendNak(message.sysid, message.compid, MavlinkFTP::kErrFail, outgoingSeqNumber, (MavlinkFTP::OpCode_t)request->hdr.opcode);
            return;
        }
    }

    switch (request->hdr.opcode) {
    case MavlinkFTP::kCmdNone:
        // ignored, always acked
        ackResponse.hdr.opcode = MavlinkFTP::kRspAck;
        ackResponse.hdr.session = 0;
        ackResponse.hdr.size = 0;
        _sendResponse(message.sysid, message.compid, &ackResponse, outgoingSeqNumber);
        break;

    case MavlinkFTP::kCmdListDirectory:
        _listCommand(message.sysid, message.compid, request, incomingSeqNumber);
        break;

    case MavlinkFTP::kCmdOpenFileRO:
        _openCommand(message.sysid, message.compid, request, incomingSeqNumber);
        break;

    case MavlinkFTP::kCmdReadFile:
        _readCommand(message.sysid, message.compid, request, incomingSeqNumber);
        break;

    case MavlinkFTP::kCmdBurstReadFile:
        _burstReadCommand(message.sysid, message.compid, request, incomingSeqNumber);
        break;

    case MavlinkFTP::kCmdTerminateSession:
        _terminateCommand(message.sysid, message.compid, request, incomingSeqNumber);
        break;

    case MavlinkFTP::kCmdResetSessions:
        _resetCommand(message.sysid, message.compid, incomingSeqNumber);
        break;

    default:
        // nack for all NYI opcodes
        _sendNak(message.sysid, message.compid, MavlinkFTP::kErrUnknownCommand, outgoingSeqNumber, (MavlinkFTP::OpCode_t)request->hdr.opcode);
        break;
    }
}

/// @brief Sends an Ack
void MockLinkFTP::_sendAck(uint8_t targetSystemId, uint8_t targetComponentId, uint16_t seqNumber, MavlinkFTP::OpCode_t reqOpcode)
{
    MavlinkFTP::Request ackResponse{};
    
    ackResponse.hdr.opcode      = MavlinkFTP::kRspAck;
    ackResponse.hdr.req_opcode  = reqOpcode;
    ackResponse.hdr.session     = _sessionId;
    ackResponse.hdr.size        = 0;
    
    _sendResponse(targetSystemId, targetComponentId, &ackResponse, seqNumber);
}

void MockLinkFTP::_sendNak(uint8_t targetSystemId, uint8_t targetComponentId, MavlinkFTP::ErrorCode_t error, uint16_t seqNumber, MavlinkFTP::OpCode_t reqOpcode)
{
    MavlinkFTP::Request nakResponse{};

    nakResponse.hdr.opcode      = MavlinkFTP::kRspNak;
    nakResponse.hdr.req_opcode  = reqOpcode;
    nakResponse.hdr.session     = _sessionId;
    nakResponse.hdr.size        = 1;
    nakResponse.data[0]         = error;
    
    _sendResponse(targetSystemId, targetComponentId, &nakResponse, seqNumber);
}

void MockLinkFTP::_sendNakErrno(uint8_t targetSystemId, uint8_t targetComponentId, uint8_t nakErrno, uint16_t seqNumber, MavlinkFTP::OpCode_t reqOpcode)
{
    MavlinkFTP::Request nakResponse{};

    nakResponse.hdr.opcode      = MavlinkFTP::kRspNak;
    nakResponse.hdr.req_opcode  = reqOpcode;
    nakResponse.hdr.session     = _sessionId;
    nakResponse.hdr.size        = 2;
    nakResponse.data[0]         = MavlinkFTP::kErrFailErrno;
    nakResponse.data[1]         = nakErrno;

    _sendResponse(targetSystemId, targetComponentId, &nakResponse, seqNumber);
}

/// @brief Emits a Request through the messageReceived signal.
void MockLinkFTP::_sendResponse(uint8_t targetSystemId, uint8_t targetComponentId, MavlinkFTP::Request* request, uint16_t seqNumber)
{
    request->hdr.seqNumber  = seqNumber;
    _lastReplySequence      = seqNumber;
    _lastReplyValid         = true;
    
    mavlink_msg_file_transfer_protocol_pack_chan(_systemIdServer,               // System ID
                                                 _componentIdServer,            // Component ID
                                                 _mockLink->mavlinkChannel(),
                                                 &_lastReply,                   // Mavlink Message to pack into
                                                 0,                             // Target network
                                                 targetSystemId,
                                                 targetComponentId,
                                                 (uint8_t*)request);            // Payload

    // kCmdOpenFileRO and kCmdResetSessions don't support retry so we can't drop those
    if (_randomDropsEnabled && request->hdr.req_opcode != MavlinkFTP::kCmdOpenFileRO && request->hdr.req_opcode != MavlinkFTP::kCmdResetSessions) {
        if ((rand() % 5) == 0) {
            qDebug() << "MockLinkFTP: Random drop of outgoing packet";
            return;
        }
    }
    
    _mockLink->respondWithMavlinkMessage(_lastReply);
}

/// @brief Generates the next sequence number given an incoming sequence number. Handles generating
/// bad sequence numbers when errModeBadSequence is set.
uint16_t MockLinkFTP::_nextSeqNumber(uint16_t seqNumber)
{
    uint16_t outgoingSeqNumber = seqNumber + 1;
    
    if (_errMode == errModeBadSequence) {
        outgoingSeqNumber++;
    }
    return outgoingSeqNumber;
}

QString MockLinkFTP::_createTestTempFile(int size)
{
    QGCTemporaryFile tmpFile("MockLinkFTPTestCase");
    tmpFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
    for (int i=0; i<size; i++) {
        tmpFile.write(QByteArray(1, i % 255));
    }
    tmpFile.close();
    return tmpFile.fileName();
}
