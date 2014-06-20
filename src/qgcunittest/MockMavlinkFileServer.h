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

#ifndef MOCKMAVLINKFILESERVER_H
#define MOCKMAVLINKFILESERVER_H

#include "MockMavlinkInterface.h"

class MockMavlinkFileServer : public MockMavlinkInterface
{
    Q_OBJECT
    
public:
    MockMavlinkFileServer(void) { };
    virtual void sendMessage(mavlink_message_t message);
    
private:
    // FIXME: These should be in a mavlink header somewhere shouldn't they?
    
    struct RequestHeader
    {
        uint8_t		magic;
        uint8_t		session;
        uint8_t		opcode;
        uint8_t		size;
        uint32_t	crc32;
        uint32_t	offset;
        uint8_t		data[];
    };
    
    enum Opcode
    {
        kCmdNone,	// ignored, always acked
        kCmdTerminate,	// releases sessionID, closes file
        kCmdReset,	// terminates all sessions
        kCmdList,	// list files in <path> from <offset>
        kCmdOpen,	// opens <path> for reading, returns <session>
        kCmdRead,	// reads <size> bytes from <offset> in <session>
        kCmdCreate,	// creates <path> for writing, returns <session>
        kCmdWrite,	// appends <size> bytes at <offset> in <session>
        kCmdRemove,	// remove file (only if created by server?)
        
        kRspAck,
        kRspNak
    };
    
    enum ErrorCode
    {
        kErrNone,
        kErrNoRequest,
        kErrNoSession,
        kErrSequence,
        kErrNotDir,
        kErrNotFile,
        kErrEOF,
        kErrNotAppend,
        kErrTooBig,
        kErrIO,
        kErrPerm
    };
    

};

#endif
