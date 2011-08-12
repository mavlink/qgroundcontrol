/** @file
 *	@brief MAVLink comm protocol.
 *	@see http://qgroundcontrol.org/mavlink/
 *	 Generated on Tuesday, August 9 2011, 16:16 UTC
 */
#ifndef MINIMAL_H
#define MINIMAL_H

#ifdef __cplusplus
extern "C" {
#endif


#include "../protocol.h"

#define MAVLINK_ENABLED_MINIMAL

// MAVLINK VERSION

#ifndef MAVLINK_VERSION
#define MAVLINK_VERSION 2
#endif

#if (MAVLINK_VERSION == 0)
#undef MAVLINK_VERSION
#define MAVLINK_VERSION 2
#endif

// ENUM DEFINITIONS


// MESSAGE DEFINITIONS

#include "./mavlink_msg_heartbeat.h"


// MESSAGE CRC KEYS

#undef MAVLINK_MESSAGE_KEYS
#define MAVLINK_MESSAGE_KEYS {  }

#ifdef __cplusplus
}
#endif
#endif
