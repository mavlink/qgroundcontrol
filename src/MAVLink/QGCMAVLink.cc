#include "QGCMAVLink.h"
#include "MAVLinkLib.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QCoreApplication>

QGC_LOGGING_CATEGORY(QGCMAVLinkLog, "MAVLink.QGCMAVLink")

const QHash<int, QString> QGCMAVLink::mavlinkCompIdHash {
    { MAV_COMP_ID_AUTOPILOT1, "Autopilot" },
    { MAV_COMP_ID_ONBOARD_COMPUTER, "Onboard Computer" },
    { MAV_COMP_ID_CAMERA,   "Camera1" },
    { MAV_COMP_ID_CAMERA2,  "Camera2" },
    { MAV_COMP_ID_CAMERA3,  "Camera3" },
    { MAV_COMP_ID_CAMERA4,  "Camera4" },
    { MAV_COMP_ID_CAMERA5,  "Camera5" },
    { MAV_COMP_ID_CAMERA6,  "Camera6" },
    { MAV_COMP_ID_SERVO1,   "Servo1" },
    { MAV_COMP_ID_SERVO2,   "Servo2" },
    { MAV_COMP_ID_SERVO3,   "Servo3" },
    { MAV_COMP_ID_SERVO4,   "Servo4" },
    { MAV_COMP_ID_SERVO5,   "Servo5" },
    { MAV_COMP_ID_SERVO6,   "Servo6" },
    { MAV_COMP_ID_SERVO7,   "Servo7" },
    { MAV_COMP_ID_SERVO8,   "Servo8" },
    { MAV_COMP_ID_SERVO9,   "Servo9" },
    { MAV_COMP_ID_SERVO10,  "Servo10" },
    { MAV_COMP_ID_SERVO11,  "Servo11" },
    { MAV_COMP_ID_SERVO12,  "Servo12" },
    { MAV_COMP_ID_SERVO13,  "Servo13" },
    { MAV_COMP_ID_SERVO14,  "Servo14" },
    { MAV_COMP_ID_GIMBAL,   "Gimbal1" },
    { MAV_COMP_ID_ADSB,     "ADSB" },
    { MAV_COMP_ID_OSD,      "OSD" },
    { MAV_COMP_ID_FLARM,    "FLARM" },
    { MAV_COMP_ID_GIMBAL2,  "Gimbal2" },
    { MAV_COMP_ID_GIMBAL3,  "Gimbal3" },
    { MAV_COMP_ID_GIMBAL4,  "Gimbal4" },
    { MAV_COMP_ID_GIMBAL5,  "Gimbal5" },
    { MAV_COMP_ID_GIMBAL6,  "Gimbal6" },
    { MAV_COMP_ID_IMU,      "IMU1" },
    { MAV_COMP_ID_IMU_2,    "IMU2" },
    { MAV_COMP_ID_IMU_3,    "IMU3" },
    { MAV_COMP_ID_GPS,      "GPS1" },
    { MAV_COMP_ID_GPS2,     "GPS2" }
};

#ifdef MAVLINK_EXTERNAL_RX_STATUS
    mavlink_status_t m_mavlink_status[MAVLINK_COMM_NUM_BUFFERS];
#endif

#ifdef MAVLINK_GET_CHANNEL_STATUS
mavlink_status_t* mavlink_get_channel_status(uint8_t channel)
{
#ifndef MAVLINK_EXTERNAL_RX_STATUS
    static QList<mavlink_status_t> m_mavlink_status(MAVLINK_COMM_NUM_BUFFERS);
#endif
    if (!QGCMAVLink::isValidChannel(channel)) {
        qCWarning(QGCMAVLinkLog) << Q_FUNC_INFO << "Invalid Channel Number:" << channel;
        return nullptr;
    }

    return &m_mavlink_status[channel];
}
#endif

QGCMAVLink::QGCMAVLink(QObject *parent)
    : QObject(parent)
{
    // qCDebug(StatusTextHandlerLog) << Q_FUNC_INFO << this;

   (void) qRegisterMetaType<mavlink_message_t>("mavlink_message_t");
   // Removes dependence on Q_ENUM_NS static-init order for queued connections.
   (void) qRegisterMetaType<GRIPPER_ACTIONS>("GRIPPER_ACTIONS");
}

QGCMAVLink::~QGCMAVLink()
{
    // qCDebug(StatusTextHandlerLog) << Q_FUNC_INFO << this;
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
        return QCoreApplication::translate("Firmware Class", "PX4 Pro");
    case FirmwareClassArduPilot:
        return QCoreApplication::translate("Firmware Class", "ArduPilot");
    case FirmwareClassGeneric:
        return QCoreApplication::translate("Firmware Class", "Generic");
    default:
        return QCoreApplication::translate("Firmware Class", "Unknown");
    }
}

const char* QGCMAVLink::firmwareClassToCanonicalString(FirmwareClass_t firmwareClass)
{
    switch (firmwareClass) {
    case FirmwareClassPX4:
        return "PX4 Pro";
    case FirmwareClassArduPilot:
        return "ArduPilot";
    case FirmwareClassGeneric:
        return "Generic";
    default:
        return "Unknown";
    }
}

MAV_AUTOPILOT QGCMAVLink::firmwareTypeFromString(const QString &firmwareTypeStr)
{
    const QString type = firmwareTypeStr.trimmed();
    if (type == QLatin1String("ArduPilot")) {
        return MAV_AUTOPILOT_ARDUPILOTMEGA;
    }
    if (type == QLatin1String("PX4 Pro")) {
        return MAV_AUTOPILOT_PX4;
    }
    return MAV_AUTOPILOT_GENERIC;
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

bool QGCMAVLink::isSpacecraft(MAV_TYPE mavType)
{
    return vehicleClass(mavType) == VehicleClassSpacecraft;
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
    case MAV_TYPE_SPACECRAFT_ORBITER:
        return VehicleClassSpacecraft;
    case MAV_TYPE_QUADROTOR:
    case MAV_TYPE_COAXIAL:
    case MAV_TYPE_HELICOPTER:
    case MAV_TYPE_HEXAROTOR:
    case MAV_TYPE_OCTOROTOR:
    case MAV_TYPE_TRICOPTER:
        return VehicleClassMultiRotor;
    case MAV_TYPE_VTOL_TAILSITTER_DUOROTOR:
    case MAV_TYPE_VTOL_TAILSITTER_QUADROTOR:
    case MAV_TYPE_VTOL_TILTROTOR:
    case MAV_TYPE_VTOL_FIXEDROTOR:
    case MAV_TYPE_VTOL_TAILSITTER:
    case MAV_TYPE_VTOL_TILTWING:
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

QString QGCMAVLink::vehicleClassToUserVisibleString(VehicleClass_t vehicleClass)
{
    switch (vehicleClass) {
    case VehicleClassAirship:
        return QCoreApplication::translate("Vehicle Class", "Airship");
    case VehicleClassFixedWing:
        return QCoreApplication::translate("Vehicle Class", "Fixed Wing");
    case VehicleClassRoverBoat:
        return QCoreApplication::translate("Vehicle Class", "Rover-Boat");
    case VehicleClassSub:
        return QCoreApplication::translate("Vehicle Class", "Sub");
    case VehicleClassSpacecraft:
        return QCoreApplication::translate("Vehicle Class", "Spacecraft");
    case VehicleClassMultiRotor:
        return QCoreApplication::translate("Vehicle Class", "Multi-Rotor");
    case VehicleClassVTOL:
        return QCoreApplication::translate("Vehicle Class", "VTOL");
    case VehicleClassGeneric:
        return QCoreApplication::translate("Vehicle Class", "Generic");
    default:
        return QCoreApplication::translate("Vehicle Class", "Unknown");
    }
}

MAV_TYPE QGCMAVLink::vehicleTypeFromString(const QString &vehicleStr)
{
    const QString type = vehicleStr.trimmed();
    if (type == QLatin1String("Multi-Rotor")) {
        return MAV_TYPE_QUADROTOR;
    }
    if (type == QLatin1String("Fixed Wing")) {
        return MAV_TYPE_FIXED_WING;
    }
    if (type == QLatin1String("Rover-Boat") || type == QLatin1String("Rover")) {
        return MAV_TYPE_GROUND_ROVER;
    }
    if (type == QLatin1String("Sub")) {
        return MAV_TYPE_SUBMARINE;
    }
    return MAV_TYPE_GENERIC;
}

const char* QGCMAVLink::vehicleClassToCanonicalString(VehicleClass_t vehicleClass)
{
    switch (vehicleClass) {
    case VehicleClassAirship:   return "Airship";
    case VehicleClassFixedWing: return "Fixed Wing";
    case VehicleClassRoverBoat: return "Rover-Boat";
    case VehicleClassSub:       return "Sub";
    case VehicleClassSpacecraft:return "Spacecraft";
    case VehicleClassMultiRotor:return "Multi-Rotor";
    case VehicleClassVTOL:      return "VTOL";
    case VehicleClassGeneric:   return "Generic";
    default:                    return "Unknown";
    }
}

QString QGCMAVLink::vehicleClassToInternalString(VehicleClass_t vehicleClass)
{
    switch (vehicleClass) {
    case VehicleClassAirship:
        return QStringLiteral("Airship");
    case VehicleClassFixedWing:
        return QStringLiteral("FixedWing");
    case VehicleClassRoverBoat:
        return QStringLiteral("RoverBoat");
    case VehicleClassSub:
        return QStringLiteral("Sub");
    case VehicleClassSpacecraft:
        return QStringLiteral("Spacecraft");
    case VehicleClassMultiRotor:
        return QStringLiteral("MultiRotor");
    case VehicleClassVTOL:
        return QStringLiteral("VTOL");
    case VehicleClassGeneric:
        return QStringLiteral("Generic");
    default:
        return QStringLiteral("Unknown");
    }
}

QString QGCMAVLink::mavResultToString(uint8_t result)
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
        MAV_SYS_STATUS_SENSOR bit;
        const char*           sensorName;
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
        { MAV_SYS_STATUS_SENSOR_PROPULSION,             QT_TRANSLATE_NOOP("MAVLink SYS_STATUS_SENSOR value", "Propulsion") }
    };

    for (size_t i=0; i<sizeof(rgSensorInfo)/sizeof(sensorInfo_s); i++) {
        const sensorInfo_s* pSensorInfo = &rgSensorInfo[i];
        if (sysStatusSensor == pSensorInfo->bit) {
            return QCoreApplication::translate("MAVLink SYS_STATUS_SENSOR value", pSensorInfo->sensorName);
        }
    }

    qWarning() << "QGCMAVLink::mavSysStatusSensorToString: Unknown sensor" << sysStatusSensor;

    return QCoreApplication::translate("MAVLink unknown SYS_STATUS_SENSOR value", "Unknown sensor");
}

QString QGCMAVLink::mavTypeToString(MAV_TYPE mavType) {
    switch (mavType) {
    case MAV_TYPE_GENERIC:                      return QCoreApplication::translate("MAV_TYPE", "Generic micro air vehicle");
    case MAV_TYPE_FIXED_WING:                   return QCoreApplication::translate("MAV_TYPE", "Fixed wing aircraft");
    case MAV_TYPE_QUADROTOR:                    return QCoreApplication::translate("MAV_TYPE", "Quadrotor");
    case MAV_TYPE_COAXIAL:                      return QCoreApplication::translate("MAV_TYPE", "Coaxial helicopter");
    case MAV_TYPE_HELICOPTER:                   return QCoreApplication::translate("MAV_TYPE", "Normal helicopter with tail rotor.");
    case MAV_TYPE_ANTENNA_TRACKER:              return QCoreApplication::translate("MAV_TYPE", "Ground installation");
    case MAV_TYPE_GCS:                          return QCoreApplication::translate("MAV_TYPE", "Operator control unit / ground control station");
    case MAV_TYPE_AIRSHIP:                      return QCoreApplication::translate("MAV_TYPE", "Airship, controlled");
    case MAV_TYPE_FREE_BALLOON:                 return QCoreApplication::translate("MAV_TYPE", "Free balloon, uncontrolled");
    case MAV_TYPE_ROCKET:                       return QCoreApplication::translate("MAV_TYPE", "Rocket");
    case MAV_TYPE_GROUND_ROVER:                 return QCoreApplication::translate("MAV_TYPE", "Ground rover");
    case MAV_TYPE_SURFACE_BOAT:                 return QCoreApplication::translate("MAV_TYPE", "Surface vessel, boat, ship");
    case MAV_TYPE_SUBMARINE:                    return QCoreApplication::translate("MAV_TYPE", "Submarine");
    case MAV_TYPE_SPACECRAFT_ORBITER:           return QCoreApplication::translate("MAV_TYPE", "Spacecraft, orbiter");
    case MAV_TYPE_HEXAROTOR:                    return QCoreApplication::translate("MAV_TYPE", "Hexarotor");
    case MAV_TYPE_OCTOROTOR:                    return QCoreApplication::translate("MAV_TYPE", "Octorotor");
    case MAV_TYPE_TRICOPTER:                    return QCoreApplication::translate("MAV_TYPE", "trirotor");
    case MAV_TYPE_FLAPPING_WING:                return QCoreApplication::translate("MAV_TYPE", "Flapping wing");
    case MAV_TYPE_KITE:                         return QCoreApplication::translate("MAV_TYPE", "Kite");
    case MAV_TYPE_ONBOARD_CONTROLLER:           return QCoreApplication::translate("MAV_TYPE", "Onboard companion controller");
    case MAV_TYPE_VTOL_TAILSITTER_DUOROTOR:     return QCoreApplication::translate("MAV_TYPE", "Two-rotor VTOL using control surfaces in vertical operation in addition. Tailsitter");
    case MAV_TYPE_VTOL_TAILSITTER_QUADROTOR:    return QCoreApplication::translate("MAV_TYPE", "Quad-rotor VTOL using a V-shaped quad config in vertical operation. Tailsitter");
    case MAV_TYPE_VTOL_TILTROTOR:               return QCoreApplication::translate("MAV_TYPE", "Tiltrotor VTOL");
    case MAV_TYPE_VTOL_FIXEDROTOR:              return QCoreApplication::translate("MAV_TYPE", "VTOL Fixedrotor");
    case MAV_TYPE_VTOL_TAILSITTER:              return QCoreApplication::translate("MAV_TYPE", "VTOL Tailsitter");
    case MAV_TYPE_VTOL_TILTWING:                return QCoreApplication::translate("MAV_TYPE", "VTOL Tiltwing");
    case MAV_TYPE_VTOL_RESERVED5:               return QCoreApplication::translate("MAV_TYPE", "VTOL reserved 5");
    case MAV_TYPE_GIMBAL:                       return QCoreApplication::translate("MAV_TYPE", "Onboard gimbal");
    case MAV_TYPE_ADSB:                         return QCoreApplication::translate("MAV_TYPE", "Onboard ADSB peripheral");
    default:                                    return QStringLiteral("MAV_TYPE_UNKNOWN");
    }
}

QString QGCMAVLink::firmwareVersionTypeToString(FIRMWARE_VERSION_TYPE firmwareVersionType)
{
    switch (firmwareVersionType) {
    case FIRMWARE_VERSION_TYPE_DEV:
        return QStringLiteral("dev");
    case FIRMWARE_VERSION_TYPE_ALPHA:
        return QStringLiteral("alpha");
    case FIRMWARE_VERSION_TYPE_BETA:
        return QStringLiteral("beta");
    case FIRMWARE_VERSION_TYPE_RC:
        return QStringLiteral("rc");
    case FIRMWARE_VERSION_TYPE_OFFICIAL:
    default:
        return QStringLiteral("");
    }
}

FIRMWARE_VERSION_TYPE QGCMAVLink::firmwareVersionTypeFromString(const QString &typeStr)
{
    const QString type = typeStr.trimmed();
    if (type == QLatin1String("dev")) {
        return FIRMWARE_VERSION_TYPE_DEV;
    }
    if (type == QLatin1String("alpha")) {
        return FIRMWARE_VERSION_TYPE_ALPHA;
    }
    if (type == QLatin1String("beta")) {
        return FIRMWARE_VERSION_TYPE_BETA;
    }
    if (type == QLatin1String("rc")) {
        return FIRMWARE_VERSION_TYPE_RC;
    }
    return FIRMWARE_VERSION_TYPE_OFFICIAL;
}

int QGCMAVLink::motorCount(MAV_TYPE mavType, uint8_t frameType)
{
    switch (mavType) {
    case MAV_TYPE_HELICOPTER:
        return 1;
    case MAV_TYPE_VTOL_TAILSITTER_DUOROTOR:
        return 2;
    case MAV_TYPE_TRICOPTER:
        return 3;
    case MAV_TYPE_QUADROTOR:
    case MAV_TYPE_VTOL_TAILSITTER_QUADROTOR:
        return 4;
    case MAV_TYPE_HEXAROTOR:
        return 6;
    case MAV_TYPE_OCTOROTOR:
        return 8;
    case MAV_TYPE_SUBMARINE:
    {
        // Supported frame types
        enum {
            SUB_FRAME_BLUEROV1,
            SUB_FRAME_VECTORED,
            SUB_FRAME_VECTORED_6DOF,
            SUB_FRAME_VECTORED_6DOF_90DEG,
            SUB_FRAME_SIMPLEROV_3,
            SUB_FRAME_SIMPLEROV_4,
            SUB_FRAME_SIMPLEROV_5,
            SUB_FRAME_CUSTOM
        };

        switch (frameType) {  // ardupilot/libraries/AP_Motors/AP_Motors6DOF.h sub_frame_t

        case SUB_FRAME_BLUEROV1:
        case SUB_FRAME_VECTORED:
            return 6;

        case SUB_FRAME_SIMPLEROV_3:
            return 3;

        case SUB_FRAME_SIMPLEROV_4:
            return 4;

        case SUB_FRAME_SIMPLEROV_5:
            return 5;

        case SUB_FRAME_VECTORED_6DOF:
        case SUB_FRAME_VECTORED_6DOF_90DEG:
        case SUB_FRAME_CUSTOM:
            return 8;

        default:
            return -1;
        }
    }
    case MAV_TYPE_SPACECRAFT_ORBITER:
        return 8;

    default:
        return -1;
    }
}

uint32_t QGCMAVLink::highLatencyFailuresToMavSysStatus(mavlink_high_latency2_t& highLatency2)
{
    struct failure2Sensor_s {
        HL_FAILURE_FLAG         failureBit;
        MAV_SYS_STATUS_SENSOR   sensorBit;
    };

    static constexpr const failure2Sensor_s rgFailure2Sensor[] = {
        { HL_FAILURE_FLAG_GPS,                      MAV_SYS_STATUS_SENSOR_GPS },
        { HL_FAILURE_FLAG_DIFFERENTIAL_PRESSURE,    MAV_SYS_STATUS_SENSOR_DIFFERENTIAL_PRESSURE },
        { HL_FAILURE_FLAG_ABSOLUTE_PRESSURE,        MAV_SYS_STATUS_SENSOR_ABSOLUTE_PRESSURE },
        { HL_FAILURE_FLAG_3D_ACCEL,                 MAV_SYS_STATUS_SENSOR_3D_ACCEL },
        { HL_FAILURE_FLAG_3D_GYRO,                  MAV_SYS_STATUS_SENSOR_3D_GYRO },
        { HL_FAILURE_FLAG_3D_MAG,                   MAV_SYS_STATUS_SENSOR_3D_MAG },
    };

    // Map from MAV_FAILURE bits to standard SYS_STATUS message handling
    uint32_t onboardControlSensorsEnabled = 0;
    for (size_t i=0; i<sizeof(rgFailure2Sensor)/sizeof(failure2Sensor_s); i++) {
        const failure2Sensor_s* pFailure2Sensor = &rgFailure2Sensor[i];
        if (highLatency2.failure_flags & pFailure2Sensor->failureBit) {
            // Assume if reporting as unhealthy that is it present and enabled
            onboardControlSensorsEnabled |= pFailure2Sensor->sensorBit;
        }
    }

    return onboardControlSensorsEnabled;
}

QString QGCMAVLink::compIdToString(uint8_t compId)
{
    QString compIdStr;

    if (mavlinkCompIdHash.contains(compId)) {
        compIdStr = mavlinkCompIdHash.value(compId);
    } else {
        compIdStr = QStringLiteral("Unknown");
    }

    return QStringLiteral("%1 (%2)").arg(compIdStr).arg(static_cast<int>(compId));
}
