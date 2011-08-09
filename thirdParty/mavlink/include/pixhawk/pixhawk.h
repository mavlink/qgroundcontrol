/** @file
 *	@brief MAVLink comm protocol.
 *	@see http://qgroundcontrol.org/mavlink/
 *	 Generated on Tuesday, August 9 2011, 16:16 UTC
 */
#ifndef PIXHAWK_H
#define PIXHAWK_H

#ifdef __cplusplus
extern "C" {
#endif


#include "../protocol.h"

#define MAVLINK_ENABLED_PIXHAWK


#include "../common/common.h"
// MAVLINK VERSION

#ifndef MAVLINK_VERSION
#define MAVLINK_VERSION 0
#endif

#if (MAVLINK_VERSION == 0)
#undef MAVLINK_VERSION
#define MAVLINK_VERSION 0
#endif

// ENUM DEFINITIONS

/** @brief Content Types for data transmission handshake */
enum DATA_TYPES
{
	DATA_TYPE_JPEG_IMAGE=1,
	DATA_TYPE_RAW_IMAGE=2,
	DATA_TYPE_KINECT=3,
	DATA_TYPES_ENUM_END
};


// MESSAGE DEFINITIONS

#include "./mavlink_msg_attitude_control.h"
#include "./mavlink_msg_set_cam_shutter.h"
#include "./mavlink_msg_image_triggered.h"
#include "./mavlink_msg_image_trigger_control.h"
#include "./mavlink_msg_image_available.h"
#include "./mavlink_msg_vision_position_estimate.h"
#include "./mavlink_msg_vicon_position_estimate.h"
#include "./mavlink_msg_vision_speed_estimate.h"
#include "./mavlink_msg_position_control_setpoint_set.h"
#include "./mavlink_msg_position_control_offset_set.h"
#include "./mavlink_msg_position_control_setpoint.h"
#include "./mavlink_msg_marker.h"
#include "./mavlink_msg_raw_aux.h"
#include "./mavlink_msg_aux_status.h"
#include "./mavlink_msg_watchdog_heartbeat.h"
#include "./mavlink_msg_watchdog_process_info.h"
#include "./mavlink_msg_watchdog_process_status.h"
#include "./mavlink_msg_watchdog_command.h"
#include "./mavlink_msg_pattern_detected.h"
#include "./mavlink_msg_point_of_interest.h"
#include "./mavlink_msg_point_of_interest_connection.h"
#include "./mavlink_msg_data_transmission_handshake.h"
#include "./mavlink_msg_encapsulated_data.h"
#include "./mavlink_msg_brief_feature.h"


// MESSAGE CRC KEYS

#undef MAVLINK_MESSAGE_KEYS
#define MAVLINK_MESSAGE_KEYS { 71, 249, 232, 226, 76, 126, 117, 186, 0, 165, 44, 249, 133, 0, 0, 0, 0, 0, 0, 0, 112, 34, 81, 152, 0, 223, 28, 99, 28, 21, 243, 240, 204, 21, 111, 43, 192, 234, 22, 197, 192, 192, 166, 34, 233, 34, 166, 158, 142, 60, 10, 75, 249, 247, 234, 157, 125, 0, 0, 0, 85, 211, 62, 75, 185, 18, 42, 38, 0, 127, 200, 0, 0, 53, 251, 20, 22, 0, 0, 0, 0, 0, 0, 0, 0, 127, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 36, 8, 238, 165, 0, 0, 0, 0, 0, 0, 0, 218, 218, 235, 0, 0, 0, 0, 0, 0, 225, 114, 0, 0, 0, 0, 0, 0, 0, 0, 221, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 171, 122, 0, 0, 0, 0, 0, 0, 0, 92, 114, 4, 169, 10, 0, 0, 0, 0, 0, 72, 62, 83, 0, 0, 0, 0, 0, 0, 0, 200, 135, 217, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 43, 141, 211, 166 }

#ifdef __cplusplus
}
#endif
#endif
