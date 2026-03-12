#pragma once
// MESSAGE CONTROL_LOADING_AXIS PACKING

#define MAVLINK_MSG_ID_CONTROL_LOADING_AXIS 52501


typedef struct __mavlink_control_loading_axis_t {
 uint32_t time_boot_ms; /*< [ms] Timestamp (time since system boot).*/
 float position; /*< [deg] Axis position*/
 float velocity; /*< [deg/s] Axis velocity*/
 float force; /*<  Force applied in the pilot in the direction of movement axis (not gripping force), measured at the position of pilot's third finger (ring). Unit N (Newton), currently not part of mavschema.xsd*/
 uint8_t axis; /*<  Control axis on which the measurements were taken.*/
} mavlink_control_loading_axis_t;

#define MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_LEN 17
#define MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_MIN_LEN 17
#define MAVLINK_MSG_ID_52501_LEN 17
#define MAVLINK_MSG_ID_52501_MIN_LEN 17

#define MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_CRC 240
#define MAVLINK_MSG_ID_52501_CRC 240



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_CONTROL_LOADING_AXIS { \
    52501, \
    "CONTROL_LOADING_AXIS", \
    5, \
    {  { "time_boot_ms", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_control_loading_axis_t, time_boot_ms) }, \
         { "axis", NULL, MAVLINK_TYPE_UINT8_T, 0, 16, offsetof(mavlink_control_loading_axis_t, axis) }, \
         { "position", NULL, MAVLINK_TYPE_FLOAT, 0, 4, offsetof(mavlink_control_loading_axis_t, position) }, \
         { "velocity", NULL, MAVLINK_TYPE_FLOAT, 0, 8, offsetof(mavlink_control_loading_axis_t, velocity) }, \
         { "force", NULL, MAVLINK_TYPE_FLOAT, 0, 12, offsetof(mavlink_control_loading_axis_t, force) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_CONTROL_LOADING_AXIS { \
    "CONTROL_LOADING_AXIS", \
    5, \
    {  { "time_boot_ms", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_control_loading_axis_t, time_boot_ms) }, \
         { "axis", NULL, MAVLINK_TYPE_UINT8_T, 0, 16, offsetof(mavlink_control_loading_axis_t, axis) }, \
         { "position", NULL, MAVLINK_TYPE_FLOAT, 0, 4, offsetof(mavlink_control_loading_axis_t, position) }, \
         { "velocity", NULL, MAVLINK_TYPE_FLOAT, 0, 8, offsetof(mavlink_control_loading_axis_t, velocity) }, \
         { "force", NULL, MAVLINK_TYPE_FLOAT, 0, 12, offsetof(mavlink_control_loading_axis_t, force) }, \
         } \
}
#endif

/**
 * @brief Pack a control_loading_axis message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_boot_ms [ms] Timestamp (time since system boot).
 * @param axis  Control axis on which the measurements were taken.
 * @param position [deg] Axis position
 * @param velocity [deg/s] Axis velocity
 * @param force  Force applied in the pilot in the direction of movement axis (not gripping force), measured at the position of pilot's third finger (ring). Unit N (Newton), currently not part of mavschema.xsd
 * @return length of the message in bytes (excluding serial stream start sign)
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_control_loading_axis_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint32_t time_boot_ms, uint8_t axis, float position, float velocity, float force)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_float(buf, 4, position);
    _mav_put_float(buf, 8, velocity);
    _mav_put_float(buf, 12, force);
    _mav_put_uint8_t(buf, 16, axis);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_LEN);
#else
    mavlink_control_loading_axis_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.position = position;
    packet.velocity = velocity;
    packet.force = force;
    packet.axis = axis;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_CONTROL_LOADING_AXIS;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_MIN_LEN, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_LEN, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_CRC);
}

/**
 * @brief Pack a control_loading_axis message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_boot_ms [ms] Timestamp (time since system boot).
 * @param axis  Control axis on which the measurements were taken.
 * @param position [deg] Axis position
 * @param velocity [deg/s] Axis velocity
 * @param force  Force applied in the pilot in the direction of movement axis (not gripping force), measured at the position of pilot's third finger (ring). Unit N (Newton), currently not part of mavschema.xsd
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_control_loading_axis_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint32_t time_boot_ms, uint8_t axis, float position, float velocity, float force)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_float(buf, 4, position);
    _mav_put_float(buf, 8, velocity);
    _mav_put_float(buf, 12, force);
    _mav_put_uint8_t(buf, 16, axis);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_LEN);
#else
    mavlink_control_loading_axis_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.position = position;
    packet.velocity = velocity;
    packet.force = force;
    packet.axis = axis;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_CONTROL_LOADING_AXIS;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_MIN_LEN, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_LEN, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_MIN_LEN, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_LEN);
#endif
}

/**
 * @brief Pack a control_loading_axis message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param time_boot_ms [ms] Timestamp (time since system boot).
 * @param axis  Control axis on which the measurements were taken.
 * @param position [deg] Axis position
 * @param velocity [deg/s] Axis velocity
 * @param force  Force applied in the pilot in the direction of movement axis (not gripping force), measured at the position of pilot's third finger (ring). Unit N (Newton), currently not part of mavschema.xsd
 * @return length of the message in bytes (excluding serial stream start sign)
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_control_loading_axis_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint32_t time_boot_ms,uint8_t axis,float position,float velocity,float force)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_float(buf, 4, position);
    _mav_put_float(buf, 8, velocity);
    _mav_put_float(buf, 12, force);
    _mav_put_uint8_t(buf, 16, axis);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_LEN);
#else
    mavlink_control_loading_axis_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.position = position;
    packet.velocity = velocity;
    packet.force = force;
    packet.axis = axis;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_CONTROL_LOADING_AXIS;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_MIN_LEN, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_LEN, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_CRC);
}

/**
 * @brief Encode a control_loading_axis struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param control_loading_axis C-struct to read the message contents from
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_control_loading_axis_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_control_loading_axis_t* control_loading_axis)
{
    return mavlink_msg_control_loading_axis_pack(system_id, component_id, msg, control_loading_axis->time_boot_ms, control_loading_axis->axis, control_loading_axis->position, control_loading_axis->velocity, control_loading_axis->force);
}

/**
 * @brief Encode a control_loading_axis struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param control_loading_axis C-struct to read the message contents from
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_control_loading_axis_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_control_loading_axis_t* control_loading_axis)
{
    return mavlink_msg_control_loading_axis_pack_chan(system_id, component_id, chan, msg, control_loading_axis->time_boot_ms, control_loading_axis->axis, control_loading_axis->position, control_loading_axis->velocity, control_loading_axis->force);
}

/**
 * @brief Encode a control_loading_axis struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param control_loading_axis C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_control_loading_axis_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_control_loading_axis_t* control_loading_axis)
{
    return mavlink_msg_control_loading_axis_pack_status(system_id, component_id, _status, msg,  control_loading_axis->time_boot_ms, control_loading_axis->axis, control_loading_axis->position, control_loading_axis->velocity, control_loading_axis->force);
}

/**
 * @brief Send a control_loading_axis message
 * @param chan MAVLink channel to send the message
 *
 * @param time_boot_ms [ms] Timestamp (time since system boot).
 * @param axis  Control axis on which the measurements were taken.
 * @param position [deg] Axis position
 * @param velocity [deg/s] Axis velocity
 * @param force  Force applied in the pilot in the direction of movement axis (not gripping force), measured at the position of pilot's third finger (ring). Unit N (Newton), currently not part of mavschema.xsd
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

MAVLINK_WIP
static inline void mavlink_msg_control_loading_axis_send(mavlink_channel_t chan, uint32_t time_boot_ms, uint8_t axis, float position, float velocity, float force)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_float(buf, 4, position);
    _mav_put_float(buf, 8, velocity);
    _mav_put_float(buf, 12, force);
    _mav_put_uint8_t(buf, 16, axis);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS, buf, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_MIN_LEN, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_LEN, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_CRC);
#else
    mavlink_control_loading_axis_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.position = position;
    packet.velocity = velocity;
    packet.force = force;
    packet.axis = axis;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS, (const char *)&packet, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_MIN_LEN, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_LEN, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_CRC);
#endif
}

/**
 * @brief Send a control_loading_axis message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
MAVLINK_WIP
static inline void mavlink_msg_control_loading_axis_send_struct(mavlink_channel_t chan, const mavlink_control_loading_axis_t* control_loading_axis)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_control_loading_axis_send(chan, control_loading_axis->time_boot_ms, control_loading_axis->axis, control_loading_axis->position, control_loading_axis->velocity, control_loading_axis->force);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS, (const char *)control_loading_axis, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_MIN_LEN, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_LEN, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_CRC);
#endif
}

#if MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
MAVLINK_WIP
static inline void mavlink_msg_control_loading_axis_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint32_t time_boot_ms, uint8_t axis, float position, float velocity, float force)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_float(buf, 4, position);
    _mav_put_float(buf, 8, velocity);
    _mav_put_float(buf, 12, force);
    _mav_put_uint8_t(buf, 16, axis);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS, buf, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_MIN_LEN, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_LEN, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_CRC);
#else
    mavlink_control_loading_axis_t *packet = (mavlink_control_loading_axis_t *)msgbuf;
    packet->time_boot_ms = time_boot_ms;
    packet->position = position;
    packet->velocity = velocity;
    packet->force = force;
    packet->axis = axis;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS, (const char *)packet, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_MIN_LEN, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_LEN, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_CRC);
#endif
}
#endif

#endif

// MESSAGE CONTROL_LOADING_AXIS UNPACKING


/**
 * @brief Get field time_boot_ms from control_loading_axis message
 *
 * @return [ms] Timestamp (time since system boot).
 */
MAVLINK_WIP
static inline uint32_t mavlink_msg_control_loading_axis_get_time_boot_ms(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  0);
}

/**
 * @brief Get field axis from control_loading_axis message
 *
 * @return  Control axis on which the measurements were taken.
 */
MAVLINK_WIP
static inline uint8_t mavlink_msg_control_loading_axis_get_axis(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  16);
}

/**
 * @brief Get field position from control_loading_axis message
 *
 * @return [deg] Axis position
 */
MAVLINK_WIP
static inline float mavlink_msg_control_loading_axis_get_position(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  4);
}

/**
 * @brief Get field velocity from control_loading_axis message
 *
 * @return [deg/s] Axis velocity
 */
MAVLINK_WIP
static inline float mavlink_msg_control_loading_axis_get_velocity(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  8);
}

/**
 * @brief Get field force from control_loading_axis message
 *
 * @return  Force applied in the pilot in the direction of movement axis (not gripping force), measured at the position of pilot's third finger (ring). Unit N (Newton), currently not part of mavschema.xsd
 */
MAVLINK_WIP
static inline float mavlink_msg_control_loading_axis_get_force(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  12);
}

/**
 * @brief Decode a control_loading_axis message into a struct
 *
 * @param msg The message to decode
 * @param control_loading_axis C-struct to decode the message contents into
 */
MAVLINK_WIP
static inline void mavlink_msg_control_loading_axis_decode(const mavlink_message_t* msg, mavlink_control_loading_axis_t* control_loading_axis)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    control_loading_axis->time_boot_ms = mavlink_msg_control_loading_axis_get_time_boot_ms(msg);
    control_loading_axis->position = mavlink_msg_control_loading_axis_get_position(msg);
    control_loading_axis->velocity = mavlink_msg_control_loading_axis_get_velocity(msg);
    control_loading_axis->force = mavlink_msg_control_loading_axis_get_force(msg);
    control_loading_axis->axis = mavlink_msg_control_loading_axis_get_axis(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_LEN? msg->len : MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_LEN;
        memset(control_loading_axis, 0, MAVLINK_MSG_ID_CONTROL_LOADING_AXIS_LEN);
    memcpy(control_loading_axis, _MAV_PAYLOAD(msg), len);
#endif
}
