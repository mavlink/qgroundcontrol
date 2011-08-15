/** @file
 *	@brief MAVLink comm protocol.
 *	@see http://qgroundcontrol.org/mavlink/
 *	 Generated on Wednesday, July 27 2011, 14:17 UTC
 */
#ifndef COMMON_H
#define COMMON_H

#ifdef __cplusplus
extern "C" {
#endif


#include "../protocol.h"

#define MAVLINK_ENABLED_COMMON

// MAVLINK VERSION

#ifndef MAVLINK_VERSION
#define MAVLINK_VERSION 2
#endif

#if (MAVLINK_VERSION == 0)
#undef MAVLINK_VERSION
#define MAVLINK_VERSION 2
#endif

// ENUM DEFINITIONS

/** @brief  Commands to be executed by the MAV. They can be executed on user request,       or as part of a mission script. If the action is used in a mission, the parameter mapping       to the waypoint/mission message is as follows:       Param 1, Param 2, Param 3, Param 4, X: Param 5, Y:Param 6, Z:Param 7. This command list is similar what       ARINC 424 is for commercial aircraft: A data format how to interpret waypoint/mission data. */
enum MAV_CMD
{
	MAV_CMD_NAV_WAYPOINT=16, /* Navigate to waypoint. | Hold time in decimal seconds. (ignored by fixed wing, time to stay at waypoint for rotary wing) | Acceptance radius in meters (if the sphere with this radius is hit, the waypoint counts as reached) | 0 to pass through the WP, if > 0 radius in meters to pass by WP. Positive value for clockwise orbit, negative value for counter-clockwise orbit. Allows trajectory control. | Desired yaw angle at waypoint (rotary wing) | Latitude | Longitude | Altitude | */
	MAV_CMD_NAV_LOITER_UNLIM=17, /* Loiter around this waypoint an unlimited amount of time | Empty | Empty | Radius around waypoint, in meters. If positive loiter clockwise, else counter-clockwise | Desired yaw angle. | Latitude | Longitude | Altitude | */
	MAV_CMD_NAV_LOITER_TURNS=18, /* Loiter around this waypoint for X turns | Turns | Empty | Radius around waypoint, in meters. If positive loiter clockwise, else counter-clockwise | Desired yaw angle. | Latitude | Longitude | Altitude | */
	MAV_CMD_NAV_LOITER_TIME=19, /* Loiter around this waypoint for X seconds | Seconds (decimal) | Empty | Radius around waypoint, in meters. If positive loiter clockwise, else counter-clockwise | Desired yaw angle. | Latitude | Longitude | Altitude | */
	MAV_CMD_NAV_RETURN_TO_LAUNCH=20, /* Return to launch location | Empty | Empty | Empty | Empty | Empty | Empty | Empty | */
	MAV_CMD_NAV_LAND=21, /* Land at location | Empty | Empty | Empty | Desired yaw angle. | Latitude | Longitude | Altitude | */
	MAV_CMD_NAV_TAKEOFF=22, /* Takeoff from ground / hand | Minimum pitch (if airspeed sensor present), desired pitch without sensor | Empty | Empty | Yaw angle (if magnetometer present), ignored without magnetometer | Latitude | Longitude | Altitude | */
	MAV_CMD_NAV_ROI=80, /* Sets the region of interest (ROI) for a sensor set or the vehicle itself. This can then be used by the vehicles control system to control the vehicle attitude and the attitude of various sensors such as cameras. | Region of intereset mode. (see MAV_ROI enum) | Waypoint index/ target ID. (see MAV_ROI enum) | ROI index (allows a vehicle to manage multiple ROI's) | Empty | x the location of the fixed ROI (see MAV_FRAME) | y | z | */
	MAV_CMD_NAV_PATHPLANNING=81, /* Control autonomous path planning on the MAV. | 0: Disable local obstacle avoidance / local path planning (without resetting map), 1: Enable local path planning, 2: Enable and reset local path planning | 0: Disable full path planning (without resetting map), 1: Enable, 2: Enable and reset map/occupancy grid, 3: Enable and reset planned route, but not occupancy grid | Empty | Yaw angle at goal, in compass degrees, [0..360] | Latitude/X of goal | Longitude/Y of goal | Altitude/Z of goal | */
	MAV_CMD_NAV_LAST=95, /* NOP - This command is only used to mark the upper limit of the NAV/ACTION commands in the enumeration | Empty | Empty | Empty | Empty | Empty | Empty | Empty | */
	MAV_CMD_CONDITION_DELAY=112, /* Delay mission state machine. | Delay in seconds (decimal) | Empty | Empty | Empty | Empty | Empty | Empty | */
	MAV_CMD_CONDITION_CHANGE_ALT=113, /* Ascend/descend at rate. Delay mission state machine until desired altitude reached. | Descent / Ascend rate (m/s) | Empty | Empty | Empty | Empty | Empty | Finish Altitude | */
	MAV_CMD_CONDITION_DISTANCE=114, /* Delay mission state machine until within desired distance of next NAV point. | Distance (meters) | Empty | Empty | Empty | Empty | Empty | Empty | */
	MAV_CMD_CONDITION_YAW=115, /* Reach a certain target angle. | target angle: [0-360], 0 is north | speed during yaw change:[deg per second] | direction: negative: counter clockwise, positive: clockwise [-1,1] | relative offset or absolute angle: [ 1,0] | Empty | Empty | Empty | */
	MAV_CMD_CONDITION_LAST=159, /* NOP - This command is only used to mark the upper limit of the CONDITION commands in the enumeration | Empty | Empty | Empty | Empty | Empty | Empty | Empty | */
	MAV_CMD_DO_SET_MODE=176, /* Set system mode. | Mode, as defined by ENUM MAV_MODE | Empty | Empty | Empty | Empty | Empty | Empty | */
	MAV_CMD_DO_JUMP=177, /* Jump to the desired command in the mission list. Repeat this action only the specified number of times | Sequence number | Repeat count | Empty | Empty | Empty | Empty | Empty | */
	MAV_CMD_DO_CHANGE_SPEED=178, /* Change speed and/or throttle set points. | Speed type (0=Airspeed, 1=Ground Speed) | Speed (m/s, -1 indicates no change) | Throttle ( Percent, -1 indicates no change) | Empty | Empty | Empty | Empty | */
	MAV_CMD_DO_SET_HOME=179, /* Changes the home location either to the current location or a specified location. | Use current (1=use current location, 0=use specified location) | Empty | Empty | Empty | Latitude | Longitude | Altitude | */
	MAV_CMD_DO_SET_PARAMETER=180, /* Set a system parameter. Caution! Use of this command requires knowledge of the numeric enumeration value of the parameter. | Parameter number | Parameter value | Empty | Empty | Empty | Empty | Empty | */
	MAV_CMD_DO_SET_RELAY=181, /* Set a relay to a condition. | Relay number | Setting (1=on, 0=off, others possible depending on system hardware) | Empty | Empty | Empty | Empty | Empty | */
	MAV_CMD_DO_REPEAT_RELAY=182, /* Cycle a relay on and off for a desired number of cyles with a desired period. | Relay number | Cycle count | Cycle time (seconds, decimal) | Empty | Empty | Empty | Empty | */
	MAV_CMD_DO_SET_SERVO=183, /* Set a servo to a desired PWM value. | Servo number | PWM (microseconds, 1000 to 2000 typical) | Empty | Empty | Empty | Empty | Empty | */
	MAV_CMD_DO_REPEAT_SERVO=184, /* Cycle a between its nominal setting and a desired PWM for a desired number of cycles with a desired period. | Servo number | PWM (microseconds, 1000 to 2000 typical) | Cycle count | Cycle time (seconds) | Empty | Empty | Empty | */
	MAV_CMD_DO_CONTROL_VIDEO=200, /* Control onboard camera system. | Camera ID (-1 for all) | Transmission: 0: disabled, 1: enabled compressed, 2: enabled raw | Transmission mode: 0: video stream, >0: single images every n seconds (decimal) | Recording: 0: disabled, 1: enabled compressed, 2: enabled raw | Empty | Empty | Empty | */
	MAV_CMD_DO_LAST=240, /* NOP - This command is only used to mark the upper limit of the DO commands in the enumeration | Empty | Empty | Empty | Empty | Empty | Empty | Empty | */
	MAV_CMD_PREFLIGHT_CALIBRATION=241, /* Trigger calibration. This command will be only accepted if in pre-flight mode. | Gyro calibration: 0: no, 1: yes | Magnetometer calibration: 0: no, 1: yes | Ground pressure: 0: no, 1: yes | Radio calibration: 0: no, 1: yes | Empty | Empty | Empty | */
	MAV_CMD_PREFLIGHT_STORAGE=245, /* Request storage of different parameter values and logs. This command will be only accepted if in pre-flight mode. | Parameter storage: 0: READ FROM FLASH/EEPROM, 1: WRITE CURRENT TO FLASH/EEPROM | Mission storage: 0: READ FROM FLASH/EEPROM, 1: WRITE CURRENT TO FLASH/EEPROM | Reserved | Reserved | Empty | Empty | Empty | */
	MAV_CMD_ENUM_END
};

/** @brief  Data stream IDs. A data stream is not a fixed set of messages, but rather a      recommendation to the autopilot software. Individual autopilots may or may not obey      the recommended messages.       */
enum MAV_DATA_STREAM
{
	MAV_DATA_STREAM_ALL=0, /* Enable all data streams | */
	MAV_DATA_STREAM_RAW_SENSORS=1, /* Enable IMU_RAW, GPS_RAW, GPS_STATUS packets. | */
	MAV_DATA_STREAM_EXTENDED_STATUS=2, /* Enable GPS_STATUS, CONTROL_STATUS, AUX_STATUS | */
	MAV_DATA_STREAM_RC_CHANNELS=3, /* Enable RC_CHANNELS_SCALED, RC_CHANNELS_RAW, SERVO_OUTPUT_RAW | */
	MAV_DATA_STREAM_RAW_CONTROLLER=4, /* Enable ATTITUDE_CONTROLLER_OUTPUT, POSITION_CONTROLLER_OUTPUT, NAV_CONTROLLER_OUTPUT. | */
	MAV_DATA_STREAM_POSITION=6, /* Enable LOCAL_POSITION, GLOBAL_POSITION/GLOBAL_POSITION_INT messages. | */
	MAV_DATA_STREAM_EXTRA1=10, /* Dependent on the autopilot | */
	MAV_DATA_STREAM_EXTRA2=11, /* Dependent on the autopilot | */
	MAV_DATA_STREAM_EXTRA3=12, /* Dependent on the autopilot | */
	MAV_DATA_STREAM_ENUM_END
};

/** @brief   The ROI (region of interest) for the vehicle. This can be       be used by the vehicle for camera/vehicle attitude alignment (see 	  MAV_CMD_NAV_ROI).       */
enum MAV_ROI
{
	MAV_ROI_WPNEXT=0, /* Point toward next waypoint. | */
	MAV_ROI_WPINDEX=1, /* Point toward given waypoint. | */
	MAV_ROI_LOCATION=2, /* Point toward fixed location. | */
	MAV_ROI_TARGET=3, /* Point toward of given id. | */
	MAV_ROI_ENUM_END
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
#include "./mavlink_msg_action_ack.h"
#include "./mavlink_msg_action.h"
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
#include "./mavlink_msg_attitude_controller_output.h"
#include "./mavlink_msg_position_controller_output.h"
#include "./mavlink_msg_nav_controller_output.h"
#include "./mavlink_msg_position_target.h"
#include "./mavlink_msg_state_correction.h"
#include "./mavlink_msg_set_altitude.h"
#include "./mavlink_msg_request_data_stream.h"
#include "./mavlink_msg_manual_control.h"
#include "./mavlink_msg_global_position_int.h"
#include "./mavlink_msg_vfr_hud.h"
#include "./mavlink_msg_command.h"
#include "./mavlink_msg_command_ack.h"
#include "./mavlink_msg_debug_vect.h"
#include "./mavlink_msg_named_value_float.h"
#include "./mavlink_msg_named_value_int.h"
#include "./mavlink_msg_statustext.h"
#include "./mavlink_msg_debug.h"


// MESSAGE LENGTHS

#undef MAVLINK_MESSAGE_LENGTHS
#define MAVLINK_MESSAGE_LENGTHS { 3, 4, 8, 14, 8, 28, 3, 32, 0, 2, 3, 2, 2, 0, 0, 0, 0, 0, 0, 0, 19, 2, 23, 21, 0, 37, 26, 101, 26, 16, 32, 32, 37, 32, 11, 17, 17, 16, 18, 36, 4, 4, 2, 2, 4, 2, 2, 3, 14, 12, 18, 16, 8, 27, 25, 0, 0, 0, 0, 0, 5, 5, 26, 16, 36, 5, 6, 0, 0, 21, 0, 0, 0, 18, 20, 20, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 30, 14, 14, 51 }

#ifdef __cplusplus
}
#endif
#endif
