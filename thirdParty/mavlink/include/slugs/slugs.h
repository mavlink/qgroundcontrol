/** @file
 *	@brief MAVLink comm protocol.
 *	@see http://qgroundcontrol.org/mavlink/
 *	 Generated on Tuesday, August 9 2011, 16:16 UTC
 */
#ifndef SLUGS_H
#define SLUGS_H

#ifdef __cplusplus
extern "C" {
#endif


#include "../protocol.h"

#define MAVLINK_ENABLED_SLUGS


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


// MESSAGE DEFINITIONS

#include "./mavlink_msg_cpu_load.h"
#include "./mavlink_msg_air_data.h"
#include "./mavlink_msg_sensor_bias.h"
#include "./mavlink_msg_diagnostic.h"
#include "./mavlink_msg_slugs_navigation.h"
#include "./mavlink_msg_data_log.h"
#include "./mavlink_msg_gps_date_time.h"
#include "./mavlink_msg_mid_lvl_cmds.h"
#include "./mavlink_msg_ctrl_srfc_pt.h"
#include "./mavlink_msg_slugs_action.h"


// MESSAGE CRC KEYS

#undef MAVLINK_MESSAGE_KEYS
#define MAVLINK_MESSAGE_KEYS { 71, 249, 232, 226, 76, 126, 117, 186, 0, 165, 44, 249, 133, 0, 0, 0, 0, 0, 0, 0, 112, 34, 81, 152, 0, 223, 28, 99, 28, 21, 243, 240, 204, 21, 111, 43, 192, 234, 22, 197, 192, 192, 166, 34, 233, 34, 166, 158, 142, 60, 10, 75, 249, 247, 234, 157, 125, 0, 0, 0, 85, 211, 62, 75, 185, 18, 42, 38, 0, 127, 200, 0, 0, 53, 251, 20, 22, 0, 0, 0, 0, 0, 0, 0, 0, 127, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 36, 8, 238, 165, 0, 0, 0, 0, 0, 0, 0, 218, 218, 235, 0, 0, 0, 0, 0, 0, 225, 114, 0, 0, 0, 0, 0, 0, 0, 0, 221, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 171, 122, 0, 0, 0, 0, 0, 0, 0, 92, 114, 4, 169, 10, 0, 0, 0, 0, 0, 72, 62, 83, 0, 0, 0, 0, 0, 0, 0, 202, 144, 106, 254, 0, 0, 255, 185, 0, 14, 136, 53, 0, 212, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 43, 141, 211, 166 }

#ifdef __cplusplus
}
#endif
#endif
