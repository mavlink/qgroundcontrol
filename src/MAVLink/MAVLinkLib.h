#pragma once

#define MAVLINK_USE_MESSAGE_INFO
#define MAVLINK_EXTERNAL_RX_STATUS // Single m_mavlink_status instance is in QGCApplication.cc
// #define MAVLINK_USE_CONVENIENCE_FUNCTIONS

#define HAVE_MAVLINK_CHANNEL_T
#ifdef HAVE_MAVLINK_CHANNEL_T
typedef enum {
    MAVLINK_COMM_0,
    MAVLINK_COMM_1,
    MAVLINK_COMM_2,
    MAVLINK_COMM_3,
    MAVLINK_COMM_4,
    MAVLINK_COMM_5,
    MAVLINK_COMM_6,
    MAVLINK_COMM_7,
    MAVLINK_COMM_8,
    MAVLINK_COMM_9,
    MAVLINK_COMM_10,
    MAVLINK_COMM_11,
    MAVLINK_COMM_12,
    MAVLINK_COMM_13,
    MAVLINK_COMM_14,
    MAVLINK_COMM_15
} mavlink_channel_t;
#endif

#define MAVLINK_COMM_NUM_BUFFERS 16
#define MAVLINK_MAX_SIGNING_STREAMS MAVLINK_COMM_NUM_BUFFERS

#include <stddef.h> // Hack workaround for Mav 2.0 header problem with respect to offsetof usage

// Ignore warnings from mavlink headers for both GCC/Clang and MSVC
#ifdef __GNUC__
#	if __GNUC__ > 8
#		pragma GCC diagnostic push
#		pragma GCC diagnostic ignored "-Waddress-of-packed-member"
#	else
#		pragma GCC diagnostic push
#		pragma GCC diagnostic ignored "-Wall"
#	endif
#else
#	pragma warning(push, 0)
#endif

#include <mavlink_types.h>

#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
	extern mavlink_system_t MAVLinkProtocol::getMavlinkSystem();
	#define mavlink_system (mavlink_system_t) { MAVLinkProtocol::getSystemId(), MAVLinkProtocol::getComponentId() };

	extern void mavInitBuffer( mavlink_channel_t xChan, size_t uxLength );
	extern void mavBuildBuffer( mavlink_channel_t xChan, const uint8_t * pucBuf, size_t uxLength );
	extern void mavSendBuffer( mavlink_channel_t xChan, size_t uxLength );

	#define MAVLINK_START_UART_SEND( chan, length ) mavInitBuffer( chan, length )
	#define MAVLINK_SEND_UART_BYTES( chan, buf, length ) mavBuildBuffer( chan, buf, length )
	#define MAVLINK_END_UART_SEND( chan, length ) mavSendBuffer( chan, length )
#endif

extern mavlink_status_t m_mavlink_status[MAVLINK_COMM_NUM_BUFFERS];

#include <mavlink.h>

#ifdef __GNUC__
#	pragma GCC diagnostic pop
#else
#	pragma warning(pop, 0)
#endif
