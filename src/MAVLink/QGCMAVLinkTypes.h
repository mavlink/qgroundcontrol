#pragma once

/// QGC MAVLink type aliases, constants, and MAVLink struct forward-declarations.
/// Include from headers that only need pointers/references to MAVLink structs;
/// include MAVLinkLib.h in the implementation TU to dereference them.

#include <cstdint>

struct QGCMAVLinkTypes {
    typedef int FirmwareClass_t;
    typedef int VehicleClass_t;

    static constexpr VehicleClass_t VehicleClassGeneric = 0; // Must match MAV_TYPE_GENERIC

    static constexpr uint8_t maxRcChannels = 18;
};

#ifndef MAVLINK_MSG_PARAM_EXT_SET_FIELD_PARAM_VALUE_LEN
#define MAVLINK_MSG_PARAM_EXT_SET_FIELD_PARAM_VALUE_LEN 128
#endif

typedef struct __mavlink_message mavlink_message_t;
typedef struct __mavlink_command_ack_t mavlink_command_ack_t;
typedef struct __mavlink_command_long_t mavlink_command_long_t;
typedef struct __mavlink_obstacle_distance_t mavlink_obstacle_distance_t;
typedef struct __mavlink_camera_information_t mavlink_camera_information_t;
typedef struct __mavlink_high_latency2_t mavlink_high_latency2_t;
typedef struct __mavlink_event_t mavlink_event_t;
typedef struct __mavlink_request_event_t mavlink_request_event_t;
typedef struct param_union mavlink_param_union_t;
