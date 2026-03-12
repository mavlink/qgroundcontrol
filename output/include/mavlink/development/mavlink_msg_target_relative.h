#pragma once
// MESSAGE TARGET_RELATIVE PACKING

#define MAVLINK_MSG_ID_TARGET_RELATIVE 511


typedef struct __mavlink_target_relative_t {
 uint64_t timestamp; /*< [us] Timestamp (UNIX epoch time)*/
 float x; /*< [m] X Position of the target in TARGET_OBS_FRAME*/
 float y; /*< [m] Y Position of the target in TARGET_OBS_FRAME*/
 float z; /*< [m] Z Position of the target in TARGET_OBS_FRAME*/
 float pos_std[3]; /*< [m] Standard deviation of the target's position in TARGET_OBS_FRAME*/
 float yaw_std; /*< [rad] Standard deviation of the target's orientation in TARGET_OBS_FRAME*/
 float q_target[4]; /*<  Quaternion of the target's orientation from the target's frame to the TARGET_OBS_FRAME (w, x, y, z order, zero-rotation is 1, 0, 0, 0)*/
 float q_sensor[4]; /*<  Quaternion of the sensor's orientation from TARGET_OBS_FRAME to vehicle-carried NED. (Ignored if set to (0,0,0,0)) (w, x, y, z order, zero-rotation is 1, 0, 0, 0)*/
 uint8_t id; /*<  The ID of the target if multiple targets are present*/
 uint8_t frame; /*<  Coordinate frame used for following fields.*/
 uint8_t type; /*<  Type of target*/
} mavlink_target_relative_t;

#define MAVLINK_MSG_ID_TARGET_RELATIVE_LEN 71
#define MAVLINK_MSG_ID_TARGET_RELATIVE_MIN_LEN 71
#define MAVLINK_MSG_ID_511_LEN 71
#define MAVLINK_MSG_ID_511_MIN_LEN 71

#define MAVLINK_MSG_ID_TARGET_RELATIVE_CRC 28
#define MAVLINK_MSG_ID_511_CRC 28

#define MAVLINK_MSG_TARGET_RELATIVE_FIELD_POS_STD_LEN 3
#define MAVLINK_MSG_TARGET_RELATIVE_FIELD_Q_TARGET_LEN 4
#define MAVLINK_MSG_TARGET_RELATIVE_FIELD_Q_SENSOR_LEN 4

#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_TARGET_RELATIVE { \
    511, \
    "TARGET_RELATIVE", \
    11, \
    {  { "timestamp", NULL, MAVLINK_TYPE_UINT64_T, 0, 0, offsetof(mavlink_target_relative_t, timestamp) }, \
         { "id", NULL, MAVLINK_TYPE_UINT8_T, 0, 68, offsetof(mavlink_target_relative_t, id) }, \
         { "frame", NULL, MAVLINK_TYPE_UINT8_T, 0, 69, offsetof(mavlink_target_relative_t, frame) }, \
         { "x", NULL, MAVLINK_TYPE_FLOAT, 0, 8, offsetof(mavlink_target_relative_t, x) }, \
         { "y", NULL, MAVLINK_TYPE_FLOAT, 0, 12, offsetof(mavlink_target_relative_t, y) }, \
         { "z", NULL, MAVLINK_TYPE_FLOAT, 0, 16, offsetof(mavlink_target_relative_t, z) }, \
         { "pos_std", NULL, MAVLINK_TYPE_FLOAT, 3, 20, offsetof(mavlink_target_relative_t, pos_std) }, \
         { "yaw_std", NULL, MAVLINK_TYPE_FLOAT, 0, 32, offsetof(mavlink_target_relative_t, yaw_std) }, \
         { "q_target", NULL, MAVLINK_TYPE_FLOAT, 4, 36, offsetof(mavlink_target_relative_t, q_target) }, \
         { "q_sensor", NULL, MAVLINK_TYPE_FLOAT, 4, 52, offsetof(mavlink_target_relative_t, q_sensor) }, \
         { "type", NULL, MAVLINK_TYPE_UINT8_T, 0, 70, offsetof(mavlink_target_relative_t, type) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_TARGET_RELATIVE { \
    "TARGET_RELATIVE", \
    11, \
    {  { "timestamp", NULL, MAVLINK_TYPE_UINT64_T, 0, 0, offsetof(mavlink_target_relative_t, timestamp) }, \
         { "id", NULL, MAVLINK_TYPE_UINT8_T, 0, 68, offsetof(mavlink_target_relative_t, id) }, \
         { "frame", NULL, MAVLINK_TYPE_UINT8_T, 0, 69, offsetof(mavlink_target_relative_t, frame) }, \
         { "x", NULL, MAVLINK_TYPE_FLOAT, 0, 8, offsetof(mavlink_target_relative_t, x) }, \
         { "y", NULL, MAVLINK_TYPE_FLOAT, 0, 12, offsetof(mavlink_target_relative_t, y) }, \
         { "z", NULL, MAVLINK_TYPE_FLOAT, 0, 16, offsetof(mavlink_target_relative_t, z) }, \
         { "pos_std", NULL, MAVLINK_TYPE_FLOAT, 3, 20, offsetof(mavlink_target_relative_t, pos_std) }, \
         { "yaw_std", NULL, MAVLINK_TYPE_FLOAT, 0, 32, offsetof(mavlink_target_relative_t, yaw_std) }, \
         { "q_target", NULL, MAVLINK_TYPE_FLOAT, 4, 36, offsetof(mavlink_target_relative_t, q_target) }, \
         { "q_sensor", NULL, MAVLINK_TYPE_FLOAT, 4, 52, offsetof(mavlink_target_relative_t, q_sensor) }, \
         { "type", NULL, MAVLINK_TYPE_UINT8_T, 0, 70, offsetof(mavlink_target_relative_t, type) }, \
         } \
}
#endif

/**
 * @brief Pack a target_relative message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param timestamp [us] Timestamp (UNIX epoch time)
 * @param id  The ID of the target if multiple targets are present
 * @param frame  Coordinate frame used for following fields.
 * @param x [m] X Position of the target in TARGET_OBS_FRAME
 * @param y [m] Y Position of the target in TARGET_OBS_FRAME
 * @param z [m] Z Position of the target in TARGET_OBS_FRAME
 * @param pos_std [m] Standard deviation of the target's position in TARGET_OBS_FRAME
 * @param yaw_std [rad] Standard deviation of the target's orientation in TARGET_OBS_FRAME
 * @param q_target  Quaternion of the target's orientation from the target's frame to the TARGET_OBS_FRAME (w, x, y, z order, zero-rotation is 1, 0, 0, 0)
 * @param q_sensor  Quaternion of the sensor's orientation from TARGET_OBS_FRAME to vehicle-carried NED. (Ignored if set to (0,0,0,0)) (w, x, y, z order, zero-rotation is 1, 0, 0, 0)
 * @param type  Type of target
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_target_relative_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint64_t timestamp, uint8_t id, uint8_t frame, float x, float y, float z, const float *pos_std, float yaw_std, const float *q_target, const float *q_sensor, uint8_t type)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_TARGET_RELATIVE_LEN];
    _mav_put_uint64_t(buf, 0, timestamp);
    _mav_put_float(buf, 8, x);
    _mav_put_float(buf, 12, y);
    _mav_put_float(buf, 16, z);
    _mav_put_float(buf, 32, yaw_std);
    _mav_put_uint8_t(buf, 68, id);
    _mav_put_uint8_t(buf, 69, frame);
    _mav_put_uint8_t(buf, 70, type);
    _mav_put_float_array(buf, 20, pos_std, 3);
    _mav_put_float_array(buf, 36, q_target, 4);
    _mav_put_float_array(buf, 52, q_sensor, 4);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_TARGET_RELATIVE_LEN);
#else
    mavlink_target_relative_t packet;
    packet.timestamp = timestamp;
    packet.x = x;
    packet.y = y;
    packet.z = z;
    packet.yaw_std = yaw_std;
    packet.id = id;
    packet.frame = frame;
    packet.type = type;
    mav_array_memcpy(packet.pos_std, pos_std, sizeof(float)*3);
    mav_array_memcpy(packet.q_target, q_target, sizeof(float)*4);
    mav_array_memcpy(packet.q_sensor, q_sensor, sizeof(float)*4);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_TARGET_RELATIVE_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_TARGET_RELATIVE;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_TARGET_RELATIVE_MIN_LEN, MAVLINK_MSG_ID_TARGET_RELATIVE_LEN, MAVLINK_MSG_ID_TARGET_RELATIVE_CRC);
}

/**
 * @brief Pack a target_relative message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param timestamp [us] Timestamp (UNIX epoch time)
 * @param id  The ID of the target if multiple targets are present
 * @param frame  Coordinate frame used for following fields.
 * @param x [m] X Position of the target in TARGET_OBS_FRAME
 * @param y [m] Y Position of the target in TARGET_OBS_FRAME
 * @param z [m] Z Position of the target in TARGET_OBS_FRAME
 * @param pos_std [m] Standard deviation of the target's position in TARGET_OBS_FRAME
 * @param yaw_std [rad] Standard deviation of the target's orientation in TARGET_OBS_FRAME
 * @param q_target  Quaternion of the target's orientation from the target's frame to the TARGET_OBS_FRAME (w, x, y, z order, zero-rotation is 1, 0, 0, 0)
 * @param q_sensor  Quaternion of the sensor's orientation from TARGET_OBS_FRAME to vehicle-carried NED. (Ignored if set to (0,0,0,0)) (w, x, y, z order, zero-rotation is 1, 0, 0, 0)
 * @param type  Type of target
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_target_relative_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint64_t timestamp, uint8_t id, uint8_t frame, float x, float y, float z, const float *pos_std, float yaw_std, const float *q_target, const float *q_sensor, uint8_t type)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_TARGET_RELATIVE_LEN];
    _mav_put_uint64_t(buf, 0, timestamp);
    _mav_put_float(buf, 8, x);
    _mav_put_float(buf, 12, y);
    _mav_put_float(buf, 16, z);
    _mav_put_float(buf, 32, yaw_std);
    _mav_put_uint8_t(buf, 68, id);
    _mav_put_uint8_t(buf, 69, frame);
    _mav_put_uint8_t(buf, 70, type);
    _mav_put_float_array(buf, 20, pos_std, 3);
    _mav_put_float_array(buf, 36, q_target, 4);
    _mav_put_float_array(buf, 52, q_sensor, 4);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_TARGET_RELATIVE_LEN);
#else
    mavlink_target_relative_t packet;
    packet.timestamp = timestamp;
    packet.x = x;
    packet.y = y;
    packet.z = z;
    packet.yaw_std = yaw_std;
    packet.id = id;
    packet.frame = frame;
    packet.type = type;
    mav_array_memcpy(packet.pos_std, pos_std, sizeof(float)*3);
    mav_array_memcpy(packet.q_target, q_target, sizeof(float)*4);
    mav_array_memcpy(packet.q_sensor, q_sensor, sizeof(float)*4);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_TARGET_RELATIVE_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_TARGET_RELATIVE;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_TARGET_RELATIVE_MIN_LEN, MAVLINK_MSG_ID_TARGET_RELATIVE_LEN, MAVLINK_MSG_ID_TARGET_RELATIVE_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_TARGET_RELATIVE_MIN_LEN, MAVLINK_MSG_ID_TARGET_RELATIVE_LEN);
#endif
}

/**
 * @brief Pack a target_relative message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param timestamp [us] Timestamp (UNIX epoch time)
 * @param id  The ID of the target if multiple targets are present
 * @param frame  Coordinate frame used for following fields.
 * @param x [m] X Position of the target in TARGET_OBS_FRAME
 * @param y [m] Y Position of the target in TARGET_OBS_FRAME
 * @param z [m] Z Position of the target in TARGET_OBS_FRAME
 * @param pos_std [m] Standard deviation of the target's position in TARGET_OBS_FRAME
 * @param yaw_std [rad] Standard deviation of the target's orientation in TARGET_OBS_FRAME
 * @param q_target  Quaternion of the target's orientation from the target's frame to the TARGET_OBS_FRAME (w, x, y, z order, zero-rotation is 1, 0, 0, 0)
 * @param q_sensor  Quaternion of the sensor's orientation from TARGET_OBS_FRAME to vehicle-carried NED. (Ignored if set to (0,0,0,0)) (w, x, y, z order, zero-rotation is 1, 0, 0, 0)
 * @param type  Type of target
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_target_relative_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint64_t timestamp,uint8_t id,uint8_t frame,float x,float y,float z,const float *pos_std,float yaw_std,const float *q_target,const float *q_sensor,uint8_t type)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_TARGET_RELATIVE_LEN];
    _mav_put_uint64_t(buf, 0, timestamp);
    _mav_put_float(buf, 8, x);
    _mav_put_float(buf, 12, y);
    _mav_put_float(buf, 16, z);
    _mav_put_float(buf, 32, yaw_std);
    _mav_put_uint8_t(buf, 68, id);
    _mav_put_uint8_t(buf, 69, frame);
    _mav_put_uint8_t(buf, 70, type);
    _mav_put_float_array(buf, 20, pos_std, 3);
    _mav_put_float_array(buf, 36, q_target, 4);
    _mav_put_float_array(buf, 52, q_sensor, 4);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_TARGET_RELATIVE_LEN);
#else
    mavlink_target_relative_t packet;
    packet.timestamp = timestamp;
    packet.x = x;
    packet.y = y;
    packet.z = z;
    packet.yaw_std = yaw_std;
    packet.id = id;
    packet.frame = frame;
    packet.type = type;
    mav_array_memcpy(packet.pos_std, pos_std, sizeof(float)*3);
    mav_array_memcpy(packet.q_target, q_target, sizeof(float)*4);
    mav_array_memcpy(packet.q_sensor, q_sensor, sizeof(float)*4);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_TARGET_RELATIVE_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_TARGET_RELATIVE;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_TARGET_RELATIVE_MIN_LEN, MAVLINK_MSG_ID_TARGET_RELATIVE_LEN, MAVLINK_MSG_ID_TARGET_RELATIVE_CRC);
}

/**
 * @brief Encode a target_relative struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param target_relative C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_target_relative_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_target_relative_t* target_relative)
{
    return mavlink_msg_target_relative_pack(system_id, component_id, msg, target_relative->timestamp, target_relative->id, target_relative->frame, target_relative->x, target_relative->y, target_relative->z, target_relative->pos_std, target_relative->yaw_std, target_relative->q_target, target_relative->q_sensor, target_relative->type);
}

/**
 * @brief Encode a target_relative struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_relative C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_target_relative_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_target_relative_t* target_relative)
{
    return mavlink_msg_target_relative_pack_chan(system_id, component_id, chan, msg, target_relative->timestamp, target_relative->id, target_relative->frame, target_relative->x, target_relative->y, target_relative->z, target_relative->pos_std, target_relative->yaw_std, target_relative->q_target, target_relative->q_sensor, target_relative->type);
}

/**
 * @brief Encode a target_relative struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param target_relative C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_target_relative_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_target_relative_t* target_relative)
{
    return mavlink_msg_target_relative_pack_status(system_id, component_id, _status, msg,  target_relative->timestamp, target_relative->id, target_relative->frame, target_relative->x, target_relative->y, target_relative->z, target_relative->pos_std, target_relative->yaw_std, target_relative->q_target, target_relative->q_sensor, target_relative->type);
}

/**
 * @brief Send a target_relative message
 * @param chan MAVLink channel to send the message
 *
 * @param timestamp [us] Timestamp (UNIX epoch time)
 * @param id  The ID of the target if multiple targets are present
 * @param frame  Coordinate frame used for following fields.
 * @param x [m] X Position of the target in TARGET_OBS_FRAME
 * @param y [m] Y Position of the target in TARGET_OBS_FRAME
 * @param z [m] Z Position of the target in TARGET_OBS_FRAME
 * @param pos_std [m] Standard deviation of the target's position in TARGET_OBS_FRAME
 * @param yaw_std [rad] Standard deviation of the target's orientation in TARGET_OBS_FRAME
 * @param q_target  Quaternion of the target's orientation from the target's frame to the TARGET_OBS_FRAME (w, x, y, z order, zero-rotation is 1, 0, 0, 0)
 * @param q_sensor  Quaternion of the sensor's orientation from TARGET_OBS_FRAME to vehicle-carried NED. (Ignored if set to (0,0,0,0)) (w, x, y, z order, zero-rotation is 1, 0, 0, 0)
 * @param type  Type of target
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_target_relative_send(mavlink_channel_t chan, uint64_t timestamp, uint8_t id, uint8_t frame, float x, float y, float z, const float *pos_std, float yaw_std, const float *q_target, const float *q_sensor, uint8_t type)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_TARGET_RELATIVE_LEN];
    _mav_put_uint64_t(buf, 0, timestamp);
    _mav_put_float(buf, 8, x);
    _mav_put_float(buf, 12, y);
    _mav_put_float(buf, 16, z);
    _mav_put_float(buf, 32, yaw_std);
    _mav_put_uint8_t(buf, 68, id);
    _mav_put_uint8_t(buf, 69, frame);
    _mav_put_uint8_t(buf, 70, type);
    _mav_put_float_array(buf, 20, pos_std, 3);
    _mav_put_float_array(buf, 36, q_target, 4);
    _mav_put_float_array(buf, 52, q_sensor, 4);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_TARGET_RELATIVE, buf, MAVLINK_MSG_ID_TARGET_RELATIVE_MIN_LEN, MAVLINK_MSG_ID_TARGET_RELATIVE_LEN, MAVLINK_MSG_ID_TARGET_RELATIVE_CRC);
#else
    mavlink_target_relative_t packet;
    packet.timestamp = timestamp;
    packet.x = x;
    packet.y = y;
    packet.z = z;
    packet.yaw_std = yaw_std;
    packet.id = id;
    packet.frame = frame;
    packet.type = type;
    mav_array_memcpy(packet.pos_std, pos_std, sizeof(float)*3);
    mav_array_memcpy(packet.q_target, q_target, sizeof(float)*4);
    mav_array_memcpy(packet.q_sensor, q_sensor, sizeof(float)*4);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_TARGET_RELATIVE, (const char *)&packet, MAVLINK_MSG_ID_TARGET_RELATIVE_MIN_LEN, MAVLINK_MSG_ID_TARGET_RELATIVE_LEN, MAVLINK_MSG_ID_TARGET_RELATIVE_CRC);
#endif
}

/**
 * @brief Send a target_relative message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_target_relative_send_struct(mavlink_channel_t chan, const mavlink_target_relative_t* target_relative)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_target_relative_send(chan, target_relative->timestamp, target_relative->id, target_relative->frame, target_relative->x, target_relative->y, target_relative->z, target_relative->pos_std, target_relative->yaw_std, target_relative->q_target, target_relative->q_sensor, target_relative->type);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_TARGET_RELATIVE, (const char *)target_relative, MAVLINK_MSG_ID_TARGET_RELATIVE_MIN_LEN, MAVLINK_MSG_ID_TARGET_RELATIVE_LEN, MAVLINK_MSG_ID_TARGET_RELATIVE_CRC);
#endif
}

#if MAVLINK_MSG_ID_TARGET_RELATIVE_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_target_relative_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint64_t timestamp, uint8_t id, uint8_t frame, float x, float y, float z, const float *pos_std, float yaw_std, const float *q_target, const float *q_sensor, uint8_t type)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint64_t(buf, 0, timestamp);
    _mav_put_float(buf, 8, x);
    _mav_put_float(buf, 12, y);
    _mav_put_float(buf, 16, z);
    _mav_put_float(buf, 32, yaw_std);
    _mav_put_uint8_t(buf, 68, id);
    _mav_put_uint8_t(buf, 69, frame);
    _mav_put_uint8_t(buf, 70, type);
    _mav_put_float_array(buf, 20, pos_std, 3);
    _mav_put_float_array(buf, 36, q_target, 4);
    _mav_put_float_array(buf, 52, q_sensor, 4);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_TARGET_RELATIVE, buf, MAVLINK_MSG_ID_TARGET_RELATIVE_MIN_LEN, MAVLINK_MSG_ID_TARGET_RELATIVE_LEN, MAVLINK_MSG_ID_TARGET_RELATIVE_CRC);
#else
    mavlink_target_relative_t *packet = (mavlink_target_relative_t *)msgbuf;
    packet->timestamp = timestamp;
    packet->x = x;
    packet->y = y;
    packet->z = z;
    packet->yaw_std = yaw_std;
    packet->id = id;
    packet->frame = frame;
    packet->type = type;
    mav_array_memcpy(packet->pos_std, pos_std, sizeof(float)*3);
    mav_array_memcpy(packet->q_target, q_target, sizeof(float)*4);
    mav_array_memcpy(packet->q_sensor, q_sensor, sizeof(float)*4);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_TARGET_RELATIVE, (const char *)packet, MAVLINK_MSG_ID_TARGET_RELATIVE_MIN_LEN, MAVLINK_MSG_ID_TARGET_RELATIVE_LEN, MAVLINK_MSG_ID_TARGET_RELATIVE_CRC);
#endif
}
#endif

#endif

// MESSAGE TARGET_RELATIVE UNPACKING


/**
 * @brief Get field timestamp from target_relative message
 *
 * @return [us] Timestamp (UNIX epoch time)
 */
static inline uint64_t mavlink_msg_target_relative_get_timestamp(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint64_t(msg,  0);
}

/**
 * @brief Get field id from target_relative message
 *
 * @return  The ID of the target if multiple targets are present
 */
static inline uint8_t mavlink_msg_target_relative_get_id(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  68);
}

/**
 * @brief Get field frame from target_relative message
 *
 * @return  Coordinate frame used for following fields.
 */
static inline uint8_t mavlink_msg_target_relative_get_frame(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  69);
}

/**
 * @brief Get field x from target_relative message
 *
 * @return [m] X Position of the target in TARGET_OBS_FRAME
 */
static inline float mavlink_msg_target_relative_get_x(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  8);
}

/**
 * @brief Get field y from target_relative message
 *
 * @return [m] Y Position of the target in TARGET_OBS_FRAME
 */
static inline float mavlink_msg_target_relative_get_y(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  12);
}

/**
 * @brief Get field z from target_relative message
 *
 * @return [m] Z Position of the target in TARGET_OBS_FRAME
 */
static inline float mavlink_msg_target_relative_get_z(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  16);
}

/**
 * @brief Get field pos_std from target_relative message
 *
 * @return [m] Standard deviation of the target's position in TARGET_OBS_FRAME
 */
static inline uint16_t mavlink_msg_target_relative_get_pos_std(const mavlink_message_t* msg, float *pos_std)
{
    return _MAV_RETURN_float_array(msg, pos_std, 3,  20);
}

/**
 * @brief Get field yaw_std from target_relative message
 *
 * @return [rad] Standard deviation of the target's orientation in TARGET_OBS_FRAME
 */
static inline float mavlink_msg_target_relative_get_yaw_std(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  32);
}

/**
 * @brief Get field q_target from target_relative message
 *
 * @return  Quaternion of the target's orientation from the target's frame to the TARGET_OBS_FRAME (w, x, y, z order, zero-rotation is 1, 0, 0, 0)
 */
static inline uint16_t mavlink_msg_target_relative_get_q_target(const mavlink_message_t* msg, float *q_target)
{
    return _MAV_RETURN_float_array(msg, q_target, 4,  36);
}

/**
 * @brief Get field q_sensor from target_relative message
 *
 * @return  Quaternion of the sensor's orientation from TARGET_OBS_FRAME to vehicle-carried NED. (Ignored if set to (0,0,0,0)) (w, x, y, z order, zero-rotation is 1, 0, 0, 0)
 */
static inline uint16_t mavlink_msg_target_relative_get_q_sensor(const mavlink_message_t* msg, float *q_sensor)
{
    return _MAV_RETURN_float_array(msg, q_sensor, 4,  52);
}

/**
 * @brief Get field type from target_relative message
 *
 * @return  Type of target
 */
static inline uint8_t mavlink_msg_target_relative_get_type(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  70);
}

/**
 * @brief Decode a target_relative message into a struct
 *
 * @param msg The message to decode
 * @param target_relative C-struct to decode the message contents into
 */
static inline void mavlink_msg_target_relative_decode(const mavlink_message_t* msg, mavlink_target_relative_t* target_relative)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    target_relative->timestamp = mavlink_msg_target_relative_get_timestamp(msg);
    target_relative->x = mavlink_msg_target_relative_get_x(msg);
    target_relative->y = mavlink_msg_target_relative_get_y(msg);
    target_relative->z = mavlink_msg_target_relative_get_z(msg);
    mavlink_msg_target_relative_get_pos_std(msg, target_relative->pos_std);
    target_relative->yaw_std = mavlink_msg_target_relative_get_yaw_std(msg);
    mavlink_msg_target_relative_get_q_target(msg, target_relative->q_target);
    mavlink_msg_target_relative_get_q_sensor(msg, target_relative->q_sensor);
    target_relative->id = mavlink_msg_target_relative_get_id(msg);
    target_relative->frame = mavlink_msg_target_relative_get_frame(msg);
    target_relative->type = mavlink_msg_target_relative_get_type(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_TARGET_RELATIVE_LEN? msg->len : MAVLINK_MSG_ID_TARGET_RELATIVE_LEN;
        memset(target_relative, 0, MAVLINK_MSG_ID_TARGET_RELATIVE_LEN);
    memcpy(target_relative, _MAV_PAYLOAD(msg), len);
#endif
}
