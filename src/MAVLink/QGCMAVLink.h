#pragma once

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

#include "MAVLinkEnums.h"
#include "MAVLinkMessageType.h"
#include "QGCMAVLinkTypes.h"

class QGCMAVLink : public QObject, public QGCMAVLinkTypes
{
    Q_OBJECT
    QML_NAMED_ELEMENT(MAVLink)
    QML_SINGLETON

public:
    // Creating an instance of QGCMAVLink is only meant to be used for the Qml Singleton
    QGCMAVLink(QObject *parent = nullptr);
    ~QGCMAVLink();

    // FirmwareClass_t, VehicleClass_t, VehicleClassGeneric, maxRcChannels inherited from QGCMAVLinkTypes

    static constexpr const FirmwareClass_t FirmwareClassPX4       = MAV_AUTOPILOT_PX4;
    static constexpr const FirmwareClass_t FirmwareClassArduPilot = MAV_AUTOPILOT_ARDUPILOTMEGA;
    static constexpr const FirmwareClass_t FirmwareClassGeneric   = MAV_AUTOPILOT_GENERIC;

    static constexpr const VehicleClass_t VehicleClassAirship     = MAV_TYPE_AIRSHIP;
    static constexpr const VehicleClass_t VehicleClassFixedWing   = MAV_TYPE_FIXED_WING;
    static constexpr const VehicleClass_t VehicleClassRoverBoat   = MAV_TYPE_GROUND_ROVER;
    static constexpr const VehicleClass_t VehicleClassSub         = MAV_TYPE_SUBMARINE;
    static constexpr const VehicleClass_t VehicleClassSpacecraft  = MAV_TYPE_SPACECRAFT_ORBITER;
    static constexpr const VehicleClass_t VehicleClassMultiRotor  = MAV_TYPE_QUADROTOR;
    static constexpr const VehicleClass_t VehicleClassVTOL        = MAV_TYPE_VTOL_TAILSITTER_QUADROTOR;
    // VehicleClassGeneric inherited from QGCMAVLinkTypes
    static_assert(QGCMAVLinkTypes::VehicleClassGeneric == MAV_TYPE_GENERIC, "VehicleClassGeneric value mismatch");

    static bool                     isPX4FirmwareClass          (MAV_AUTOPILOT autopilot) { return autopilot == MAV_AUTOPILOT_PX4; }
    static bool                     isArduPilotFirmwareClass    (MAV_AUTOPILOT autopilot) { return autopilot == MAV_AUTOPILOT_ARDUPILOTMEGA; }
    static bool                     isGenericFirmwareClass      (MAV_AUTOPILOT autopilot) { return !isPX4FirmwareClass(autopilot) && ! isArduPilotFirmwareClass(autopilot); }
    static FirmwareClass_t          firmwareClass               (MAV_AUTOPILOT autopilot);
    static MAV_AUTOPILOT            firmwareClassToAutopilot    (FirmwareClass_t firmwareClass) { return static_cast<MAV_AUTOPILOT>(firmwareClass); }
    static QString                  firmwareClassToString       (FirmwareClass_t firmwareClass);
    static MAV_AUTOPILOT            firmwareTypeFromString      (const QString &firmwareTypeStr);
    static QList<FirmwareClass_t>   allFirmwareClasses          ();

    static bool                     isAirship                   (MAV_TYPE mavType);
    static bool                     isFixedWing                 (MAV_TYPE mavType);
    static bool                     isRoverBoat                 (MAV_TYPE mavType);
    static bool                     isSub                       (MAV_TYPE mavType);
    static bool                     isSpacecraft                (MAV_TYPE mavType);
    static bool                     isMultiRotor                (MAV_TYPE mavType);
    static bool                     isVTOL                      (MAV_TYPE mavType);
    static VehicleClass_t           vehicleClass                (MAV_TYPE mavType);
    static MAV_TYPE                 vehicleClassToMavType       (VehicleClass_t vehicleClass) { return static_cast<MAV_TYPE>(vehicleClass); }
    static QString                  vehicleClassToUserVisibleString(VehicleClass_t vehicleClass);
    static QString                  vehicleClassToInternalString(VehicleClass_t vehicleClass);
    static MAV_TYPE                 vehicleTypeFromString(const QString &vehicleStr);
    static QList<VehicleClass_t>    allVehicleClasses           (void);

    static QString                  mavResultToString           (uint8_t result);
    static QString                  mavResultToString           (MAV_RESULT result) { return mavResultToString(static_cast<uint8_t>(result)); }
    static QString                  mavSysStatusSensorToString  (MAV_SYS_STATUS_SENSOR sysStatusSensor);
    static QString                  mavTypeToString             (MAV_TYPE mavType);
    static QString                  firmwareVersionTypeToString (FIRMWARE_VERSION_TYPE firmwareVersionType);
    static FIRMWARE_VERSION_TYPE    firmwareVersionTypeFromString(const QString &typeStr);
    static int                      motorCount                  (MAV_TYPE mavType, uint8_t frameType = 0);
    static uint32_t                 highLatencyFailuresToMavSysStatus(mavlink_high_latency2_t& highLatency2);
    static QString                  compIdToString              (uint8_t compId);

    enum CalibrationType {
        CalibrationNone,
        CalibrationRadio,
        CalibrationGyro,
        CalibrationMag,
        CalibrationAccel,
        CalibrationLevel,
        CalibrationEsc,
        CalibrationCopyTrims,
        CalibrationAPMCompassMot,
        CalibrationAPMPressureAirspeed,
        CalibrationAPMPreFlight,
        CalibrationPX4Airspeed,
        CalibrationPX4Pressure,
        CalibrationAPMAccelSimple,
    };
    Q_ENUM(CalibrationType)

    MAVPACKED(
    typedef struct param_ext_union {
        union {
            float param_float;
            double param_double;
            int64_t param_int64;
            uint64_t param_uint64;
            int32_t param_int32;
            uint32_t param_uint32;
            int16_t param_int16;
            uint16_t param_uint16;
            int8_t param_int8;
            uint8_t param_uint8;
            uint8_t bytes[MAVLINK_MSG_PARAM_EXT_SET_FIELD_PARAM_VALUE_LEN];
        };
        uint8_t type;
    }) param_ext_union_t;

    static bool isValidChannel(uint8_t channel) { return (channel < MAVLINK_COMM_NUM_BUFFERS); }
    static bool isValidChannel(mavlink_channel_t channel) { return isValidChannel(static_cast<uint8_t>(channel)); }

    static mavlink_status_t* getChannelStatus(mavlink_channel_t channel) { return mavlink_get_channel_status(static_cast<uint8_t>(channel)); }

    static const QHash<int, QString> mavlinkCompIdHash;
};
