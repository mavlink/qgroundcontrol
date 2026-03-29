#pragma once

/// Lightweight header providing only MAVLink base types (mavlink_message_t, mavlink_status_t,
/// mavlink_channel_t, etc.) WITHOUT pulling in the full dialect message definitions.
///
/// Use this instead of MAVLinkLib.h in headers that only reference mavlink_message_t in
/// function signatures. This avoids moc parsing ~118k lines of MAVLink message pack/unpack code.

#include <stdint.h>

#define HAVE_MAVLINK_CHANNEL_T
#ifdef HAVE_MAVLINK_CHANNEL_T
typedef enum : uint8_t {
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

#include <mavlink_types.h>

#define MAVLINK_EXTERNAL_RX_STATUS
#ifdef MAVLINK_EXTERNAL_RX_STATUS
    extern mavlink_status_t m_mavlink_status[MAVLINK_COMM_NUM_BUFFERS];
#endif

#define MAVLINK_GET_CHANNEL_STATUS
#ifdef MAVLINK_GET_CHANNEL_STATUS
    extern mavlink_status_t* mavlink_get_channel_status(uint8_t chan);
#endif
