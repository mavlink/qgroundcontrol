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

void MockMavlinkFileServer::sendMessage(mavlink_message_t message)
{
    QGCUASFileManager::Request  ackResponse;
    QString                     path;

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
            // We only support root path
            path = (char *)&request->data[0];
            if (!path.isEmpty() && path != "/") {
                _sendNak(QGCUASFileManager::kErrNotDir);
                break;
            }
            
            if (request->hdr.offset > (uint32_t)_fileList.size()) {
                _sendNak(QGCUASFileManager::kErrEOF);
                break;
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

            break;
            
        // Remainder of commands are NYI

        case QGCUASFileManager::kCmdTerminate:
            // releases sessionID, closes file
        case QGCUASFileManager::kCmdOpen:
            // opens <path> for reading, returns <session>
        case QGCUASFileManager::kCmdRead:
            // reads <size> bytes from <offset> in <session>
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

void MockMavlinkFileServer::_emitResponse(QGCUASFileManager::Request* request)
{
    mavlink_message_t   mavlinkMessage;
    
    request->hdr.crc32 = QGCUASFileManager::crc32(request);
    
    mavlink_msg_encapsulated_data_pack(250, 0, &mavlinkMessage, 0 /*_encdata_seq*/, (uint8_t*)request);
    
    emit messageReceived(NULL, mavlinkMessage);
}