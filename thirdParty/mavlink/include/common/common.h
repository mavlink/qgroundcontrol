/** @file
 *	@brief MAVLink comm protocol.
 *	@see http://qgroundcontrol.org/mavlink/
 *	 Generated on Saturday, August 13 2011, 07:20 UTC
 */
#ifndef COMMON_H
#define COMMON_H

#ifdef __cplusplus
extern "C" {
#endif


#include "../mavlink_protocol.h"

#define MAVLINK_ENABLED_COMMON

// MAVLINK VERSION

#ifndef MAVLINK_VERSION
#define MAVLINK_VERSION 3
#endif

#if (MAVLINK_VERSION == 0)
#undef MAVLINK_VERSION
#define MAVLINK_VERSION 3
#endif

// ENUM DEFINITIONS

/** @brief Micro air vehicle / autopilot classes. This identifies the individual model. */
enum MAV_CLASS
{
	MAV_CLASS_GENERIC=0, /* Generic autopilot, full support for everything */
	MAV_CLASS_PIXHAWK=1, /* PIXHAWK autopilot, http://pixhawk.ethz.ch */
	MAV_CLASS_SLUGS=2, /* SLUGS autopilot, http://slugsuav.soe.ucsc.edu */
	MAV_CLASS_ARDUPILOTMEGA=3, /* ArduPilotMega / ArduCopter, http://diydrones.com */
	MAV_CLASS_OPENPILOT=4, /* OpenPilot, http://openpilot.org */
	MAV_CLASS_GENERIC_MISSION_WAYPOINTS_ONLY=5, /* Generic autopilot only supporting simple waypoints */
	MAV_CLASS_GENERIC_MISSION_NAVIGATION_ONLY=6, /* Generic autopilot supporting waypoints and other simple navigation commands */
	MAV_CLASS_GENERIC_MISSION_FULL=7, /* Generic autopilot supporting the full mission command set */
	MAV_CLASS_INVALID=8, /* No valid autopilot, e.g. a GCS or other MAVLink component */
	MAV_CLASS_PPZ=9, /* PPZ UAV - http://nongnu.org/paparazzi */
	MAV_CLASS_UDB=10, /* UAV Dev Board */
	MAV_CLASS_FP=11, /* FlexiPilot */
	MAV_CLASS_ENUM_END
};

/** @brief  */
enum MAV_MODE
{
	MAV_MODE_UNINIT=0, /* System is in undefined state */
	MAV_MODE_LOCKED=1, /* Motors are blocked, system is safe */
	MAV_MODE_MANUAL=2, /* System is allowed to be active, under manual (RC) control */
	MAV_MODE_GUIDED=3, /* System is allowed to be active, under autonomous control, manual setpoint */
	MAV_MODE_AUTO=4, /* System is allowed to be active, under autonomous control and navigation */
	MAV_MODE_TEST1=5, /* Generic test mode, for custom use */
	MAV_MODE_TEST2=6, /* Generic test mode, for custom use */
	MAV_MODE_TEST3=7, /* Generic test mode, for custom use */
	MAV_MODE_READY=8, /* System is ready, motors are unblocked, but controllers are inactive */
	MAV_MODE_RC_TRAINING=9, /* System is blocked, only RC valued are read and reported back */
	MAV_MODE_ENUM_END
};

/** @brief  */
enum MAV_NAV
{
	MAV_NAV_GROUNDED=0, /* System is currently on ground. */
	MAV_NAV_LIFTOFF=1, /* System is during liftoff, not in normal navigation mode yet. */
	MAV_NAV_HOLD=2, /* System is holding its current position (rotorcraft or rover / boat). */
	MAV_NAV_WAYPOINT=3, /* System is navigating towards the next waypoint. */
	MAV_NAV_VECTOR=4, /* System is flying at a defined course and speed. */
	MAV_NAV_RETURNING=5, /* System is return straight to home position. */
	MAV_NAV_LANDING=6, /* System is landing. */
	MAV_NAV_LOST=7, /* System lost its position input and is in attitude / flight stabilization only. */
	MAV_NAV_LOITER=8, /* System is loitering in wait position. DO NOT USE THIS FOR WAYPOINT LOITER! */
	MAV_NAV_ENUM_END
};

/** @brief  */
enum MAV_STATE
{
	MAV_STATE_UNINIT=0, /* Uninitialized system, state is unknown. */
	MAV_STATE_BOOT=1, /* System is booting up. */
	MAV_STATE_CALIBRATING=2, /* System is calibrating and not flight-ready. */
	MAV_STATE_STANDBY=3, /* System is grounded and on standby. It can be launched any time. */
	MAV_STATE_ACTIVE=4, /* System is active and might be already airborne. Motors are engaged. */
	MAV_STATE_CRITICAL=5, /* System is in a non-normal flight mode. It can however still navigate. */
	MAV_STATE_EMERGENCY=6, /* System is in a non-normal flight mode. It lost control over parts or over the whole airframe. It is in mayday and going down. */
	MAV_STATE_POWEROFF=7, /* System just initialized its power-down sequence, will shut down now. */
	MAV_STATE_ENUM_END
};

/** @brief Data stream IDs. A data stream is not a fixed set of messages, but rather a recommendation to the autopilot software. Individual autopilots may or may not obey the recommended messages. */
enum MAV_DATA_STREAM
{
	MAV_DATA_STREAM_ALL=0, /* Enable all data streams */
	MAV_DATA_STREAM_RAW_SENSORS=1, /* Enable IMU_RAW, GPS_RAW, GPS_STATUS packets. */
	MAV_DATA_STREAM_EXTENDED_STATUS=2, /* Enable GPS_STATUS, CONTROL_STATUS, AUX_STATUS */
	MAV_DATA_STREAM_RC_CHANNELS=3, /* Enable RC_CHANNELS_SCALED, RC_CHANNELS_RAW, SERVO_OUTPUT_RAW */
	MAV_DATA_STREAM_RAW_CONTROLLER=4, /* Enable ATTITUDE_CONTROLLER_OUTPUT, POSITION_CONTROLLER_OUTPUT, NAV_CONTROLLER_OUTPUT. */
	MAV_DATA_STREAM_POSITION=6, /* Enable LOCAL_POSITION, GLOBAL_POSITION/GLOBAL_POSITION_INT messages. */
	MAV_DATA_STREAM_EXTRA1=10, /* Dependent on the autopilot */
	MAV_DATA_STREAM_EXTRA2=11, /* Dependent on the autopilot */
	MAV_DATA_STREAM_EXTRA3=12, /* Dependent on the autopilot */
	MAV_DATA_STREAM_ENUM_END
};

/** @brief The ROI (region of interest) for the vehicle. This can be be used by the vehicle for camera/vehicle attitude alignment (see MAV_CMD_NAV_ROI). */
enum MAV_ROI
{
	MAV_ROI_WPNEXT=0, /* Point toward next waypoint. */
	MAV_ROI_WPINDEX=1, /* Point toward given waypoint. */
	MAV_ROI_LOCATION=2, /* Point toward fixed location. */
	MAV_ROI_TARGET=3, /* Point toward of given id. */
	MAV_ROI_ENUM_END
};

/** @brief  */
enum MAV_TYPE
{
	MAV_TYPE_GENERIC=0, /* Generic micro air vehicle. */
	MAV_TYPE_FIXED_WING=1, /* Fixed wing aircraft. */
	MAV_TYPE_QUADROTOR=2, /* Quadrotor */
	MAV_TYPE_COAXIAL=3, /* Coaxial helicopter */
	MAV_TYPE_HELICOPTER=4, /* Normal helicopter with tail rotor. */
	MAV_TYPE_GROUND=5, /* Ground installation */
	MAV_TYPE_OCU=6, /* Operator control unit / ground control station */
	MAV_TYPE_AIRSHIP=7, /* Airship, controlled */
	MAV_TYPE_FREE_BALLOON=8, /* Free balloon, uncontrolled */
	MAV_TYPE_ROCKET=9, /* Rocket */
	MAV_TYPE_UGV_GROUND_ROVER=10, /* Ground rover */
	MAV_TYPE_UGV_SURFACE_SHIP=11, /* Surface vessel, boat, ship */
	MAV_TYPE_UGV_SUBMARINE=12, /* Submarine */
	MAV_TYPE_HEXAROTOR=13, /* Hexarotor */
	MAV_TYPE_OCTOROTOR=14, /* Octorotor */
	MAV_TYPE_ENUM_END
};

/** @brief  */
enum MAV_COMPONENT
{
	MAV_COMP_ID_GPS=220,
	MAV_COMP_ID_WAYPOINTPLANNER=190,
	MAV_COMP_ID_PATHPLANNER=195,
	MAV_COMP_ID_MAPPER=180,
	MAV_COMP_ID_CAMERA=100,
	MAV_COMP_ID_IMU=200,
	MAV_COMP_ID_IMU_2=201,
	MAV_COMP_ID_IMU_3=202,
	MAV_COMP_ID_UDP_BRIDGE=240,
	MAV_COMP_ID_UART_BRIDGE=241,
	MAV_COMP_ID_SYSTEM_CONTROL=250,
	MAV_COMP_ID_SERVO1=140,
	MAV_COMP_ID_SERVO2=141,
	MAV_COMP_ID_SERVO3=142,
	MAV_COMP_ID_SERVO4=143,
	MAV_COMP_ID_SERVO5=144,
	MAV_COMP_ID_SERVO6=145,
	MAV_COMP_ID_SERVO7=146,
	MAV_COMP_ID_SERVO8=147,
	MAV_COMP_ID_SERVO9=148,
	MAV_COMP_ID_SERVO10=149,
	MAV_COMP_ID_SERVO11=150,
	MAV_COMP_ID_SERVO12=151,
	MAV_COMP_ID_SERVO13=152,
	MAV_COMP_ID_SERVO14=153,
	MAV_COMPONENT_ENUM_END
};

/** @brief  */
enum MAV_FRAME
{
	MAV_FRAME_GLOBAL=0, /* Global coordinate frame, WGS84 coordinate system. */
	MAV_FRAME_LOCAL_NED=1, /* Local coordinate frame, Z-up (x: north, y: east, z: down). */
	MAV_FRAME_MISSION=2, /* NOT a coordinate frame, indicates a mission command. */
	MAV_FRAME_GLOBAL_RELATIVE_ALT=3, /* Global coordinate frame, WGS84 coordinate system, relative altitude over ground. */
	MAV_FRAME_LOCAL_ENU=4, /* Local coordinate frame, Z-down (x: east, y: north, z: up) */
	MAV_FRAME_ENUM_END
};

/** @brief  */
enum MAVLINK_DATA_STREAM_TYPE
{
	MAVLINK_DATA_STREAM_IMG_JPEG=0,
	MAVLINK_DATA_STREAM_IMG_BMP=1,
	MAVLINK_DATA_STREAM_IMG_RAW8U=2,
	MAVLINK_DATA_STREAM_IMG_RAW32U=3,
	MAVLINK_DATA_STREAM_IMG_PGM=4,
	MAVLINK_DATA_STREAM_IMG_PNG=5,
	MAVLINK_DATA_STREAM_TYPE_ENUM_END
};

/** @brief Commands to be executed by the MAV. They can be executed on user request, or as part of a mission script. If the action is used in a mission, the parameter mapping to the waypoint/mission message is as follows: Param 1, Param 2, Param 3, Param 4, X: Param 5, Y:Param 6, Z:Param 7. This command list is similar what ARINC 424 is for commercial aircraft: A data format how to interpret waypoint/mission data. */
enum MAV_CMD
{
	MAV_CMD_NAV_WAYPOINT=16, /* Navigate to waypoint. | Hold time in decimal seconds. (ignored by fixed wing, time to stay at waypoint for rotary wing) | Acceptance radius in meters (if the sphere with this radius is hit, the waypoint counts as reached) | 0 to pass through the WP, if > 0 radius in meters to pass by WP. Positive value for clockwise orbit, negative value for counter-clockwise orbit. Allows trajectory control. | Desired yaw angle at waypoint (rotary wing) | Latitude | Longitude | Altitude */
	MAV_CMD_NAV_LOITER_UNLIM=17, /* Loiter around this waypoint an unlimited amount of time | Empty | Empty | Radius around waypoint, in meters. If positive loiter clockwise, else counter-clockwise | Desired yaw angle. | Latitude | Longitude | Altitude */
	MAV_CMD_NAV_LOITER_TURNS=18, /* Loiter around this waypoint for X turns | Turns | Empty | Radius around waypoint, in meters. If positive loiter clockwise, else counter-clockwise | Desired yaw angle. | Latitude | Longitude | Altitude */
	MAV_CMD_NAV_LOITER_TIME=19, /* Loiter around this waypoint for X seconds | Seconds (decimal) | Empty | Radius around waypoint, in meters. If positive loiter clockwise, else counter-clockwise | Desired yaw angle. | Latitude | Longitude | Altitude */
	MAV_CMD_NAV_RETURN_TO_LAUNCH=20, /* Return to launch location | Empty | Empty | Empty | Empty | Empty | Empty | Empty */
	MAV_CMD_NAV_LAND=21, /* Land at location | Empty | Empty | Empty | Desired yaw angle. | Latitude | Longitude | Altitude */
	MAV_CMD_NAV_TAKEOFF=22, /* Takeoff from ground / hand | Minimum pitch (if airspeed sensor present), desired pitch without sensor | Empty | Empty | Yaw angle (if magnetometer present), ignored without magnetometer | Latitude | Longitude | Altitude */
	MAV_CMD_NAV_ROI=80, /* Sets the region of interest (ROI) for a sensor set or the vehicle itself. This can then be used by the vehicles control system to control the vehicle attitude and the attitude of various sensors such as cameras. | Region of intereset mode. (see MAV_ROI enum) | Waypoint index/ target ID. (see MAV_ROI enum) | ROI index (allows a vehicle to manage multiple ROI's) | Empty | x the location of the fixed ROI (see MAV_FRAME) | y | z */
	MAV_CMD_NAV_PATHPLANNING=81, /* Control autonomous path planning on the MAV. | 0: Disable local obstacle avoidance / local path planning (without resetting map), 1: Enable local path planning, 2: Enable and reset local path planning | 0: Disable full path planning (without resetting map), 1: Enable, 2: Enable and reset map/occupancy grid, 3: Enable and reset planned route, but not occupancy grid | Empty | Yaw angle at goal, in compass degrees, [0..360] | Latitude/X of goal | Longitude/Y of goal | Altitude/Z of goal */
	MAV_CMD_NAV_LAST=95, /* NOP - This command is only used to mark the upper limit of the NAV/ACTION commands in the enumeration | Empty | Empty | Empty | Empty | Empty | Empty | Empty */
	MAV_CMD_CONDITION_DELAY=112, /* Delay mission state machine. | Delay in seconds (decimal) | Empty | Empty | Empty | Empty | Empty | Empty */
	MAV_CMD_CONDITION_CHANGE_ALT=113, /* Ascend/descend at rate. Delay mission state machine until desired altitude reached. | Descent / Ascend rate (m/s) | Empty | Empty | Empty | Empty | Empty | Finish Altitude */
	MAV_CMD_CONDITION_DISTANCE=114, /* Delay mission state machine until within desired distance of next NAV point. | Distance (meters) | Empty | Empty | Empty | Empty | Empty | Empty */
	MAV_CMD_CONDITION_YAW=115, /* Reach a certain target angle. | target angle: [0-360], 0 is north | speed during yaw change:[deg per second] | direction: negative: counter clockwise, positive: clockwise [-1,1] | relative offset or absolute angle: [ 1,0] | Empty | Empty | Empty */
	MAV_CMD_CONDITION_LAST=159, /* NOP - This command is only used to mark the upper limit of the CONDITION commands in the enumeration | Empty | Empty | Empty | Empty | Empty | Empty | Empty */
	MAV_CMD_DO_SET_MODE=176, /* Set system mode. | Mode, as defined by ENUM MAV_MODE | Empty | Empty | Empty | Empty | Empty | Empty */
	MAV_CMD_DO_JUMP=177, /* Jump to the desired command in the mission list. Repeat this action only the specified number of times | Sequence number | Repeat count | Empty | Empty | Empty | Empty | Empty */
	MAV_CMD_DO_CHANGE_SPEED=178, /* Change speed and/or throttle set points. | Speed type (0=Airspeed, 1=Ground Speed) | Speed (m/s, -1 indicates no change) | Throttle ( Percent, -1 indicates no change) | Empty | Empty | Empty | Empty */
	MAV_CMD_DO_SET_HOME=179, /* Changes the home location either to the current location or a specified location. | Use current (1=use current location, 0=use specified location) | Empty | Empty | Empty | Latitude | Longitude | Altitude */
	MAV_CMD_DO_SET_PARAMETER=180, /* Set a system parameter. Caution! Use of this command requires knowledge of the numeric enumeration value of the parameter. | Parameter number | Parameter value | Empty | Empty | Empty | Empty | Empty */
	MAV_CMD_DO_SET_RELAY=181, /* Set a relay to a condition. | Relay number | Setting (1=on, 0=off, others possible depending on system hardware) | Empty | Empty | Empty | Empty | Empty */
	MAV_CMD_DO_REPEAT_RELAY=182, /* Cycle a relay on and off for a desired number of cyles with a desired period. | Relay number | Cycle count | Cycle time (seconds, decimal) | Empty | Empty | Empty | Empty */
	MAV_CMD_DO_SET_SERVO=183, /* Set a servo to a desired PWM value. | Servo number | PWM (microseconds, 1000 to 2000 typical) | Empty | Empty | Empty | Empty | Empty */
	MAV_CMD_DO_REPEAT_SERVO=184, /* Cycle a between its nominal setting and a desired PWM for a desired number of cycles with a desired period. | Servo number | PWM (microseconds, 1000 to 2000 typical) | Cycle count | Cycle time (seconds) | Empty | Empty | Empty */
	MAV_CMD_DO_CONTROL_VIDEO=200, /* Control onboard camera system. | Camera ID (-1 for all) | Transmission: 0: disabled, 1: enabled compressed, 2: enabled raw | Transmission mode: 0: video stream, >0: single images every n seconds (decimal) | Recording: 0: disabled, 1: enabled compressed, 2: enabled raw | Empty | Empty | Empty */
	MAV_CMD_DO_LAST=240, /* NOP - This command is only used to mark the upper limit of the DO commands in the enumeration | Empty | Empty | Empty | Empty | Empty | Empty | Empty */
	MAV_CMD_PREFLIGHT_CALIBRATION=241, /* Trigger calibration. This command will be only accepted if in pre-flight mode. | Gyro calibration: 0: no, 1: yes | Magnetometer calibration: 0: no, 1: yes | Ground pressure: 0: no, 1: yes | Radio calibration: 0: no, 1: yes | Empty | Empty | Empty */
	MAV_CMD_PREFLIGHT_STORAGE=245, /* Request storage of different parameter values and logs. This command will be only accepted if in pre-flight mode. | Parameter storage: 0: READ FROM FLASH/EEPROM, 1: WRITE CURRENT TO FLASH/EEPROM | Mission storage: 0: READ FROM FLASH/EEPROM, 1: WRITE CURRENT TO FLASH/EEPROM | Reserved | Reserved | Empty | Empty | Empty */
	MAV_CMD_ENUM_END
};


// MESSAGE DEFINITIONS

#include "./mavlink_msg_heartbeat.h"
#include "./mavlink_msg_boot.h"
#include "./mavlink_msg_system_time.h"
#include "./mavlink_msg_ping.h"
#include "./mavlink_msg_system_time_utc.h"
#include "./mavlink_msg_change_operator_control.h"
#include "./mavlink_msg_change_operator_control_ack.h"
#include "./mavlink_msg_auth_key.h"
#include "./mavlink_msg_cmd_ack.h"
#include "./mavlink_msg_set_mode.h"
#include "./mavlink_msg_set_nav_mode.h"
#include "./mavlink_msg_param_request_read.h"
#include "./mavlink_msg_param_request_list.h"
#include "./mavlink_msg_param_value.h"
#include "./mavlink_msg_param_set.h"
#include "./mavlink_msg_gps_raw_int.h"
#include "./mavlink_msg_scaled_imu.h"
#include "./mavlink_msg_gps_status.h"
#include "./mavlink_msg_raw_imu.h"
#include "./mavlink_msg_raw_pressure.h"
#include "./mavlink_msg_scaled_pressure.h"
#include "./mavlink_msg_attitude.h"
#include "./mavlink_msg_local_position.h"
#include "./mavlink_msg_global_position.h"
#include "./mavlink_msg_gps_raw.h"
#include "./mavlink_msg_sys_status.h"
#include "./mavlink_msg_rc_channels_raw.h"
#include "./mavlink_msg_rc_channels_scaled.h"
#include "./mavlink_msg_servo_output_raw.h"
#include "./mavlink_msg_waypoint.h"
#include "./mavlink_msg_waypoint_request.h"
#include "./mavlink_msg_waypoint_set_current.h"
#include "./mavlink_msg_waypoint_current.h"
#include "./mavlink_msg_waypoint_request_list.h"
#include "./mavlink_msg_waypoint_count.h"
#include "./mavlink_msg_waypoint_clear_all.h"
#include "./mavlink_msg_waypoint_reached.h"
#include "./mavlink_msg_waypoint_ack.h"
#include "./mavlink_msg_gps_set_global_origin.h"
#include "./mavlink_msg_gps_local_origin_set.h"
#include "./mavlink_msg_local_position_setpoint_set.h"
#include "./mavlink_msg_local_position_setpoint.h"
#include "./mavlink_msg_control_status.h"
#include "./mavlink_msg_safety_set_allowed_area.h"
#include "./mavlink_msg_safety_allowed_area.h"
#include "./mavlink_msg_set_roll_pitch_yaw_thrust.h"
#include "./mavlink_msg_set_roll_pitch_yaw_speed_thrust.h"
#include "./mavlink_msg_roll_pitch_yaw_thrust_setpoint.h"
#include "./mavlink_msg_roll_pitch_yaw_speed_thrust_setpoint.h"
#include "./mavlink_msg_nav_controller_output.h"
#include "./mavlink_msg_position_target.h"
#include "./mavlink_msg_state_correction.h"
#include "./mavlink_msg_set_altitude.h"
#include "./mavlink_msg_request_data_stream.h"
#include "./mavlink_msg_full_state.h"
#include "./mavlink_msg_manual_control.h"
#include "./mavlink_msg_rc_channels_override.h"
#include "./mavlink_msg_global_position_int.h"
#include "./mavlink_msg_vfr_hud.h"
#include "./mavlink_msg_command.h"
#include "./mavlink_msg_command_ack.h"
#include "./mavlink_msg_memory_vect.h"
#include "./mavlink_msg_debug_vect.h"
#include "./mavlink_msg_named_value_float.h"
#include "./mavlink_msg_named_value_int.h"
#include "./mavlink_msg_statustext.h"
#include "./mavlink_msg_debug.h"


// MESSAGE CRC KEYS

#undef MAVLINK_MESSAGE_KEYS
#define MAVLINK_MESSAGE_KEYS { 71, 249, 232, 226, 76, 126, 117, 186, 0, 144, 0, 249, 133, 0, 0, 0, 0, 0, 0, 0, 33, 34, 163, 45, 0, 166, 28, 99, 28, 21, 243, 240, 91, 21, 111, 43, 192, 234, 22, 197, 192, 192, 166, 34, 233, 34, 166, 158, 142, 60, 10, 75, 249, 247, 234, 161, 116, 56, 245, 0, 0, 0, 62, 75, 185, 18, 42, 38, 0, 127, 200, 0, 0, 212, 251, 20, 22, 0, 0, 0, 0, 0, 0, 0, 0, 127, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 36, 8, 238, 165, 0, 0, 0, 0, 0, 0, 0, 218, 218, 235, 0, 0, 0, 0, 0, 0, 225, 114, 0, 0, 0, 0, 0, 0, 0, 0, 221, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 171, 122, 0, 0, 0, 0, 0, 0, 0, 92, 99, 4, 169, 10, 0, 0, 0, 0, 0, 52, 163, 16, 0, 0, 0, 0, 0, 0, 0, 200, 135, 217, 254, 0, 0, 255, 185, 0, 14, 136, 53, 0, 212, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 73, 239, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 138, 43, 141, 211, 144 }

// MESSAGE LENGTHS

#undef MAVLINK_MESSAGE_LENGTHS
#define MAVLINK_MESSAGE_LENGTHS { 3, 4, 8, 14, 8, 28, 3, 32, 0, 2, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 20, 2, 24, 22, 0, 30, 26, 101, 26, 16, 32, 32, 38, 32, 11, 17, 17, 16, 18, 36, 4, 4, 2, 2, 4, 2, 2, 3, 14, 12, 18, 16, 8, 27, 25, 18, 18, 20, 20, 0, 0, 0, 26, 16, 36, 5, 6, 56, 0, 21, 18, 0, 0, 20, 20, 20, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 36, 30, 14, 14, 51 }

#ifdef __cplusplus
}
#endif
#endif
