/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MAVLinkFTP.h"

QString MavlinkFTP::opCodeToString(OpCode_t opCode)
{
    switch (opCode) {
    case kCmdNone:
        return "None";
    case kCmdTerminateSession:
        return "Terminate Session";
    case kCmdResetSessions:
        return "Reset Sessions";
    case kCmdListDirectory:
        return "List Directory";
    case kCmdOpenFileRO:
        return "Open File RO";
    case kCmdReadFile:
        return "Read File";
    case kCmdCreateFile:
        return "Create File";
    case kCmdWriteFile:
        return "Write File";
    case kCmdRemoveFile:
        return "Remove File";
    case kCmdCreateDirectory:
        return "Create Directory";
    case kCmdRemoveDirectory:
        return "Remove Directory";
    case kCmdOpenFileWO:
        return "Open File WO";
    case kCmdTruncateFile:
        return "Truncate File";
    case kCmdRename:
        return "Rename";
    case kCmdCalcFileCRC32:
        return "Calc File CRC32";
    case kCmdBurstReadFile:
        return "Burst Read File";
    case kRspAck:
        return "Ack";
    case kRspNak:
        return "Nak";
    }

    return "Unknown OpCode";
}

QString MavlinkFTP::errorCodeToString(ErrorCode_t errorCode)
{
    switch (errorCode) {
    case kErrNone:
        return "None";
    case kErrFail:
        return "Fail";
    case kErrFailErrno:
        return "Fail Errorno";
    case kErrInvalidDataSize:
        return "Invalid Data Size";
    case kErrInvalidSession:
        return "Invalid Session";
    case kErrNoSessionsAvailable:
        return "No Sessions Available";
    case kErrEOF:
        return "EOF";
    case kErrUnknownCommand:
        return "Unknown Command";
    case kErrFailFileExists:
        return "File Already Exists";
    case kErrFailFileProtected:
        return "File Protected";
    case kErrFailFileNotFound:
        return "File Not Found";
    }

    return "Unknown Error";
}

