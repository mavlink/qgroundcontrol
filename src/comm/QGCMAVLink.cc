/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCMAVLink.h"

constexpr QGCMAVLink::FirmwareClass_t QGCMAVLink::FirmwareClassPX4;
constexpr QGCMAVLink::FirmwareClass_t QGCMAVLink::FirmwareClassArduPilot;
constexpr QGCMAVLink::FirmwareClass_t QGCMAVLink::FirmwareClassGeneric;

constexpr QGCMAVLink::VehicleClass_t QGCMAVLink::VehicleClassFixedWing;
constexpr QGCMAVLink::VehicleClass_t QGCMAVLink::VehicleClassRoverBoat;
constexpr QGCMAVLink::VehicleClass_t QGCMAVLink::VehicleClassSub;
constexpr QGCMAVLink::VehicleClass_t QGCMAVLink::VehicleClassMultiRotor;
constexpr QGCMAVLink::VehicleClass_t QGCMAVLink::VehicleClassVTOL;
constexpr QGCMAVLink::VehicleClass_t QGCMAVLink::VehicleClassGeneric;

QList<QGCMAVLink::FirmwareClass_t> QGCMAVLink::allFirmwareClasses(void)
{
    static const QList<QGCMAVLink::FirmwareClass_t> classes = {
        FirmwareClassPX4,
        FirmwareClassArduPilot,
        FirmwareClassGeneric
    };

    return classes;
}

QList<QGCMAVLink::VehicleClass_t> QGCMAVLink::allVehicleClasses(void)
{
    static const QList<QGCMAVLink::VehicleClass_t> classes = {
        VehicleClassFixedWing,
        VehicleClassRoverBoat,
        VehicleClassSub,
        VehicleClassMultiRotor,
        VehicleClassVTOL,
        VehicleClassGeneric,
    };

    return classes;
}

QGCMAVLink::FirmwareClass_t QGCMAVLink::firmwareClass(MAV_AUTOPILOT autopilot)
{
    if (isPX4FirmwareClass(autopilot)) {
        return FirmwareClassPX4;
    } else if (isArduPilotFirmwareClass(autopilot)) {
        return FirmwareClassArduPilot;
    } else {
        return FirmwareClassGeneric;
    }
}

bool QGCMAVLink::isFixedWing(MAV_TYPE mavType)
{
    return vehicleClass(mavType) == VehicleClassFixedWing;
}

bool QGCMAVLink::isRoverBoat(MAV_TYPE mavType)
{
    return vehicleClass(mavType) == VehicleClassRoverBoat;
}

bool QGCMAVLink::isSub(MAV_TYPE mavType)
{
    return vehicleClass(mavType) == VehicleClassSub;
}

bool QGCMAVLink::isMultiRotor(MAV_TYPE mavType)
{
    return vehicleClass(mavType) == VehicleClassMultiRotor;
}

bool QGCMAVLink::isVTOL(MAV_TYPE mavType)
{
    return vehicleClass(mavType) == VehicleClassVTOL;
}

QGCMAVLink::VehicleClass_t QGCMAVLink::vehicleClass(MAV_TYPE mavType)
{
    switch (mavType) {
    case MAV_TYPE_GROUND_ROVER:
    case MAV_TYPE_SURFACE_BOAT:
        return VehicleClassRoverBoat;
    case MAV_TYPE_QUADROTOR:
    case MAV_TYPE_COAXIAL:
    case MAV_TYPE_HELICOPTER:
    case MAV_TYPE_HEXAROTOR:
    case MAV_TYPE_OCTOROTOR:
    case MAV_TYPE_TRICOPTER:
        return VehicleClassMultiRotor;
    case MAV_TYPE_VTOL_DUOROTOR:
    case MAV_TYPE_VTOL_QUADROTOR:
    case MAV_TYPE_VTOL_TILTROTOR:
    case MAV_TYPE_VTOL_RESERVED2:
    case MAV_TYPE_VTOL_RESERVED3:
    case MAV_TYPE_VTOL_RESERVED4:
    case MAV_TYPE_VTOL_RESERVED5:
        return VehicleClassVTOL;
    case MAV_TYPE_FIXED_WING:
        return VehicleClassFixedWing;
    default:
        return VehicleClassGeneric;
    }
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
