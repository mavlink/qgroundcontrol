/** @file
 *	@brief MAVLink comm protocol.
 *	@see http://qgroundcontrol.org/mavlink/
 *	 Generated on Saturday, August 13 2011, 07:20 UTC
 */
#ifndef MINIMAL_H
#define MINIMAL_H

#ifdef __cplusplus
extern "C" {
#endif


#include "../mavlink_protocol.h"

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

// MESSAGE LENGTHS

#undef MAVLINK_MESSAGE_LENGTHS
#define MAVLINK_MESSAGE_LENGTHS {  }

#ifdef __cplusplus
}
#endif
#endif
