/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCMAVLink.h"

#include <QtGlobal>
#include <QDebug>

constexpr QGCMAVLink::FirmwareClass_t QGCMAVLink::FirmwareClassPX4;
constexpr QGCMAVLink::FirmwareClass_t QGCMAVLink::FirmwareClassArduPilot;
constexpr QGCMAVLink::FirmwareClass_t QGCMAVLink::FirmwareClassGeneric;

constexpr QGCMAVLink::VehicleClass_t QGCMAVLink::VehicleClassAirship;
constexpr QGCMAVLink::VehicleClass_t QGCMAVLink::VehicleClassFixedWing;
constexpr QGCMAVLink::VehicleClass_t QGCMAVLink::VehicleClassRoverBoat;
constexpr QGCMAVLink::VehicleClass_t QGCMAVLink::VehicleClassSub;
constexpr QGCMAVLink::VehicleClass_t QGCMAVLink::VehicleClassMultiRotor;
constexpr QGCMAVLink::VehicleClass_t QGCMAVLink::VehicleClassVTOL;
constexpr QGCMAVLink::VehicleClass_t QGCMAVLink::VehicleClassGeneric;

QGCMAVLink::QGCMAVLink(QObject* parent)
    : QObject(parent)
{

}

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

QString QGCMAVLink::firmwareClassToString(FirmwareClass_t firmwareClass)
{
    switch (firmwareClass) {
    case FirmwareClassPX4:
        return QT_TRANSLATE_NOOP("Firmware Class", "PX4 Pro");
    case FirmwareClassArduPilot:
        return QT_TRANSLATE_NOOP("Firmware Class", "ArduPilot");
    case FirmwareClassGeneric:
        return QT_TRANSLATE_NOOP("Firmware Class", "Generic");
    default:
        return QT_TRANSLATE_NOOP("Firmware Class", "Unknown");
    }
}

bool QGCMAVLink::isAirship(MAV_TYPE mavType)
{
    return vehicleClass(mavType) == VehicleClassAirship;
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
    case MAV_TYPE_SUBMARINE:
        return VehicleClassSub;
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
    case MAV_TYPE_AIRSHIP:
        return VehicleClassAirship;
    default:
        return VehicleClassGeneric;
    }
}

QString QGCMAVLink::vehicleClassToString(VehicleClass_t vehicleClass)
{
    switch (vehicleClass) {
    case VehicleClassAirship:
        return QT_TRANSLATE_NOOP("Vehicle Class", "Airship");
    case VehicleClassFixedWing:
        return QT_TRANSLATE_NOOP("Vehicle Class", "Fixed Wing");
    case VehicleClassRoverBoat:
        return QT_TRANSLATE_NOOP("Vehicle Class", "Rover-Boat");
    case VehicleClassSub:
        return QT_TRANSLATE_NOOP("Vehicle Class", "Sub");
    case VehicleClassMultiRotor:
        return QT_TRANSLATE_NOOP("Vehicle Class", "Multi-Rotor");
    case VehicleClassVTOL:
        return QT_TRANSLATE_NOOP("Vehicle Class", "VTOL");
    case VehicleClassGeneric:
        return QT_TRANSLATE_NOOP("Vehicle Class", "Generic");
    default:
        return QT_TRANSLATE_NOOP("Vehicle Class", "Unknown");
    }
}

QString QGCMAVLink::mavResultToString(MAV_RESULT result)
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

QString QGCMAVLink::mavSysStatusSensorToString(MAV_SYS_STATUS_SENSOR sysStatusSensor)
{
    struct sensorInfo_s {
        uint32_t    bit;
        QString     sensorName;
    };

    static const sensorInfo_s rgSensorInfo[] = {
        { MAV_SYS_STATUS_SENSOR_3D_GYRO,                QT_TRANSLATE_NOOP("MAVLink SYS_STATUS_SENSOR value", "Gyro") },
        { MAV_SYS_STATUS_SENSOR_3D_ACCEL,               QT_TRANSLATE_NOOP("MAVLink SYS_STATUS_SENSOR value", "Accelerometer") },
        { MAV_SYS_STATUS_SENSOR_3D_MAG,                 QT_TRANSLATE_NOOP("MAVLink SYS_STATUS_SENSOR value", "Magnetometer") },
        { MAV_SYS_STATUS_SENSOR_ABSOLUTE_PRESSURE,      QT_TRANSLATE_NOOP("MAVLink SYS_STATUS_SENSOR value", "Absolute pressure") },
        { MAV_SYS_STATUS_SENSOR_DIFFERENTIAL_PRESSURE,  QT_TRANSLATE_NOOP("MAVLink SYS_STATUS_SENSOR value", "Differential pressure") },
        { MAV_SYS_STATUS_SENSOR_GPS,                    QT_TRANSLATE_NOOP("MAVLink SYS_STATUS_SENSOR value", "GPS") },
        { MAV_SYS_STATUS_SENSOR_OPTICAL_FLOW,           QT_TRANSLATE_NOOP("MAVLink SYS_STATUS_SENSOR value", "Optical flow") },
        { MAV_SYS_STATUS_SENSOR_VISION_POSITION,        QT_TRANSLATE_NOOP("MAVLink SYS_STATUS_SENSOR value", "Computer vision position") },
        { MAV_SYS_STATUS_SENSOR_LASER_POSITION,         QT_TRANSLATE_NOOP("MAVLink SYS_STATUS_SENSOR value", "Laser based position") },
        { MAV_SYS_STATUS_SENSOR_EXTERNAL_GROUND_TRUTH,  QT_TRANSLATE_NOOP("MAVLink SYS_STATUS_SENSOR value", "External ground truth") },
        { MAV_SYS_STATUS_SENSOR_ANGULAR_RATE_CONTROL,   QT_TRANSLATE_NOOP("MAVLink SYS_STATUS_SENSOR value", "Angular rate control") },
        { MAV_SYS_STATUS_SENSOR_ATTITUDE_STABILIZATION, QT_TRANSLATE_NOOP("MAVLink SYS_STATUS_SENSOR value", "Attitude stabilization") },
        { MAV_SYS_STATUS_SENSOR_YAW_POSITION,           QT_TRANSLATE_NOOP("MAVLink SYS_STATUS_SENSOR value", "Yaw position") },
        { MAV_SYS_STATUS_SENSOR_Z_ALTITUDE_CONTROL,     QT_TRANSLATE_NOOP("MAVLink SYS_STATUS_SENSOR value", "Z/altitude control") },
        { MAV_SYS_STATUS_SENSOR_XY_POSITION_CONTROL,    QT_TRANSLATE_NOOP("MAVLink SYS_STATUS_SENSOR value", "X/Y position control") },
        { MAV_SYS_STATUS_SENSOR_MOTOR_OUTPUTS,          QT_TRANSLATE_NOOP("MAVLink SYS_STATUS_SENSOR value", "Motor outputs / control") },
        { MAV_SYS_STATUS_SENSOR_RC_RECEIVER,            QT_TRANSLATE_NOOP("MAVLink SYS_STATUS_SENSOR value", "RC receiver") },
        { MAV_SYS_STATUS_SENSOR_3D_GYRO2,               QT_TRANSLATE_NOOP("MAVLink SYS_STATUS_SENSOR value", "Gyro 2") },
        { MAV_SYS_STATUS_SENSOR_3D_ACCEL2,              QT_TRANSLATE_NOOP("MAVLink SYS_STATUS_SENSOR value", "Accelerometer 2") },
        { MAV_SYS_STATUS_SENSOR_3D_MAG2,                QT_TRANSLATE_NOOP("MAVLink SYS_STATUS_SENSOR value", "Magnetometer 2") },
        { MAV_SYS_STATUS_GEOFENCE,                      QT_TRANSLATE_NOOP("MAVLink SYS_STATUS_SENSOR value", "GeoFence") },
        { MAV_SYS_STATUS_AHRS,                          QT_TRANSLATE_NOOP("MAVLink SYS_STATUS_SENSOR value", "AHRS") },
        { MAV_SYS_STATUS_TERRAIN,                       QT_TRANSLATE_NOOP("MAVLink SYS_STATUS_SENSOR value", "Terrain") },
        { MAV_SYS_STATUS_REVERSE_MOTOR,                 QT_TRANSLATE_NOOP("MAVLink SYS_STATUS_SENSOR value", "Motors reversed") },
        { MAV_SYS_STATUS_LOGGING,                       QT_TRANSLATE_NOOP("MAVLink SYS_STATUS_SENSOR value", "Logging") },
        { MAV_SYS_STATUS_SENSOR_BATTERY,                QT_TRANSLATE_NOOP("MAVLink SYS_STATUS_SENSOR value", "Battery") },
        { MAV_SYS_STATUS_SENSOR_PROXIMITY,              QT_TRANSLATE_NOOP("MAVLink SYS_STATUS_SENSOR value", "Proximity") },
        { MAV_SYS_STATUS_SENSOR_SATCOM,                 QT_TRANSLATE_NOOP("MAVLink SYS_STATUS_SENSOR value", "Satellite Communication") },
        { MAV_SYS_STATUS_PREARM_CHECK,                  QT_TRANSLATE_NOOP("MAVLink SYS_STATUS_SENSOR value", "Pre-Arm Check") },
        { MAV_SYS_STATUS_OBSTACLE_AVOIDANCE,            QT_TRANSLATE_NOOP("MAVLink SYS_STATUS_SENSOR value", "Avoidance/collision prevention") },
    };

    for (size_t i=0; i<sizeof(rgSensorInfo)/sizeof(sensorInfo_s); i++) {
        const sensorInfo_s* pSensorInfo = &rgSensorInfo[i];
        if (sysStatusSensor == pSensorInfo->bit) {
            return pSensorInfo->sensorName;
        }
    }

    qWarning() << "QGCMAVLink::mavSysStatusSensorToString: Unknown sensor" << sysStatusSensor;

    return QT_TRANSLATE_NOOP("MAVLink unknown SYS_STATUS_SENSOR value", "Unknown sensor");
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

