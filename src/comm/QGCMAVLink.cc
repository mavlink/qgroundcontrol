/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCMAVLink.h"

bool QGCMAVLink::isFixedWing(MAV_TYPE mavType)
{
    return mavType == MAV_TYPE_FIXED_WING;
}

bool QGCMAVLink::isRover(MAV_TYPE mavType)
{
    switch (mavType) {
    case MAV_TYPE_GROUND_ROVER:
    case MAV_TYPE_SURFACE_BOAT:
        return true;
    default:
        return false;
    }
}

bool QGCMAVLink::isSub(MAV_TYPE mavType)
{
    return mavType == MAV_TYPE_SUBMARINE;
}

bool QGCMAVLink::isMultiRotor(MAV_TYPE mavType)
{
    switch (mavType) {
    case MAV_TYPE_QUADROTOR:
    case MAV_TYPE_COAXIAL:
    case MAV_TYPE_HELICOPTER:
    case MAV_TYPE_HEXAROTOR:
    case MAV_TYPE_OCTOROTOR:
    case MAV_TYPE_TRICOPTER:
        return true;
    default:
        return false;
    }
}

bool QGCMAVLink::isVTOL(MAV_TYPE mavType)
{
    switch (mavType) {
    case MAV_TYPE_VTOL_DUOROTOR:
    case MAV_TYPE_VTOL_QUADROTOR:
    case MAV_TYPE_VTOL_TILTROTOR:
    case MAV_TYPE_VTOL_RESERVED2:
    case MAV_TYPE_VTOL_RESERVED3:
    case MAV_TYPE_VTOL_RESERVED4:
    case MAV_TYPE_VTOL_RESERVED5:
        return true;
    default:
        return false;
    }
}

MAV_TYPE QGCMAVLink::vehicleClass(MAV_TYPE mavType)
{
    if (isFixedWing(mavType)) {
        return MAV_TYPE_FIXED_WING;
    } else if (isRover(mavType)) {
        return MAV_TYPE_GROUND_ROVER;
    } else if (isSub(mavType)) {
        return MAV_TYPE_SUBMARINE;
    } else if (isMultiRotor(mavType)) {
        return MAV_TYPE_QUADROTOR;
    } else if (isVTOL(mavType)) {
        return MAV_TYPE_VTOL_QUADROTOR;
    }

    return MAV_TYPE_GENERIC;
}

QString  QGCMAVLink::mavResultToString(MAV_RESULT result)
{
    switch (result) {
    case MAV_RESULT_ACCEPTED:
        return QStringLiteral("MAV_RESULT_ACCEPTED");
    case MAV_RESULT_TEMPORARILY_REJECTED:
        return QStringLiteral("MAV_RESULT_TEMPORARILY_REJECTED");
    case MAV_RESULT_DENIED:
        return QStringLiteral("MAV_RESULT_DENIED");
    case MAV_RESULT_UNSUPPORTED:
        return QStringLiteral("MAV_RESULT_UNSUPPORTED");
    case MAV_RESULT_FAILED:
        return QStringLiteral("MAV_RESULT_FAILED");
    case MAV_RESULT_IN_PROGRESS:
        return QStringLiteral("MAV_RESULT_IN_PROGRESS");
    default:
        return QStringLiteral("MAV_RESULT unknown %1").arg(result);
    }
}

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
