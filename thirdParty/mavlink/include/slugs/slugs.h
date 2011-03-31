/** @file
 *	@brief MAVLink comm protocol.
 *	@see http://pixhawk.ethz.ch/software/mavlink
 *	 Generated on Thursday, March 31 2011, 22:06 UTC
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
#ifdef __cplusplus
}
#endif
#endif
