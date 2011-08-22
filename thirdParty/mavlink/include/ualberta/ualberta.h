/** @file
 *	@brief MAVLink comm protocol.
 *	@see http://qgroundcontrol.org/mavlink/
 *	 Generated on Saturday, August 20 2011, 11:06 UTC
 */
#ifndef UALBERTA_H
#define UALBERTA_H

#ifdef __cplusplus
extern "C" {
#endif


#include "../mavlink_protocol.h"

#define MAVLINK_ENABLED_UALBERTA


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

/** @brief Available autopilot modes for ualberta uav */
enum UALBERTA_AUTOPILOT_MODE
{
	MODE_MANUAL_DIRECT=0, /*  */
	MODE_MANUAL_SCALED=1, /*  */
	MODE_AUTO_PID_ATT=2, /*  */
	MODE_AUTO_PID_VEL=3, /*  */
	MODE_AUTO_PID_POS=4, /*  */
	UALBERTA_AUTOPILOT_MODE_ENUM_END
};

/** @brief Navigation filter mode */
enum UALBERTA_NAV_MODE
{
	NAV_AHRS_INIT=0,
	NAV_AHRS=1, /*  */
	NAV_INS_GPS_INIT=2, /*  */
	NAV_INS_GPS=3, /*  */
	UALBERTA_NAV_MODE_ENUM_END
};

/** @brief Mode currently commanded by pilot */
enum UALBERTA_PILOT_MODE
{
	PILOT_MANUAL=0, /*  */
	PILOT_AUTO=1, /*  */
	PILOT_ROTO=2, /*  */
	UALBERTA_PILOT_MODE_ENUM_END
};


// MESSAGE DEFINITIONS

#include "./mavlink_msg_nav_filter_bias.h"
#include "./mavlink_msg_radio_calibration.h"
#include "./mavlink_msg_ualberta_sys_status.h"


// MESSAGE CRC KEYS

#undef MAVLINK_MESSAGE_KEYS
#define MAVLINK_MESSAGE_KEYS { 179, 218, 22, 76, 226, 126, 117, 186, 0, 144, 0, 249, 172, 16, 0, 0, 0, 0, 0, 0, 33, 34, 191, 55, 0, 166, 28, 99, 28, 21, 243, 240, 91, 21, 111, 43, 192, 234, 22, 197, 192, 192, 166, 34, 233, 34, 166, 158, 142, 60, 10, 75, 249, 247, 234, 161, 116, 56, 245, 0, 0, 0, 62, 75, 185, 18, 42, 80, 0, 127, 200, 0, 0, 212, 251, 20, 38, 22, 0, 0, 0, 0, 0, 0, 0, 127, 0, 0, 0, 0, 18, 103, 59, 0, 0, 0, 0, 0, 0, 0, 74, 8, 238, 165, 0, 0, 0, 0, 0, 0, 0, 218, 218, 235, 0, 0, 0, 0, 0, 0, 225, 114, 0, 0, 0, 0, 0, 0, 0, 0, 221, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 171, 122, 0, 0, 0, 0, 0, 0, 0, 92, 99, 4, 169, 10, 0, 0, 0, 0, 0, 52, 163, 16, 0, 0, 0, 0, 0, 0, 0, 200, 135, 217, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 73, 239, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 138, 43, 141, 211, 144 }

// MESSAGE LENGTHS

#undef MAVLINK_MESSAGE_LENGTHS
#define MAVLINK_MESSAGE_LENGTHS { 8, 23, 12, 8, 14, 28, 3, 32, 0, 0, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 20, 2, 25, 23, 0, 30, 26, 101, 26, 16, 32, 32, 38, 32, 0, 17, 17, 16, 18, 36, 4, 4, 2, 2, 4, 2, 2, 3, 14, 12, 18, 16, 8, 27, 25, 18, 18, 20, 20, 0, 0, 0, 26, 16, 36, 0, 6, 4, 0, 21, 18, 0, 0, 20, 20, 20, 32, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 56, 42, 33, 0, 0, 0, 0, 0, 0, 0, 18, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 42, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 36, 30, 14, 14, 51 }

#ifdef __cplusplus
}
#endif
#endif
