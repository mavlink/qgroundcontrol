#pragma once

#define MAVLINK_USE_MESSAGE_INFO
#define MAVLINK_EXTERNAL_RX_STATUS // Single m_mavlink_status instance is in QGCApplication.cc

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

extern mavlink_status_t m_mavlink_status[MAVLINK_COMM_NUM_BUFFERS];

#include <mavlink.h>

#ifdef __GNUC__
#	pragma GCC diagnostic pop
#else
#	pragma warning(pop, 0)
#endif
