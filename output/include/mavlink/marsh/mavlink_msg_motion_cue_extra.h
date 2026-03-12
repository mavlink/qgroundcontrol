#pragma once
// MESSAGE MOTION_CUE_EXTRA PACKING

#define MAVLINK_MSG_ID_MOTION_CUE_EXTRA 52504


typedef struct __mavlink_motion_cue_extra_t {
 uint32_t time_boot_ms; /*< [ms] Timestamp (time since system boot).*/
 float vel_roll; /*< [rad/s] Roll velocity, positive right.*/
 float vel_pitch; /*< [rad/s] Pitch velocity, positive nose up.*/
 float vel_yaw; /*< [rad/s] Yaw velocity, positive right.*/
 float acc_x; /*< [m/s/s] X axis (surge) acceleration, positive forward.*/
 float acc_y; /*< [m/s/s] Y axis (sway) acceleration, positive right.*/
 float acc_z; /*< [m/s/s] Z axis (heave) acceleration, positive down.*/
} mavlink_motion_cue_extra_t;

#define MAVLINK_MSG_ID_MOTION_CUE_EXTRA_LEN 28
#define MAVLINK_MSG_ID_MOTION_CUE_EXTRA_MIN_LEN 28
#define MAVLINK_MSG_ID_52504_LEN 28
#define MAVLINK_MSG_ID_52504_MIN_LEN 28

#define MAVLINK_MSG_ID_MOTION_CUE_EXTRA_CRC 177
#define MAVLINK_MSG_ID_52504_CRC 177



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_MOTION_CUE_EXTRA { \
    52504, \
    "MOTION_CUE_EXTRA", \
    7, \
    {  { "time_boot_ms", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_motion_cue_extra_t, time_boot_ms) }, \
         { "vel_roll", NULL, MAVLINK_TYPE_FLOAT, 0, 4, offsetof(mavlink_motion_cue_extra_t, vel_roll) }, \
         { "vel_pitch", NULL, MAVLINK_TYPE_FLOAT, 0, 8, offsetof(mavlink_motion_cue_extra_t, vel_pitch) }, \
         { "vel_yaw", NULL, MAVLINK_TYPE_FLOAT, 0, 12, offsetof(mavlink_motion_cue_extra_t, vel_yaw) }, \
         { "acc_x", NULL, MAVLINK_TYPE_FLOAT, 0, 16, offsetof(mavlink_motion_cue_extra_t, acc_x) }, \
         { "acc_y", NULL, MAVLINK_TYPE_FLOAT, 0, 20, offsetof(mavlink_motion_cue_extra_t, acc_y) }, \
         { "acc_z", NULL, MAVLINK_TYPE_FLOAT, 0, 24, offsetof(mavlink_motion_cue_extra_t, acc_z) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_MOTION_CUE_EXTRA { \
    "MOTION_CUE_EXTRA", \
    7, \
    {  { "time_boot_ms", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_motion_cue_extra_t, time_boot_ms) }, \
         { "vel_roll", NULL, MAVLINK_TYPE_FLOAT, 0, 4, offsetof(mavlink_motion_cue_extra_t, vel_roll) }, \
         { "vel_pitch", NULL, MAVLINK_TYPE_FLOAT, 0, 8, offsetof(mavlink_motion_cue_extra_t, vel_pitch) }, \
         { "vel_yaw", NULL, MAVLINK_TYPE_FLOAT, 0, 12, offsetof(mavlink_motion_cue_extra_t, vel_yaw) }, \
         { "acc_x", NULL, MAVLINK_TYPE_FLOAT, 0, 16, offsetof(mavlink_motion_cue_extra_t, acc_x) }, \
         { "acc_y", NULL, MAVLINK_TYPE_FLOAT, 0, 20, offsetof(mavlink_motion_cue_extra_t, acc_y) }, \
         { "acc_z", NULL, MAVLINK_TYPE_FLOAT, 0, 24, offsetof(mavlink_motion_cue_extra_t, acc_z) }, \
         } \
}
#endif

/**
 * @brief Pack a motion_cue_extra message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_boot_ms [ms] Timestamp (time since system boot).
 * @param vel_roll [rad/s] Roll velocity, positive right.
 * @param vel_pitch [rad/s] Pitch velocity, positive nose up.
 * @param vel_yaw [rad/s] Yaw velocity, positive right.
 * @param acc_x [m/s/s] X axis (surge) acceleration, positive forward.
 * @param acc_y [m/s/s] Y axis (sway) acceleration, positive right.
 * @param acc_z [m/s/s] Z axis (heave) acceleration, positive down.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_motion_cue_extra_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint32_t time_boot_ms, float vel_roll, float vel_pitch, float vel_yaw, float acc_x, float acc_y, float acc_z)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_MOTION_CUE_EXTRA_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_float(buf, 4, vel_roll);
    _mav_put_float(buf, 8, vel_pitch);
    _mav_put_float(buf, 12, vel_yaw);
    _mav_put_float(buf, 16, acc_x);
    _mav_put_float(buf, 20, acc_y);
    _mav_put_float(buf, 24, acc_z);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_MOTION_CUE_EXTRA_LEN);
#else
    mavlink_motion_cue_extra_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.vel_roll = vel_roll;
    packet.vel_pitch = vel_pitch;
    packet.vel_yaw = vel_yaw;
    packet.acc_x = acc_x;
    packet.acc_y = acc_y;
    packet.acc_z = acc_z;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_MOTION_CUE_EXTRA_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_MOTION_CUE_EXTRA;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_MOTION_CUE_EXTRA_MIN_LEN, MAVLINK_MSG_ID_MOTION_CUE_EXTRA_LEN, MAVLINK_MSG_ID_MOTION_CUE_EXTRA_CRC);
}

/**
 * @brief Pack a motion_cue_extra message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_boot_ms [ms] Timestamp (time since system boot).
 * @param vel_roll [rad/s] Roll velocity, positive right.
 * @param vel_pitch [rad/s] Pitch velocity, positive nose up.
 * @param vel_yaw [rad/s] Yaw velocity, positive right.
 * @param acc_x [m/s/s] X axis (surge) acceleration, positive forward.
 * @param acc_y [m/s/s] Y axis (sway) acceleration, positive right.
 * @param acc_z [m/s/s] Z axis (heave) acceleration, positive down.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_motion_cue_extra_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint32_t time_boot_ms, float vel_roll, float vel_pitch, float vel_yaw, float acc_x, float acc_y, float acc_z)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_MOTION_CUE_EXTRA_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_float(buf, 4, vel_roll);
    _mav_put_float(buf, 8, vel_pitch);
    _mav_put_float(buf, 12, vel_yaw);
    _mav_put_float(buf, 16, acc_x);
    _mav_put_float(buf, 20, acc_y);
    _mav_put_float(buf, 24, acc_z);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_MOTION_CUE_EXTRA_LEN);
#else
    mavlink_motion_cue_extra_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.vel_roll = vel_roll;
    packet.vel_pitch = vel_pitch;
    packet.vel_yaw = vel_yaw;
    packet.acc_x = acc_x;
    packet.acc_y = acc_y;
    packet.acc_z = acc_z;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_MOTION_CUE_EXTRA_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_MOTION_CUE_EXTRA;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_MOTION_CUE_EXTRA_MIN_LEN, MAVLINK_MSG_ID_MOTION_CUE_EXTRA_LEN, MAVLINK_MSG_ID_MOTION_CUE_EXTRA_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_MOTION_CUE_EXTRA_MIN_LEN, MAVLINK_MSG_ID_MOTION_CUE_EXTRA_LEN);
#endif
}

/**
 * @brief Pack a motion_cue_extra message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param time_boot_ms [ms] Timestamp (time since system boot).
 * @param vel_roll [rad/s] Roll velocity, positive right.
 * @param vel_pitch [rad/s] Pitch velocity, positive nose up.
 * @param vel_yaw [rad/s] Yaw velocity, positive right.
 * @param acc_x [m/s/s] X axis (surge) acceleration, positive forward.
 * @param acc_y [m/s/s] Y axis (sway) acceleration, positive right.
 * @param acc_z [m/s/s] Z axis (heave) acceleration, positive down.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_motion_cue_extra_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint32_t time_boot_ms,float vel_roll,float vel_pitch,float vel_yaw,float acc_x,float acc_y,float acc_z)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_MOTION_CUE_EXTRA_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_float(buf, 4, vel_roll);
    _mav_put_float(buf, 8, vel_pitch);
    _mav_put_float(buf, 12, vel_yaw);
    _mav_put_float(buf, 16, acc_x);
    _mav_put_float(buf, 20, acc_y);
    _mav_put_float(buf, 24, acc_z);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_MOTION_CUE_EXTRA_LEN);
#else
    mavlink_motion_cue_extra_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.vel_roll = vel_roll;
    packet.vel_pitch = vel_pitch;
    packet.vel_yaw = vel_yaw;
    packet.acc_x = acc_x;
    packet.acc_y = acc_y;
    packet.acc_z = acc_z;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_MOTION_CUE_EXTRA_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_MOTION_CUE_EXTRA;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_MOTION_CUE_EXTRA_MIN_LEN, MAVLINK_MSG_ID_MOTION_CUE_EXTRA_LEN, MAVLINK_MSG_ID_MOTION_CUE_EXTRA_CRC);
}

/**
 * @brief Encode a motion_cue_extra struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param motion_cue_extra C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_motion_cue_extra_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_motion_cue_extra_t* motion_cue_extra)
{
    return mavlink_msg_motion_cue_extra_pack(system_id, component_id, msg, motion_cue_extra->time_boot_ms, motion_cue_extra->vel_roll, motion_cue_extra->vel_pitch, motion_cue_extra->vel_yaw, motion_cue_extra->acc_x, motion_cue_extra->acc_y, motion_cue_extra->acc_z);
}

/**
 * @brief Encode a motion_cue_extra struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param motion_cue_extra C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_motion_cue_extra_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_motion_cue_extra_t* motion_cue_extra)
{
    return mavlink_msg_motion_cue_extra_pack_chan(system_id, component_id, chan, msg, motion_cue_extra->time_boot_ms, motion_cue_extra->vel_roll, motion_cue_extra->vel_pitch, motion_cue_extra->vel_yaw, motion_cue_extra->acc_x, motion_cue_extra->acc_y, motion_cue_extra->acc_z);
}

/**
 * @brief Encode a motion_cue_extra struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param motion_cue_extra C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_motion_cue_extra_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_motion_cue_extra_t* motion_cue_extra)
{
    return mavlink_msg_motion_cue_extra_pack_status(system_id, component_id, _status, msg,  motion_cue_extra->time_boot_ms, motion_cue_extra->vel_roll, motion_cue_extra->vel_pitch, motion_cue_extra->vel_yaw, motion_cue_extra->acc_x, motion_cue_extra->acc_y, motion_cue_extra->acc_z);
}

/**
 * @brief Send a motion_cue_extra message
 * @param chan MAVLink channel to send the message
 *
 * @param time_boot_ms [ms] Timestamp (time since system boot).
 * @param vel_roll [rad/s] Roll velocity, positive right.
 * @param vel_pitch [rad/s] Pitch velocity, positive nose up.
 * @param vel_yaw [rad/s] Yaw velocity, positive right.
 * @param acc_x [m/s/s] X axis (surge) acceleration, positive forward.
 * @param acc_y [m/s/s] Y axis (sway) acceleration, positive right.
 * @param acc_z [m/s/s] Z axis (heave) acceleration, positive down.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_motion_cue_extra_send(mavlink_channel_t chan, uint32_t time_boot_ms, float vel_roll, float vel_pitch, float vel_yaw, float acc_x, float acc_y, float acc_z)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_MOTION_CUE_EXTRA_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_float(buf, 4, vel_roll);
    _mav_put_float(buf, 8, vel_pitch);
    _mav_put_float(buf, 12, vel_yaw);
    _mav_put_float(buf, 16, acc_x);
    _mav_put_float(buf, 20, acc_y);
    _mav_put_float(buf, 24, acc_z);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MOTION_CUE_EXTRA, buf, MAVLINK_MSG_ID_MOTION_CUE_EXTRA_MIN_LEN, MAVLINK_MSG_ID_MOTION_CUE_EXTRA_LEN, MAVLINK_MSG_ID_MOTION_CUE_EXTRA_CRC);
#else
    mavlink_motion_cue_extra_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.vel_roll = vel_roll;
    packet.vel_pitch = vel_pitch;
    packet.vel_yaw = vel_yaw;
    packet.acc_x = acc_x;
    packet.acc_y = acc_y;
    packet.acc_z = acc_z;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MOTION_CUE_EXTRA, (const char *)&packet, MAVLINK_MSG_ID_MOTION_CUE_EXTRA_MIN_LEN, MAVLINK_MSG_ID_MOTION_CUE_EXTRA_LEN, MAVLINK_MSG_ID_MOTION_CUE_EXTRA_CRC);
#endif
}

/**
 * @brief Send a motion_cue_extra message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_motion_cue_extra_send_struct(mavlink_channel_t chan, const mavlink_motion_cue_extra_t* motion_cue_extra)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_motion_cue_extra_send(chan, motion_cue_extra->time_boot_ms, motion_cue_extra->vel_roll, motion_cue_extra->vel_pitch, motion_cue_extra->vel_yaw, motion_cue_extra->acc_x, motion_cue_extra->acc_y, motion_cue_extra->acc_z);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MOTION_CUE_EXTRA, (const char *)motion_cue_extra, MAVLINK_MSG_ID_MOTION_CUE_EXTRA_MIN_LEN, MAVLINK_MSG_ID_MOTION_CUE_EXTRA_LEN, MAVLINK_MSG_ID_MOTION_CUE_EXTRA_CRC);
#endif
}

#if MAVLINK_MSG_ID_MOTION_CUE_EXTRA_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_motion_cue_extra_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint32_t time_boot_ms, float vel_roll, float vel_pitch, float vel_yaw, float acc_x, float acc_y, float acc_z)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_float(buf, 4, vel_roll);
    _mav_put_float(buf, 8, vel_pitch);
    _mav_put_float(buf, 12, vel_yaw);
    _mav_put_float(buf, 16, acc_x);
    _mav_put_float(buf, 20, acc_y);
    _mav_put_float(buf, 24, acc_z);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MOTION_CUE_EXTRA, buf, MAVLINK_MSG_ID_MOTION_CUE_EXTRA_MIN_LEN, MAVLINK_MSG_ID_MOTION_CUE_EXTRA_LEN, MAVLINK_MSG_ID_MOTION_CUE_EXTRA_CRC);
#else
    mavlink_motion_cue_extra_t *packet = (mavlink_motion_cue_extra_t *)msgbuf;
    packet->time_boot_ms = time_boot_ms;
    packet->vel_roll = vel_roll;
    packet->vel_pitch = vel_pitch;
    packet->vel_yaw = vel_yaw;
    packet->acc_x = acc_x;
    packet->acc_y = acc_y;
    packet->acc_z = acc_z;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MOTION_CUE_EXTRA, (const char *)packet, MAVLINK_MSG_ID_MOTION_CUE_EXTRA_MIN_LEN, MAVLINK_MSG_ID_MOTION_CUE_EXTRA_LEN, MAVLINK_MSG_ID_MOTION_CUE_EXTRA_CRC);
#endif
}
#endif

#endif

// MESSAGE MOTION_CUE_EXTRA UNPACKING


/**
 * @brief Get field time_boot_ms from motion_cue_extra message
 *
 * @return [ms] Timestamp (time since system boot).
 */
static inline uint32_t mavlink_msg_motion_cue_extra_get_time_boot_ms(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  0);
}

/**
 * @brief Get field vel_roll from motion_cue_extra message
 *
 * @return [rad/s] Roll velocity, positive right.
 */
static inline float mavlink_msg_motion_cue_extra_get_vel_roll(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  4);
}

/**
 * @brief Get field vel_pitch from motion_cue_extra message
 *
 * @return [rad/s] Pitch velocity, positive nose up.
 */
static inline float mavlink_msg_motion_cue_extra_get_vel_pitch(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  8);
}

/**
 * @brief Get field vel_yaw from motion_cue_extra message
 *
 * @return [rad/s] Yaw velocity, positive right.
 */
static inline float mavlink_msg_motion_cue_extra_get_vel_yaw(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  12);
}

/**
 * @brief Get field acc_x from motion_cue_extra message
 *
 * @return [m/s/s] X axis (surge) acceleration, positive forward.
 */
static inline float mavlink_msg_motion_cue_extra_get_acc_x(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  16);
}

/**
 * @brief Get field acc_y from motion_cue_extra message
 *
 * @return [m/s/s] Y axis (sway) acceleration, positive right.
 */
static inline float mavlink_msg_motion_cue_extra_get_acc_y(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  20);
}

/**
 * @brief Get field acc_z from motion_cue_extra message
 *
 * @return [m/s/s] Z axis (heave) acceleration, positive down.
 */
static inline float mavlink_msg_motion_cue_extra_get_acc_z(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  24);
}

/**
 * @brief Decode a motion_cue_extra message into a struct
 *
 * @param msg The message to decode
 * @param motion_cue_extra C-struct to decode the message contents into
 */
static inline void mavlink_msg_motion_cue_extra_decode(const mavlink_message_t* msg, mavlink_motion_cue_extra_t* motion_cue_extra)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    motion_cue_extra->time_boot_ms = mavlink_msg_motion_cue_extra_get_time_boot_ms(msg);
    motion_cue_extra->vel_roll = mavlink_msg_motion_cue_extra_get_vel_roll(msg);
    motion_cue_extra->vel_pitch = mavlink_msg_motion_cue_extra_get_vel_pitch(msg);
    motion_cue_extra->vel_yaw = mavlink_msg_motion_cue_extra_get_vel_yaw(msg);
    motion_cue_extra->acc_x = mavlink_msg_motion_cue_extra_get_acc_x(msg);
    motion_cue_extra->acc_y = mavlink_msg_motion_cue_extra_get_acc_y(msg);
    motion_cue_extra->acc_z = mavlink_msg_motion_cue_extra_get_acc_z(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_MOTION_CUE_EXTRA_LEN? msg->len : MAVLINK_MSG_ID_MOTION_CUE_EXTRA_LEN;
        memset(motion_cue_extra, 0, MAVLINK_MSG_ID_MOTION_CUE_EXTRA_LEN);
    memcpy(motion_cue_extra, _MAV_PAYLOAD(msg), len);
#endif
}
