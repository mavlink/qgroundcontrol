#pragma once
// MESSAGE SET_VELOCITY_LIMITS PACKING

#define MAVLINK_MSG_ID_SET_VELOCITY_LIMITS 354


typedef struct __mavlink_set_velocity_limits_t {
 float horizontal_speed_limit; /*< [m/s] Limit for horizontal movement in MAV_FRAME_LOCAL_NED. NaN: Field not used (ignore)*/
 float vertical_speed_limit; /*< [m/s] Limit for vertical movement in MAV_FRAME_LOCAL_NED. NaN: Field not used (ignore)*/
 float yaw_rate_limit; /*< [rad/s] Limit for vehicle turn rate around its yaw axis. NaN: Field not used (ignore)*/
 uint8_t target_system; /*<  System ID (0 for broadcast).*/
 uint8_t target_component; /*<  Component ID (0 for broadcast).*/
} mavlink_set_velocity_limits_t;

#define MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_LEN 14
#define MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_MIN_LEN 14
#define MAVLINK_MSG_ID_354_LEN 14
#define MAVLINK_MSG_ID_354_MIN_LEN 14

#define MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_CRC 210
#define MAVLINK_MSG_ID_354_CRC 210



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_SET_VELOCITY_LIMITS { \
    354, \
    "SET_VELOCITY_LIMITS", \
    5, \
    {  { "target_system", NULL, MAVLINK_TYPE_UINT8_T, 0, 12, offsetof(mavlink_set_velocity_limits_t, target_system) }, \
         { "target_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 13, offsetof(mavlink_set_velocity_limits_t, target_component) }, \
         { "horizontal_speed_limit", NULL, MAVLINK_TYPE_FLOAT, 0, 0, offsetof(mavlink_set_velocity_limits_t, horizontal_speed_limit) }, \
         { "vertical_speed_limit", NULL, MAVLINK_TYPE_FLOAT, 0, 4, offsetof(mavlink_set_velocity_limits_t, vertical_speed_limit) }, \
         { "yaw_rate_limit", NULL, MAVLINK_TYPE_FLOAT, 0, 8, offsetof(mavlink_set_velocity_limits_t, yaw_rate_limit) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_SET_VELOCITY_LIMITS { \
    "SET_VELOCITY_LIMITS", \
    5, \
    {  { "target_system", NULL, MAVLINK_TYPE_UINT8_T, 0, 12, offsetof(mavlink_set_velocity_limits_t, target_system) }, \
         { "target_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 13, offsetof(mavlink_set_velocity_limits_t, target_component) }, \
         { "horizontal_speed_limit", NULL, MAVLINK_TYPE_FLOAT, 0, 0, offsetof(mavlink_set_velocity_limits_t, horizontal_speed_limit) }, \
         { "vertical_speed_limit", NULL, MAVLINK_TYPE_FLOAT, 0, 4, offsetof(mavlink_set_velocity_limits_t, vertical_speed_limit) }, \
         { "yaw_rate_limit", NULL, MAVLINK_TYPE_FLOAT, 0, 8, offsetof(mavlink_set_velocity_limits_t, yaw_rate_limit) }, \
         } \
}
#endif

/**
 * @brief Pack a set_velocity_limits message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system  System ID (0 for broadcast).
 * @param target_component  Component ID (0 for broadcast).
 * @param horizontal_speed_limit [m/s] Limit for horizontal movement in MAV_FRAME_LOCAL_NED. NaN: Field not used (ignore)
 * @param vertical_speed_limit [m/s] Limit for vertical movement in MAV_FRAME_LOCAL_NED. NaN: Field not used (ignore)
 * @param yaw_rate_limit [rad/s] Limit for vehicle turn rate around its yaw axis. NaN: Field not used (ignore)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_set_velocity_limits_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint8_t target_system, uint8_t target_component, float horizontal_speed_limit, float vertical_speed_limit, float yaw_rate_limit)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_LEN];
    _mav_put_float(buf, 0, horizontal_speed_limit);
    _mav_put_float(buf, 4, vertical_speed_limit);
    _mav_put_float(buf, 8, yaw_rate_limit);
    _mav_put_uint8_t(buf, 12, target_system);
    _mav_put_uint8_t(buf, 13, target_component);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_LEN);
#else
    mavlink_set_velocity_limits_t packet;
    packet.horizontal_speed_limit = horizontal_speed_limit;
    packet.vertical_speed_limit = vertical_speed_limit;
    packet.yaw_rate_limit = yaw_rate_limit;
    packet.target_system = target_system;
    packet.target_component = target_component;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_SET_VELOCITY_LIMITS;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_MIN_LEN, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_LEN, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_CRC);
}

/**
 * @brief Pack a set_velocity_limits message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system  System ID (0 for broadcast).
 * @param target_component  Component ID (0 for broadcast).
 * @param horizontal_speed_limit [m/s] Limit for horizontal movement in MAV_FRAME_LOCAL_NED. NaN: Field not used (ignore)
 * @param vertical_speed_limit [m/s] Limit for vertical movement in MAV_FRAME_LOCAL_NED. NaN: Field not used (ignore)
 * @param yaw_rate_limit [rad/s] Limit for vehicle turn rate around its yaw axis. NaN: Field not used (ignore)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_set_velocity_limits_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint8_t target_system, uint8_t target_component, float horizontal_speed_limit, float vertical_speed_limit, float yaw_rate_limit)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_LEN];
    _mav_put_float(buf, 0, horizontal_speed_limit);
    _mav_put_float(buf, 4, vertical_speed_limit);
    _mav_put_float(buf, 8, yaw_rate_limit);
    _mav_put_uint8_t(buf, 12, target_system);
    _mav_put_uint8_t(buf, 13, target_component);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_LEN);
#else
    mavlink_set_velocity_limits_t packet;
    packet.horizontal_speed_limit = horizontal_speed_limit;
    packet.vertical_speed_limit = vertical_speed_limit;
    packet.yaw_rate_limit = yaw_rate_limit;
    packet.target_system = target_system;
    packet.target_component = target_component;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_SET_VELOCITY_LIMITS;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_MIN_LEN, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_LEN, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_MIN_LEN, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_LEN);
#endif
}

/**
 * @brief Pack a set_velocity_limits message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system  System ID (0 for broadcast).
 * @param target_component  Component ID (0 for broadcast).
 * @param horizontal_speed_limit [m/s] Limit for horizontal movement in MAV_FRAME_LOCAL_NED. NaN: Field not used (ignore)
 * @param vertical_speed_limit [m/s] Limit for vertical movement in MAV_FRAME_LOCAL_NED. NaN: Field not used (ignore)
 * @param yaw_rate_limit [rad/s] Limit for vehicle turn rate around its yaw axis. NaN: Field not used (ignore)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_set_velocity_limits_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint8_t target_system,uint8_t target_component,float horizontal_speed_limit,float vertical_speed_limit,float yaw_rate_limit)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_LEN];
    _mav_put_float(buf, 0, horizontal_speed_limit);
    _mav_put_float(buf, 4, vertical_speed_limit);
    _mav_put_float(buf, 8, yaw_rate_limit);
    _mav_put_uint8_t(buf, 12, target_system);
    _mav_put_uint8_t(buf, 13, target_component);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_LEN);
#else
    mavlink_set_velocity_limits_t packet;
    packet.horizontal_speed_limit = horizontal_speed_limit;
    packet.vertical_speed_limit = vertical_speed_limit;
    packet.yaw_rate_limit = yaw_rate_limit;
    packet.target_system = target_system;
    packet.target_component = target_component;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_SET_VELOCITY_LIMITS;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_MIN_LEN, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_LEN, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_CRC);
}

/**
 * @brief Encode a set_velocity_limits struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param set_velocity_limits C-struct to read the message contents from
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_set_velocity_limits_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_set_velocity_limits_t* set_velocity_limits)
{
    return mavlink_msg_set_velocity_limits_pack(system_id, component_id, msg, set_velocity_limits->target_system, set_velocity_limits->target_component, set_velocity_limits->horizontal_speed_limit, set_velocity_limits->vertical_speed_limit, set_velocity_limits->yaw_rate_limit);
}

/**
 * @brief Encode a set_velocity_limits struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param set_velocity_limits C-struct to read the message contents from
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_set_velocity_limits_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_set_velocity_limits_t* set_velocity_limits)
{
    return mavlink_msg_set_velocity_limits_pack_chan(system_id, component_id, chan, msg, set_velocity_limits->target_system, set_velocity_limits->target_component, set_velocity_limits->horizontal_speed_limit, set_velocity_limits->vertical_speed_limit, set_velocity_limits->yaw_rate_limit);
}

/**
 * @brief Encode a set_velocity_limits struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param set_velocity_limits C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_set_velocity_limits_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_set_velocity_limits_t* set_velocity_limits)
{
    return mavlink_msg_set_velocity_limits_pack_status(system_id, component_id, _status, msg,  set_velocity_limits->target_system, set_velocity_limits->target_component, set_velocity_limits->horizontal_speed_limit, set_velocity_limits->vertical_speed_limit, set_velocity_limits->yaw_rate_limit);
}

/**
 * @brief Send a set_velocity_limits message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system  System ID (0 for broadcast).
 * @param target_component  Component ID (0 for broadcast).
 * @param horizontal_speed_limit [m/s] Limit for horizontal movement in MAV_FRAME_LOCAL_NED. NaN: Field not used (ignore)
 * @param vertical_speed_limit [m/s] Limit for vertical movement in MAV_FRAME_LOCAL_NED. NaN: Field not used (ignore)
 * @param yaw_rate_limit [rad/s] Limit for vehicle turn rate around its yaw axis. NaN: Field not used (ignore)
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

MAVLINK_WIP
static inline void mavlink_msg_set_velocity_limits_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, float horizontal_speed_limit, float vertical_speed_limit, float yaw_rate_limit)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_LEN];
    _mav_put_float(buf, 0, horizontal_speed_limit);
    _mav_put_float(buf, 4, vertical_speed_limit);
    _mav_put_float(buf, 8, yaw_rate_limit);
    _mav_put_uint8_t(buf, 12, target_system);
    _mav_put_uint8_t(buf, 13, target_component);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS, buf, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_MIN_LEN, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_LEN, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_CRC);
#else
    mavlink_set_velocity_limits_t packet;
    packet.horizontal_speed_limit = horizontal_speed_limit;
    packet.vertical_speed_limit = vertical_speed_limit;
    packet.yaw_rate_limit = yaw_rate_limit;
    packet.target_system = target_system;
    packet.target_component = target_component;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS, (const char *)&packet, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_MIN_LEN, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_LEN, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_CRC);
#endif
}

/**
 * @brief Send a set_velocity_limits message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
MAVLINK_WIP
static inline void mavlink_msg_set_velocity_limits_send_struct(mavlink_channel_t chan, const mavlink_set_velocity_limits_t* set_velocity_limits)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_set_velocity_limits_send(chan, set_velocity_limits->target_system, set_velocity_limits->target_component, set_velocity_limits->horizontal_speed_limit, set_velocity_limits->vertical_speed_limit, set_velocity_limits->yaw_rate_limit);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS, (const char *)set_velocity_limits, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_MIN_LEN, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_LEN, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_CRC);
#endif
}

#if MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
MAVLINK_WIP
static inline void mavlink_msg_set_velocity_limits_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint8_t target_system, uint8_t target_component, float horizontal_speed_limit, float vertical_speed_limit, float yaw_rate_limit)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_float(buf, 0, horizontal_speed_limit);
    _mav_put_float(buf, 4, vertical_speed_limit);
    _mav_put_float(buf, 8, yaw_rate_limit);
    _mav_put_uint8_t(buf, 12, target_system);
    _mav_put_uint8_t(buf, 13, target_component);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS, buf, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_MIN_LEN, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_LEN, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_CRC);
#else
    mavlink_set_velocity_limits_t *packet = (mavlink_set_velocity_limits_t *)msgbuf;
    packet->horizontal_speed_limit = horizontal_speed_limit;
    packet->vertical_speed_limit = vertical_speed_limit;
    packet->yaw_rate_limit = yaw_rate_limit;
    packet->target_system = target_system;
    packet->target_component = target_component;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS, (const char *)packet, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_MIN_LEN, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_LEN, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_CRC);
#endif
}
#endif

#endif

// MESSAGE SET_VELOCITY_LIMITS UNPACKING


/**
 * @brief Get field target_system from set_velocity_limits message
 *
 * @return  System ID (0 for broadcast).
 */
MAVLINK_WIP
static inline uint8_t mavlink_msg_set_velocity_limits_get_target_system(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  12);
}

/**
 * @brief Get field target_component from set_velocity_limits message
 *
 * @return  Component ID (0 for broadcast).
 */
MAVLINK_WIP
static inline uint8_t mavlink_msg_set_velocity_limits_get_target_component(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  13);
}

/**
 * @brief Get field horizontal_speed_limit from set_velocity_limits message
 *
 * @return [m/s] Limit for horizontal movement in MAV_FRAME_LOCAL_NED. NaN: Field not used (ignore)
 */
MAVLINK_WIP
static inline float mavlink_msg_set_velocity_limits_get_horizontal_speed_limit(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  0);
}

/**
 * @brief Get field vertical_speed_limit from set_velocity_limits message
 *
 * @return [m/s] Limit for vertical movement in MAV_FRAME_LOCAL_NED. NaN: Field not used (ignore)
 */
MAVLINK_WIP
static inline float mavlink_msg_set_velocity_limits_get_vertical_speed_limit(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  4);
}

/**
 * @brief Get field yaw_rate_limit from set_velocity_limits message
 *
 * @return [rad/s] Limit for vehicle turn rate around its yaw axis. NaN: Field not used (ignore)
 */
MAVLINK_WIP
static inline float mavlink_msg_set_velocity_limits_get_yaw_rate_limit(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  8);
}

/**
 * @brief Decode a set_velocity_limits message into a struct
 *
 * @param msg The message to decode
 * @param set_velocity_limits C-struct to decode the message contents into
 */
MAVLINK_WIP
static inline void mavlink_msg_set_velocity_limits_decode(const mavlink_message_t* msg, mavlink_set_velocity_limits_t* set_velocity_limits)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    set_velocity_limits->horizontal_speed_limit = mavlink_msg_set_velocity_limits_get_horizontal_speed_limit(msg);
    set_velocity_limits->vertical_speed_limit = mavlink_msg_set_velocity_limits_get_vertical_speed_limit(msg);
    set_velocity_limits->yaw_rate_limit = mavlink_msg_set_velocity_limits_get_yaw_rate_limit(msg);
    set_velocity_limits->target_system = mavlink_msg_set_velocity_limits_get_target_system(msg);
    set_velocity_limits->target_component = mavlink_msg_set_velocity_limits_get_target_component(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_LEN? msg->len : MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_LEN;
        memset(set_velocity_limits, 0, MAVLINK_MSG_ID_SET_VELOCITY_LIMITS_LEN);
    memcpy(set_velocity_limits, _MAV_PAYLOAD(msg), len);
#endif
}
