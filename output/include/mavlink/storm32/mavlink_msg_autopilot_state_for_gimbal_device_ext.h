#pragma once
// MESSAGE AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT PACKING

#define MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT 60000


typedef struct __mavlink_autopilot_state_for_gimbal_device_ext_t {
 uint64_t time_boot_us; /*< [us] Timestamp (time since system boot).*/
 float wind_x; /*< [m/s] Wind X speed in NED (North,Est, Down). NAN if unknown.*/
 float wind_y; /*< [m/s] Wind Y speed in NED (North, East, Down). NAN if unknown.*/
 float wind_correction_angle; /*< [rad] Correction angle due to wind. NaN if unknown.*/
 uint8_t target_system; /*<  System ID.*/
 uint8_t target_component; /*<  Component ID.*/
} mavlink_autopilot_state_for_gimbal_device_ext_t;

#define MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_LEN 22
#define MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_MIN_LEN 22
#define MAVLINK_MSG_ID_60000_LEN 22
#define MAVLINK_MSG_ID_60000_MIN_LEN 22

#define MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_CRC 4
#define MAVLINK_MSG_ID_60000_CRC 4



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT { \
    60000, \
    "AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT", \
    6, \
    {  { "target_system", NULL, MAVLINK_TYPE_UINT8_T, 0, 20, offsetof(mavlink_autopilot_state_for_gimbal_device_ext_t, target_system) }, \
         { "target_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 21, offsetof(mavlink_autopilot_state_for_gimbal_device_ext_t, target_component) }, \
         { "time_boot_us", NULL, MAVLINK_TYPE_UINT64_T, 0, 0, offsetof(mavlink_autopilot_state_for_gimbal_device_ext_t, time_boot_us) }, \
         { "wind_x", NULL, MAVLINK_TYPE_FLOAT, 0, 8, offsetof(mavlink_autopilot_state_for_gimbal_device_ext_t, wind_x) }, \
         { "wind_y", NULL, MAVLINK_TYPE_FLOAT, 0, 12, offsetof(mavlink_autopilot_state_for_gimbal_device_ext_t, wind_y) }, \
         { "wind_correction_angle", NULL, MAVLINK_TYPE_FLOAT, 0, 16, offsetof(mavlink_autopilot_state_for_gimbal_device_ext_t, wind_correction_angle) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT { \
    "AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT", \
    6, \
    {  { "target_system", NULL, MAVLINK_TYPE_UINT8_T, 0, 20, offsetof(mavlink_autopilot_state_for_gimbal_device_ext_t, target_system) }, \
         { "target_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 21, offsetof(mavlink_autopilot_state_for_gimbal_device_ext_t, target_component) }, \
         { "time_boot_us", NULL, MAVLINK_TYPE_UINT64_T, 0, 0, offsetof(mavlink_autopilot_state_for_gimbal_device_ext_t, time_boot_us) }, \
         { "wind_x", NULL, MAVLINK_TYPE_FLOAT, 0, 8, offsetof(mavlink_autopilot_state_for_gimbal_device_ext_t, wind_x) }, \
         { "wind_y", NULL, MAVLINK_TYPE_FLOAT, 0, 12, offsetof(mavlink_autopilot_state_for_gimbal_device_ext_t, wind_y) }, \
         { "wind_correction_angle", NULL, MAVLINK_TYPE_FLOAT, 0, 16, offsetof(mavlink_autopilot_state_for_gimbal_device_ext_t, wind_correction_angle) }, \
         } \
}
#endif

/**
 * @brief Pack a autopilot_state_for_gimbal_device_ext message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system  System ID.
 * @param target_component  Component ID.
 * @param time_boot_us [us] Timestamp (time since system boot).
 * @param wind_x [m/s] Wind X speed in NED (North,Est, Down). NAN if unknown.
 * @param wind_y [m/s] Wind Y speed in NED (North, East, Down). NAN if unknown.
 * @param wind_correction_angle [rad] Correction angle due to wind. NaN if unknown.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_autopilot_state_for_gimbal_device_ext_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint8_t target_system, uint8_t target_component, uint64_t time_boot_us, float wind_x, float wind_y, float wind_correction_angle)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_LEN];
    _mav_put_uint64_t(buf, 0, time_boot_us);
    _mav_put_float(buf, 8, wind_x);
    _mav_put_float(buf, 12, wind_y);
    _mav_put_float(buf, 16, wind_correction_angle);
    _mav_put_uint8_t(buf, 20, target_system);
    _mav_put_uint8_t(buf, 21, target_component);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_LEN);
#else
    mavlink_autopilot_state_for_gimbal_device_ext_t packet;
    packet.time_boot_us = time_boot_us;
    packet.wind_x = wind_x;
    packet.wind_y = wind_y;
    packet.wind_correction_angle = wind_correction_angle;
    packet.target_system = target_system;
    packet.target_component = target_component;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_MIN_LEN, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_LEN, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_CRC);
}

/**
 * @brief Pack a autopilot_state_for_gimbal_device_ext message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system  System ID.
 * @param target_component  Component ID.
 * @param time_boot_us [us] Timestamp (time since system boot).
 * @param wind_x [m/s] Wind X speed in NED (North,Est, Down). NAN if unknown.
 * @param wind_y [m/s] Wind Y speed in NED (North, East, Down). NAN if unknown.
 * @param wind_correction_angle [rad] Correction angle due to wind. NaN if unknown.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_autopilot_state_for_gimbal_device_ext_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint8_t target_system, uint8_t target_component, uint64_t time_boot_us, float wind_x, float wind_y, float wind_correction_angle)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_LEN];
    _mav_put_uint64_t(buf, 0, time_boot_us);
    _mav_put_float(buf, 8, wind_x);
    _mav_put_float(buf, 12, wind_y);
    _mav_put_float(buf, 16, wind_correction_angle);
    _mav_put_uint8_t(buf, 20, target_system);
    _mav_put_uint8_t(buf, 21, target_component);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_LEN);
#else
    mavlink_autopilot_state_for_gimbal_device_ext_t packet;
    packet.time_boot_us = time_boot_us;
    packet.wind_x = wind_x;
    packet.wind_y = wind_y;
    packet.wind_correction_angle = wind_correction_angle;
    packet.target_system = target_system;
    packet.target_component = target_component;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_MIN_LEN, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_LEN, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_MIN_LEN, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_LEN);
#endif
}

/**
 * @brief Pack a autopilot_state_for_gimbal_device_ext message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system  System ID.
 * @param target_component  Component ID.
 * @param time_boot_us [us] Timestamp (time since system boot).
 * @param wind_x [m/s] Wind X speed in NED (North,Est, Down). NAN if unknown.
 * @param wind_y [m/s] Wind Y speed in NED (North, East, Down). NAN if unknown.
 * @param wind_correction_angle [rad] Correction angle due to wind. NaN if unknown.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_autopilot_state_for_gimbal_device_ext_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint8_t target_system,uint8_t target_component,uint64_t time_boot_us,float wind_x,float wind_y,float wind_correction_angle)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_LEN];
    _mav_put_uint64_t(buf, 0, time_boot_us);
    _mav_put_float(buf, 8, wind_x);
    _mav_put_float(buf, 12, wind_y);
    _mav_put_float(buf, 16, wind_correction_angle);
    _mav_put_uint8_t(buf, 20, target_system);
    _mav_put_uint8_t(buf, 21, target_component);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_LEN);
#else
    mavlink_autopilot_state_for_gimbal_device_ext_t packet;
    packet.time_boot_us = time_boot_us;
    packet.wind_x = wind_x;
    packet.wind_y = wind_y;
    packet.wind_correction_angle = wind_correction_angle;
    packet.target_system = target_system;
    packet.target_component = target_component;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_MIN_LEN, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_LEN, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_CRC);
}

/**
 * @brief Encode a autopilot_state_for_gimbal_device_ext struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param autopilot_state_for_gimbal_device_ext C-struct to read the message contents from
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_autopilot_state_for_gimbal_device_ext_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_autopilot_state_for_gimbal_device_ext_t* autopilot_state_for_gimbal_device_ext)
{
    return mavlink_msg_autopilot_state_for_gimbal_device_ext_pack(system_id, component_id, msg, autopilot_state_for_gimbal_device_ext->target_system, autopilot_state_for_gimbal_device_ext->target_component, autopilot_state_for_gimbal_device_ext->time_boot_us, autopilot_state_for_gimbal_device_ext->wind_x, autopilot_state_for_gimbal_device_ext->wind_y, autopilot_state_for_gimbal_device_ext->wind_correction_angle);
}

/**
 * @brief Encode a autopilot_state_for_gimbal_device_ext struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param autopilot_state_for_gimbal_device_ext C-struct to read the message contents from
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_autopilot_state_for_gimbal_device_ext_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_autopilot_state_for_gimbal_device_ext_t* autopilot_state_for_gimbal_device_ext)
{
    return mavlink_msg_autopilot_state_for_gimbal_device_ext_pack_chan(system_id, component_id, chan, msg, autopilot_state_for_gimbal_device_ext->target_system, autopilot_state_for_gimbal_device_ext->target_component, autopilot_state_for_gimbal_device_ext->time_boot_us, autopilot_state_for_gimbal_device_ext->wind_x, autopilot_state_for_gimbal_device_ext->wind_y, autopilot_state_for_gimbal_device_ext->wind_correction_angle);
}

/**
 * @brief Encode a autopilot_state_for_gimbal_device_ext struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param autopilot_state_for_gimbal_device_ext C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_autopilot_state_for_gimbal_device_ext_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_autopilot_state_for_gimbal_device_ext_t* autopilot_state_for_gimbal_device_ext)
{
    return mavlink_msg_autopilot_state_for_gimbal_device_ext_pack_status(system_id, component_id, _status, msg,  autopilot_state_for_gimbal_device_ext->target_system, autopilot_state_for_gimbal_device_ext->target_component, autopilot_state_for_gimbal_device_ext->time_boot_us, autopilot_state_for_gimbal_device_ext->wind_x, autopilot_state_for_gimbal_device_ext->wind_y, autopilot_state_for_gimbal_device_ext->wind_correction_angle);
}

/**
 * @brief Send a autopilot_state_for_gimbal_device_ext message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system  System ID.
 * @param target_component  Component ID.
 * @param time_boot_us [us] Timestamp (time since system boot).
 * @param wind_x [m/s] Wind X speed in NED (North,Est, Down). NAN if unknown.
 * @param wind_y [m/s] Wind Y speed in NED (North, East, Down). NAN if unknown.
 * @param wind_correction_angle [rad] Correction angle due to wind. NaN if unknown.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

MAVLINK_WIP
static inline void mavlink_msg_autopilot_state_for_gimbal_device_ext_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, uint64_t time_boot_us, float wind_x, float wind_y, float wind_correction_angle)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_LEN];
    _mav_put_uint64_t(buf, 0, time_boot_us);
    _mav_put_float(buf, 8, wind_x);
    _mav_put_float(buf, 12, wind_y);
    _mav_put_float(buf, 16, wind_correction_angle);
    _mav_put_uint8_t(buf, 20, target_system);
    _mav_put_uint8_t(buf, 21, target_component);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT, buf, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_MIN_LEN, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_LEN, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_CRC);
#else
    mavlink_autopilot_state_for_gimbal_device_ext_t packet;
    packet.time_boot_us = time_boot_us;
    packet.wind_x = wind_x;
    packet.wind_y = wind_y;
    packet.wind_correction_angle = wind_correction_angle;
    packet.target_system = target_system;
    packet.target_component = target_component;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT, (const char *)&packet, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_MIN_LEN, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_LEN, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_CRC);
#endif
}

/**
 * @brief Send a autopilot_state_for_gimbal_device_ext message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
MAVLINK_WIP
static inline void mavlink_msg_autopilot_state_for_gimbal_device_ext_send_struct(mavlink_channel_t chan, const mavlink_autopilot_state_for_gimbal_device_ext_t* autopilot_state_for_gimbal_device_ext)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_autopilot_state_for_gimbal_device_ext_send(chan, autopilot_state_for_gimbal_device_ext->target_system, autopilot_state_for_gimbal_device_ext->target_component, autopilot_state_for_gimbal_device_ext->time_boot_us, autopilot_state_for_gimbal_device_ext->wind_x, autopilot_state_for_gimbal_device_ext->wind_y, autopilot_state_for_gimbal_device_ext->wind_correction_angle);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT, (const char *)autopilot_state_for_gimbal_device_ext, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_MIN_LEN, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_LEN, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_CRC);
#endif
}

#if MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
MAVLINK_WIP
static inline void mavlink_msg_autopilot_state_for_gimbal_device_ext_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint8_t target_system, uint8_t target_component, uint64_t time_boot_us, float wind_x, float wind_y, float wind_correction_angle)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint64_t(buf, 0, time_boot_us);
    _mav_put_float(buf, 8, wind_x);
    _mav_put_float(buf, 12, wind_y);
    _mav_put_float(buf, 16, wind_correction_angle);
    _mav_put_uint8_t(buf, 20, target_system);
    _mav_put_uint8_t(buf, 21, target_component);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT, buf, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_MIN_LEN, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_LEN, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_CRC);
#else
    mavlink_autopilot_state_for_gimbal_device_ext_t *packet = (mavlink_autopilot_state_for_gimbal_device_ext_t *)msgbuf;
    packet->time_boot_us = time_boot_us;
    packet->wind_x = wind_x;
    packet->wind_y = wind_y;
    packet->wind_correction_angle = wind_correction_angle;
    packet->target_system = target_system;
    packet->target_component = target_component;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT, (const char *)packet, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_MIN_LEN, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_LEN, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_CRC);
#endif
}
#endif

#endif

// MESSAGE AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT UNPACKING


/**
 * @brief Get field target_system from autopilot_state_for_gimbal_device_ext message
 *
 * @return  System ID.
 */
MAVLINK_WIP
static inline uint8_t mavlink_msg_autopilot_state_for_gimbal_device_ext_get_target_system(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  20);
}

/**
 * @brief Get field target_component from autopilot_state_for_gimbal_device_ext message
 *
 * @return  Component ID.
 */
MAVLINK_WIP
static inline uint8_t mavlink_msg_autopilot_state_for_gimbal_device_ext_get_target_component(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  21);
}

/**
 * @brief Get field time_boot_us from autopilot_state_for_gimbal_device_ext message
 *
 * @return [us] Timestamp (time since system boot).
 */
MAVLINK_WIP
static inline uint64_t mavlink_msg_autopilot_state_for_gimbal_device_ext_get_time_boot_us(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint64_t(msg,  0);
}

/**
 * @brief Get field wind_x from autopilot_state_for_gimbal_device_ext message
 *
 * @return [m/s] Wind X speed in NED (North,Est, Down). NAN if unknown.
 */
MAVLINK_WIP
static inline float mavlink_msg_autopilot_state_for_gimbal_device_ext_get_wind_x(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  8);
}

/**
 * @brief Get field wind_y from autopilot_state_for_gimbal_device_ext message
 *
 * @return [m/s] Wind Y speed in NED (North, East, Down). NAN if unknown.
 */
MAVLINK_WIP
static inline float mavlink_msg_autopilot_state_for_gimbal_device_ext_get_wind_y(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  12);
}

/**
 * @brief Get field wind_correction_angle from autopilot_state_for_gimbal_device_ext message
 *
 * @return [rad] Correction angle due to wind. NaN if unknown.
 */
MAVLINK_WIP
static inline float mavlink_msg_autopilot_state_for_gimbal_device_ext_get_wind_correction_angle(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  16);
}

/**
 * @brief Decode a autopilot_state_for_gimbal_device_ext message into a struct
 *
 * @param msg The message to decode
 * @param autopilot_state_for_gimbal_device_ext C-struct to decode the message contents into
 */
MAVLINK_WIP
static inline void mavlink_msg_autopilot_state_for_gimbal_device_ext_decode(const mavlink_message_t* msg, mavlink_autopilot_state_for_gimbal_device_ext_t* autopilot_state_for_gimbal_device_ext)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    autopilot_state_for_gimbal_device_ext->time_boot_us = mavlink_msg_autopilot_state_for_gimbal_device_ext_get_time_boot_us(msg);
    autopilot_state_for_gimbal_device_ext->wind_x = mavlink_msg_autopilot_state_for_gimbal_device_ext_get_wind_x(msg);
    autopilot_state_for_gimbal_device_ext->wind_y = mavlink_msg_autopilot_state_for_gimbal_device_ext_get_wind_y(msg);
    autopilot_state_for_gimbal_device_ext->wind_correction_angle = mavlink_msg_autopilot_state_for_gimbal_device_ext_get_wind_correction_angle(msg);
    autopilot_state_for_gimbal_device_ext->target_system = mavlink_msg_autopilot_state_for_gimbal_device_ext_get_target_system(msg);
    autopilot_state_for_gimbal_device_ext->target_component = mavlink_msg_autopilot_state_for_gimbal_device_ext_get_target_component(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_LEN? msg->len : MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_LEN;
        memset(autopilot_state_for_gimbal_device_ext, 0, MAVLINK_MSG_ID_AUTOPILOT_STATE_FOR_GIMBAL_DEVICE_EXT_LEN);
    memcpy(autopilot_state_for_gimbal_device_ext, _MAV_PAYLOAD(msg), len);
#endif
}
