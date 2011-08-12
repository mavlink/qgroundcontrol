/** @file
 *	@brief MAVLink comm protocol.
 *	@see http://qgroundcontrol.org/mavlink/
 *	 Generated on Friday, August 12 2011, 12:18 UTC
 */
#ifndef SENSESOAR_H
#define SENSESOAR_H

#ifdef __cplusplus
extern "C" {
#endif


#include "../protocol.h"

#define MAVLINK_ENABLED_SENSESOAR


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

/** @brief   Different flight modes  */
enum SENSESOAR_MODE
{
	SENSESOAR_MODE_GLIDING=0, /*  */
	SENSESOAR_MODE_AUTONOMOUS=1, /*  */
	SENSESOAR_MODE_MANUAL=2, /*  */
	SENSESOAR_MODE_ENUM_END
};


// MESSAGE DEFINITIONS

#include "./mavlink_msg_obs_position.h"
#include "./mavlink_msg_obs_velocity.h"
#include "./mavlink_msg_obs_attitude.h"
#include "./mavlink_msg_obs_wind.h"
#include "./mavlink_msg_obs_air_velocity.h"
#include "./mavlink_msg_obs_bias.h"
#include "./mavlink_msg_obs_qff.h"
#include "./mavlink_msg_obs_air_temp.h"
#include "./mavlink_msg_filt_rot_vel.h"
#include "./mavlink_msg_llc_out.h"
#include "./mavlink_msg_pm_elec.h"
#include "./mavlink_msg_sys_stat.h"
#include "./mavlink_msg_cmd_airspeed_chng.h"
#include "./mavlink_msg_cmd_airspeed_ack.h"


// MESSAGE LENGTHS

#undef MAVLINK_MESSAGE_LENGTHS
#define MAVLINK_MESSAGE_LENGTHS { 3, 4, 8, 14, 8, 28, 3, 32, 0, 2, 3, 2, 2, 0, 0, 0, 0, 0, 0, 0, 19, 2, 23, 21, 0, 37, 26, 101, 26, 16, 32, 32, 37, 32, 11, 17, 17, 16, 18, 36, 4, 4, 2, 2, 4, 2, 2, 3, 14, 12, 18, 16, 8, 27, 25, 0, 0, 0, 0, 0, 5, 5, 26, 16, 36, 5, 6, 0, 0, 21, 0, 0, 0, 18, 20, 20, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 24, 0, 12, 0, 32, 0, 12, 0, 12, 0, 24, 0, 4, 4, 12, 0, 12, 0, 20, 0, 4, 0, 5, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 30, 14, 14, 51 }

#ifdef __cplusplus
}
#endif
#endif
