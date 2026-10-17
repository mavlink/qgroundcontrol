#pragma once

#include "MAVLinkMessageType.h"

// #define MAVLINK_NO_SIGN_PACKET
// #define MAVLINK_NO_SIGNATURE_CHECK
#define MAVLINK_USE_MESSAGE_INFO

#include <stddef.h>

// moc needs no message definitions, and preprocessing the full dialect after some Qt headers takes moc seconds per
// header. MAVLinkMessageType.h above still provides the base types.
#ifndef Q_MOC_RUN

// Ignore warnings from mavlink headers for both GCC/Clang and MSVC
#ifdef __GNUC__
#   if __GNUC__ > 8
#       pragma GCC diagnostic push
#       pragma GCC diagnostic ignored "-Waddress-of-packed-member"
#   else
#       pragma GCC diagnostic push
#       pragma GCC diagnostic ignored "-Wall"
#   endif
#else
#   pragma warning(push, 0)
#endif

#include <mavlink.h>

#ifdef __GNUC__
#	pragma GCC diagnostic pop
#else
#	pragma warning(pop, 0)
#endif

#endif  // Q_MOC_RUN
