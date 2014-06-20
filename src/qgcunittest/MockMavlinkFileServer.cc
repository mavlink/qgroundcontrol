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
    Q_ASSERT(message.msgid == MAVLINK_MSG_ID_ENCAPSULATED_DATA);

    mavlink_encapsulated_data_t data;
    mavlink_msg_encapsulated_data_decode(&message, &data);
    const RequestHeader *hdr = (const RequestHeader *)&data.data[0];
    
    // FIXME: Check CRC

    switch (hdr->opcode) {
        case kCmdNone:
            // ignored, always acked
            
            RequestHeader ackHdr;
            ackHdr.magic = 'f';
            ackHdr.opcode = kRspAck;
            ackHdr.session = 0;
            ackHdr.crc32 = 0;
            ackHdr.size = 0;
            // FIXME: Add CRC
            //ackHdr.crc32 = crc32((uint8_t*)&hdr, sizeof(hdr) + hdr.size, 0);
            
            mavlink_message_t ackMessage;
            mavlink_msg_encapsulated_data_pack(250, 0, &ackMessage, 0 /*_encdata_seq*/, (uint8_t*)&ackHdr);
            emit messageReceived(NULL, ackMessage);
            break;

        case kCmdTerminate:
            // releases sessionID, closes file
        case kCmdReset:
            // terminates all sessions
        case kCmdList:
            // list files in <path> from <offset>
        case kCmdOpen:
            // opens <path> for reading, returns <session>
        case kCmdRead:
            // reads <size> bytes from <offset> in <session>
        case kCmdCreate:
            // creates <path> for writing, returns <session>
        case kCmdWrite:
            // appends <size> bytes at <offset> in <session>
        case kCmdRemove:
            // remove file (only if created by server?)
        default:
            // nack for all NYI opcodes
            
            RequestHeader nakHdr;
            nakHdr.opcode = kRspNak;
            nakHdr.magic = 'f';
            nakHdr.session = 0;
            nakHdr.crc32 = 0;
            nakHdr.size = 0;
            // FIXME: Add CRC
            //ackHdr.crc32 = crc32((uint8_t*)&hdr, sizeof(hdr) + hdr.size, 0);
            
            mavlink_message_t nakMessage;
            mavlink_msg_encapsulated_data_pack(250, 0, &nakMessage, 0 /*_encdata_seq*/, (uint8_t*)&nakHdr);
            emit messageReceived(NULL, nakMessage);
            break;
    }
}
