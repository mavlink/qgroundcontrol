/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QList>

#include "MAVLinkLib.h"

class QGCMAVLink : public QObject
{
    Q_OBJECT

public:
    // Creating an instance of QGCMAVLink is only meant to be used for the Qml Singleton
    QGCMAVLink(QObject* parent = nullptr);

    typedef int FirmwareClass_t;
    typedef int VehicleClass_t;

    static constexpr FirmwareClass_t FirmwareClassPX4       = MAV_AUTOPILOT_PX4;
    static constexpr FirmwareClass_t FirmwareClassArduPilot = MAV_AUTOPILOT_ARDUPILOTMEGA;
    static constexpr FirmwareClass_t FirmwareClassGeneric   = MAV_AUTOPILOT_GENERIC;

    static constexpr VehicleClass_t VehicleClassAirship     = MAV_TYPE_AIRSHIP;
    static constexpr VehicleClass_t VehicleClassFixedWing   = MAV_TYPE_FIXED_WING;
    static constexpr VehicleClass_t VehicleClassRoverBoat   = MAV_TYPE_GROUND_ROVER;
    static constexpr VehicleClass_t VehicleClassSub         = MAV_TYPE_SUBMARINE;
    static constexpr VehicleClass_t VehicleClassMultiRotor  = MAV_TYPE_QUADROTOR;
    static constexpr VehicleClass_t VehicleClassVTOL        = MAV_TYPE_VTOL_TAILSITTER_QUADROTOR;
    static constexpr VehicleClass_t VehicleClassGeneric     = MAV_TYPE_GENERIC;

    static constexpr uint8_t        maxRcChannels           = 18; // mavlink_rc_channels_t->chancount

    static bool                     isPX4FirmwareClass          (MAV_AUTOPILOT autopilot) { return autopilot == MAV_AUTOPILOT_PX4; }
    static bool                     isArduPilotFirmwareClass    (MAV_AUTOPILOT autopilot) { return autopilot == MAV_AUTOPILOT_ARDUPILOTMEGA; }
    static bool                     isGenericFirmwareClass      (MAV_AUTOPILOT autopilot) { return !isPX4FirmwareClass(autopilot) && ! isArduPilotFirmwareClass(autopilot); }
    static FirmwareClass_t          firmwareClass               (MAV_AUTOPILOT autopilot);
    static MAV_AUTOPILOT            firmwareClassToAutopilot    (FirmwareClass_t firmwareClass) { return static_cast<MAV_AUTOPILOT>(firmwareClass); }
    static QString                  firmwareClassToString       (FirmwareClass_t firmwareClass);
    static QList<FirmwareClass_t>   allFirmwareClasses          (void);

    static bool                     isAirship                   (MAV_TYPE mavType);
    static bool                     isFixedWing                 (MAV_TYPE mavType);
    static bool                     isRoverBoat                 (MAV_TYPE mavType);
    static bool                     isSub                       (MAV_TYPE mavType);
    static bool                     isMultiRotor                (MAV_TYPE mavType);
    static bool                     isVTOL                      (MAV_TYPE mavType);
    static VehicleClass_t           vehicleClass                (MAV_TYPE mavType);
    static MAV_TYPE                 vehicleClassToMavType       (VehicleClass_t vehicleClass) { return static_cast<MAV_TYPE>(vehicleClass); }
    static QString                  vehicleClassToUserVisibleString(VehicleClass_t vehicleClass);
    static QString                  vehicleClassToInternalString(VehicleClass_t vehicleClass);
    static QList<VehicleClass_t>    allVehicleClasses           (void);

    static QString                  mavResultToString           (MAV_RESULT result);
    static QString                  mavSysStatusSensorToString  (MAV_SYS_STATUS_SENSOR sysStatusSensor);

    // Expose mavlink enums to Qml. I've tried various way to make this work without duping, but haven't found anything that works.

    enum MAV_BATTERY_FUNCTION {
        MAV_BATTERY_FUNCTION_UNKNOWN=0, /* Battery function is unknown | */
        MAV_BATTERY_FUNCTION_ALL=1, /* Battery supports all flight systems | */
        MAV_BATTERY_FUNCTION_PROPULSION=2, /* Battery for the propulsion system | */
        MAV_BATTERY_FUNCTION_AVIONICS=3, /* Avionics battery | */
        MAV_BATTERY_TYPE_PAYLOAD=4, /* Payload battery | */
    };
    Q_ENUM(MAV_BATTERY_FUNCTION)

    enum MAV_BATTERY_CHARGE_STATE
    {
       MAV_BATTERY_CHARGE_STATE_UNDEFINED=0, /* Low battery state is not provided | */
       MAV_BATTERY_CHARGE_STATE_OK=1, /* Battery is not in low state. Normal operation. | */
       MAV_BATTERY_CHARGE_STATE_LOW=2, /* Battery state is low, warn and monitor close. | */
       MAV_BATTERY_CHARGE_STATE_CRITICAL=3, /* Battery state is critical, return or abort immediately. | */
       MAV_BATTERY_CHARGE_STATE_EMERGENCY=4, /* Battery state is too low for ordinary abort sequence. Perform fastest possible emergency stop to prevent damage. | */
       MAV_BATTERY_CHARGE_STATE_FAILED=5, /* Battery failed, damage unavoidable. | */
       MAV_BATTERY_CHARGE_STATE_UNHEALTHY=6, /* Battery is diagnosed to be defective or an error occurred, usage is discouraged / prohibited. | */
       MAV_BATTERY_CHARGE_STATE_CHARGING=7, /* Battery is charging. | */
    };
    Q_ENUM(MAV_BATTERY_CHARGE_STATE)
};
