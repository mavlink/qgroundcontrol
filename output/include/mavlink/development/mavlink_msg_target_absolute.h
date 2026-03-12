#pragma once
// MESSAGE TARGET_ABSOLUTE PACKING

#define MAVLINK_MSG_ID_TARGET_ABSOLUTE 510


typedef struct __mavlink_target_absolute_t {
 uint64_t timestamp; /*< [us] Timestamp (UNIX epoch time).*/
 int32_t lat; /*< [degE7] Target's latitude (WGS84)*/
 int32_t lon; /*< [degE7] Target's longitude (WGS84)*/
 float alt; /*< [m] Target's altitude (AMSL)*/
 float vel[3]; /*< [m/s] Target's velocity in its body frame*/
 float acc[3]; /*< [m/s/s] Linear target's acceleration in its body frame*/
 float q_target[4]; /*<  Quaternion of the target's orientation from its body frame to the vehicle's NED frame.*/
 float rates[3]; /*< [rad/s] Target's roll, pitch and yaw rates*/
 float position_std[2]; /*< [m] Standard deviation of horizontal (eph) and vertical (epv) position errors*/
 float vel_std[3]; /*< [m/s] Standard deviation of the target's velocity in its body frame*/
 float acc_std[3]; /*< [m/s/s] Standard deviation of the target's acceleration in its body frame*/
 uint8_t id; /*<  The ID of the target if multiple targets are present*/
 uint8_t sensor_capabilities; /*<  Bitmap to indicate the sensor's reporting capabilities*/
} mavlink_target_absolute_t;

#define MAVLINK_MSG_ID_TARGET_ABSOLUTE_LEN 106
#define MAVLINK_MSG_ID_TARGET_ABSOLUTE_MIN_LEN 106
#define MAVLINK_MSG_ID_510_LEN 106
#define MAVLINK_MSG_ID_510_MIN_LEN 106

#define MAVLINK_MSG_ID_TARGET_ABSOLUTE_CRC 245
#define MAVLINK_MSG_ID_510_CRC 245

#define MAVLINK_MSG_TARGET_ABSOLUTE_FIELD_VEL_LEN 3
#define MAVLINK_MSG_TARGET_ABSOLUTE_FIELD_ACC_LEN 3
#define MAVLINK_MSG_TARGET_ABSOLUTE_FIELD_Q_TARGET_LEN 4
#define MAVLINK_MSG_TARGET_ABSOLUTE_FIELD_RATES_LEN 3
#define MAVLINK_MSG_TARGET_ABSOLUTE_FIELD_POSITION_STD_LEN 2
#define MAVLINK_MSG_TARGET_ABSOLUTE_FIELD_VEL_STD_LEN 3
#define MAVLINK_MSG_TARGET_ABSOLUTE_FIELD_ACC_STD_LEN 3

#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_TARGET_ABSOLUTE { \
    510, \
    "TARGET_ABSOLUTE", \
    13, \
    {  { "timestamp", NULL, MAVLINK_TYPE_UINT64_T, 0, 0, offsetof(mavlink_target_absolute_t, timestamp) }, \
         { "id", NULL, MAVLINK_TYPE_UINT8_T, 0, 104, offsetof(mavlink_target_absolute_t, id) }, \
         { "sensor_capabilities", NULL, MAVLINK_TYPE_UINT8_T, 0, 105, offsetof(mavlink_target_absolute_t, sensor_capabilities) }, \
         { "lat", NULL, MAVLINK_TYPE_INT32_T, 0, 8, offsetof(mavlink_target_absolute_t, lat) }, \
         { "lon", NULL, MAVLINK_TYPE_INT32_T, 0, 12, offsetof(mavlink_target_absolute_t, lon) }, \
         { "alt", NULL, MAVLINK_TYPE_FLOAT, 0, 16, offsetof(mavlink_target_absolute_t, alt) }, \
         { "vel", NULL, MAVLINK_TYPE_FLOAT, 3, 20, offsetof(mavlink_target_absolute_t, vel) }, \
         { "acc", NULL, MAVLINK_TYPE_FLOAT, 3, 32, offsetof(mavlink_target_absolute_t, acc) }, \
         { "q_target", NULL, MAVLINK_TYPE_FLOAT, 4, 44, offsetof(mavlink_target_absolute_t, q_target) }, \
         { "rates", NULL, MAVLINK_TYPE_FLOAT, 3, 60, offsetof(mavlink_target_absolute_t, rates) }, \
         { "position_std", NULL, MAVLINK_TYPE_FLOAT, 2, 72, offsetof(mavlink_target_absolute_t, position_std) }, \
         { "vel_std", NULL, MAVLINK_TYPE_FLOAT, 3, 80, offsetof(mavlink_target_absolute_t, vel_std) }, \
         { "acc_std", NULL, MAVLINK_TYPE_FLOAT, 3, 92, offsetof(mavlink_target_absolute_t, acc_std) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_TARGET_ABSOLUTE { \
    "TARGET_ABSOLUTE", \
    13, \
    {  { "timestamp", NULL, MAVLINK_TYPE_UINT64_T, 0, 0, offsetof(mavlink_target_absolute_t, timestamp) }, \
         { "id", NULL, MAVLINK_TYPE_UINT8_T, 0, 104, offsetof(mavlink_target_absolute_t, id) }, \
         { "sensor_capabilities", NULL, MAVLINK_TYPE_UINT8_T, 0, 105, offsetof(mavlink_target_absolute_t, sensor_capabilities) }, \
         { "lat", NULL, MAVLINK_TYPE_INT32_T, 0, 8, offsetof(mavlink_target_absolute_t, lat) }, \
         { "lon", NULL, MAVLINK_TYPE_INT32_T, 0, 12, offsetof(mavlink_target_absolute_t, lon) }, \
         { "alt", NULL, MAVLINK_TYPE_FLOAT, 0, 16, offsetof(mavlink_target_absolute_t, alt) }, \
         { "vel", NULL, MAVLINK_TYPE_FLOAT, 3, 20, offsetof(mavlink_target_absolute_t, vel) }, \
         { "acc", NULL, MAVLINK_TYPE_FLOAT, 3, 32, offsetof(mavlink_target_absolute_t, acc) }, \
         { "q_target", NULL, MAVLINK_TYPE_FLOAT, 4, 44, offsetof(mavlink_target_absolute_t, q_target) }, \
         { "rates", NULL, MAVLINK_TYPE_FLOAT, 3, 60, offsetof(mavlink_target_absolute_t, rates) }, \
         { "position_std", NULL, MAVLINK_TYPE_FLOAT, 2, 72, offsetof(mavlink_target_absolute_t, position_std) }, \
         { "vel_std", NULL, MAVLINK_TYPE_FLOAT, 3, 80, offsetof(mavlink_target_absolute_t, vel_std) }, \
         { "acc_std", NULL, MAVLINK_TYPE_FLOAT, 3, 92, offsetof(mavlink_target_absolute_t, acc_std) }, \
         } \
}
#endif

/**
 * @brief Pack a target_absolute message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param timestamp [us] Timestamp (UNIX epoch time).
 * @param id  The ID of the target if multiple targets are present
 * @param sensor_capabilities  Bitmap to indicate the sensor's reporting capabilities
 * @param lat [degE7] Target's latitude (WGS84)
 * @param lon [degE7] Target's longitude (WGS84)
 * @param alt [m] Target's altitude (AMSL)
 * @param vel [m/s] Target's velocity in its body frame
 * @param acc [m/s/s] Linear target's acceleration in its body frame
 * @param q_target  Quaternion of the target's orientation from its body frame to the vehicle's NED frame.
 * @param rates [rad/s] Target's roll, pitch and yaw rates
 * @param position_std [m] Standard deviation of horizontal (eph) and vertical (epv) position errors
 * @param vel_std [m/s] Standard deviation of the target's velocity in its body frame
 * @param acc_std [m/s/s] Standard deviation of the target's acceleration in its body frame
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_target_absolute_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint64_t timestamp, uint8_t id, uint8_t sensor_capabilities, int32_t lat, int32_t lon, float alt, const float *vel, const float *acc, const float *q_target, const float *rates, const float *position_std, const float *vel_std, const float *acc_std)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_TARGET_ABSOLUTE_LEN];
    _mav_put_uint64_t(buf, 0, timestamp);
    _mav_put_int32_t(buf, 8, lat);
    _mav_put_int32_t(buf, 12, lon);
    _mav_put_float(buf, 16, alt);
    _mav_put_uint8_t(buf, 104, id);
    _mav_put_uint8_t(buf, 105, sensor_capabilities);
    _mav_put_float_array(buf, 20, vel, 3);
    _mav_put_float_array(buf, 32, acc, 3);
    _mav_put_float_array(buf, 44, q_target, 4);
    _mav_put_float_array(buf, 60, rates, 3);
    _mav_put_float_array(buf, 72, position_std, 2);
    _mav_put_float_array(buf, 80, vel_std, 3);
    _mav_put_float_array(buf, 92, acc_std, 3);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_TARGET_ABSOLUTE_LEN);
#else
    mavlink_target_absolute_t packet;
    packet.timestamp = timestamp;
    packet.lat = lat;
    packet.lon = lon;
    packet.alt = alt;
    packet.id = id;
    packet.sensor_capabilities = sensor_capabilities;
    mav_array_memcpy(packet.vel, vel, sizeof(float)*3);
    mav_array_memcpy(packet.acc, acc, sizeof(float)*3);
    mav_array_memcpy(packet.q_target, q_target, sizeof(float)*4);
    mav_array_memcpy(packet.rates, rates, sizeof(float)*3);
    mav_array_memcpy(packet.position_std, position_std, sizeof(float)*2);
    mav_array_memcpy(packet.vel_std, vel_std, sizeof(float)*3);
    mav_array_memcpy(packet.acc_std, acc_std, sizeof(float)*3);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_TARGET_ABSOLUTE_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_TARGET_ABSOLUTE;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_TARGET_ABSOLUTE_MIN_LEN, MAVLINK_MSG_ID_TARGET_ABSOLUTE_LEN, MAVLINK_MSG_ID_TARGET_ABSOLUTE_CRC);
}

/**
 * @brief Pack a target_absolute message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param timestamp [us] Timestamp (UNIX epoch time).
 * @param id  The ID of the target if multiple targets are present
 * @param sensor_capabilities  Bitmap to indicate the sensor's reporting capabilities
 * @param lat [degE7] Target's latitude (WGS84)
 * @param lon [degE7] Target's longitude (WGS84)
 * @param alt [m] Target's altitude (AMSL)
 * @param vel [m/s] Target's velocity in its body frame
 * @param acc [m/s/s] Linear target's acceleration in its body frame
 * @param q_target  Quaternion of the target's orientation from its body frame to the vehicle's NED frame.
 * @param rates [rad/s] Target's roll, pitch and yaw rates
 * @param position_std [m] Standard deviation of horizontal (eph) and vertical (epv) position errors
 * @param vel_std [m/s] Standard deviation of the target's velocity in its body frame
 * @param acc_std [m/s/s] Standard deviation of the target's acceleration in its body frame
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_target_absolute_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint64_t timestamp, uint8_t id, uint8_t sensor_capabilities, int32_t lat, int32_t lon, float alt, const float *vel, const float *acc, const float *q_target, const float *rates, const float *position_std, const float *vel_std, const float *acc_std)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_TARGET_ABSOLUTE_LEN];
    _mav_put_uint64_t(buf, 0, timestamp);
    _mav_put_int32_t(buf, 8, lat);
    _mav_put_int32_t(buf, 12, lon);
    _mav_put_float(buf, 16, alt);
    _mav_put_uint8_t(buf, 104, id);
    _mav_put_uint8_t(buf, 105, sensor_capabilities);
    _mav_put_float_array(buf, 20, vel, 3);
    _mav_put_float_array(buf, 32, acc, 3);
    _mav_put_float_array(buf, 44, q_target, 4);
    _mav_put_float_array(buf, 60, rates, 3);
    _mav_put_float_array(buf, 72, position_std, 2);
    _mav_put_float_array(buf, 80, vel_std, 3);
    _mav_put_float_array(buf, 92, acc_std, 3);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_TARGET_ABSOLUTE_LEN);
#else
    mavlink_target_absolute_t packet;
    packet.timestamp = timestamp;
    packet.lat = lat;
    packet.lon = lon;
    packet.alt = alt;
    packet.id = id;
    packet.sensor_capabilities = sensor_capabilities;
    mav_array_memcpy(packet.vel, vel, sizeof(float)*3);
    mav_array_memcpy(packet.acc, acc, sizeof(float)*3);
    mav_array_memcpy(packet.q_target, q_target, sizeof(float)*4);
    mav_array_memcpy(packet.rates, rates, sizeof(float)*3);
    mav_array_memcpy(packet.position_std, position_std, sizeof(float)*2);
    mav_array_memcpy(packet.vel_std, vel_std, sizeof(float)*3);
    mav_array_memcpy(packet.acc_std, acc_std, sizeof(float)*3);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_TARGET_ABSOLUTE_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_TARGET_ABSOLUTE;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_TARGET_ABSOLUTE_MIN_LEN, MAVLINK_MSG_ID_TARGET_ABSOLUTE_LEN, MAVLINK_MSG_ID_TARGET_ABSOLUTE_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_TARGET_ABSOLUTE_MIN_LEN, MAVLINK_MSG_ID_TARGET_ABSOLUTE_LEN);
#endif
}

/**
 * @brief Pack a target_absolute message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param timestamp [us] Timestamp (UNIX epoch time).
 * @param id  The ID of the target if multiple targets are present
 * @param sensor_capabilities  Bitmap to indicate the sensor's reporting capabilities
 * @param lat [degE7] Target's latitude (WGS84)
 * @param lon [degE7] Target's longitude (WGS84)
 * @param alt [m] Target's altitude (AMSL)
 * @param vel [m/s] Target's velocity in its body frame
 * @param acc [m/s/s] Linear target's acceleration in its body frame
 * @param q_target  Quaternion of the target's orientation from its body frame to the vehicle's NED frame.
 * @param rates [rad/s] Target's roll, pitch and yaw rates
 * @param position_std [m] Standard deviation of horizontal (eph) and vertical (epv) position errors
 * @param vel_std [m/s] Standard deviation of the target's velocity in its body frame
 * @param acc_std [m/s/s] Standard deviation of the target's acceleration in its body frame
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_target_absolute_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint64_t timestamp,uint8_t id,uint8_t sensor_capabilities,int32_t lat,int32_t lon,float alt,const float *vel,const float *acc,const float *q_target,const float *rates,const float *position_std,const float *vel_std,const float *acc_std)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_TARGET_ABSOLUTE_LEN];
    _mav_put_uint64_t(buf, 0, timestamp);
    _mav_put_int32_t(buf, 8, lat);
    _mav_put_int32_t(buf, 12, lon);
    _mav_put_float(buf, 16, alt);
    _mav_put_uint8_t(buf, 104, id);
    _mav_put_uint8_t(buf, 105, sensor_capabilities);
    _mav_put_float_array(buf, 20, vel, 3);
    _mav_put_float_array(buf, 32, acc, 3);
    _mav_put_float_array(buf, 44, q_target, 4);
    _mav_put_float_array(buf, 60, rates, 3);
    _mav_put_float_array(buf, 72, position_std, 2);
    _mav_put_float_array(buf, 80, vel_std, 3);
    _mav_put_float_array(buf, 92, acc_std, 3);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_TARGET_ABSOLUTE_LEN);
#else
    mavlink_target_absolute_t packet;
    packet.timestamp = timestamp;
    packet.lat = lat;
    packet.lon = lon;
    packet.alt = alt;
    packet.id = id;
    packet.sensor_capabilities = sensor_capabilities;
    mav_array_memcpy(packet.vel, vel, sizeof(float)*3);
    mav_array_memcpy(packet.acc, acc, sizeof(float)*3);
    mav_array_memcpy(packet.q_target, q_target, sizeof(float)*4);
    mav_array_memcpy(packet.rates, rates, sizeof(float)*3);
    mav_array_memcpy(packet.position_std, position_std, sizeof(float)*2);
    mav_array_memcpy(packet.vel_std, vel_std, sizeof(float)*3);
    mav_array_memcpy(packet.acc_std, acc_std, sizeof(float)*3);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_TARGET_ABSOLUTE_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_TARGET_ABSOLUTE;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_TARGET_ABSOLUTE_MIN_LEN, MAVLINK_MSG_ID_TARGET_ABSOLUTE_LEN, MAVLINK_MSG_ID_TARGET_ABSOLUTE_CRC);
}

/**
 * @brief Encode a target_absolute struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param target_absolute C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_target_absolute_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_target_absolute_t* target_absolute)
{
    return mavlink_msg_target_absolute_pack(system_id, component_id, msg, target_absolute->timestamp, target_absolute->id, target_absolute->sensor_capabilities, target_absolute->lat, target_absolute->lon, target_absolute->alt, target_absolute->vel, target_absolute->acc, target_absolute->q_target, target_absolute->rates, target_absolute->position_std, target_absolute->vel_std, target_absolute->acc_std);
}

/**
 * @brief Encode a target_absolute struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_absolute C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_target_absolute_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_target_absolute_t* target_absolute)
{
    return mavlink_msg_target_absolute_pack_chan(system_id, component_id, chan, msg, target_absolute->timestamp, target_absolute->id, target_absolute->sensor_capabilities, target_absolute->lat, target_absolute->lon, target_absolute->alt, target_absolute->vel, target_absolute->acc, target_absolute->q_target, target_absolute->rates, target_absolute->position_std, target_absolute->vel_std, target_absolute->acc_std);
}

/**
 * @brief Encode a target_absolute struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param target_absolute C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_target_absolute_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_target_absolute_t* target_absolute)
{
    return mavlink_msg_target_absolute_pack_status(system_id, component_id, _status, msg,  target_absolute->timestamp, target_absolute->id, target_absolute->sensor_capabilities, target_absolute->lat, target_absolute->lon, target_absolute->alt, target_absolute->vel, target_absolute->acc, target_absolute->q_target, target_absolute->rates, target_absolute->position_std, target_absolute->vel_std, target_absolute->acc_std);
}

/**
 * @brief Send a target_absolute message
 * @param chan MAVLink channel to send the message
 *
 * @param timestamp [us] Timestamp (UNIX epoch time).
 * @param id  The ID of the target if multiple targets are present
 * @param sensor_capabilities  Bitmap to indicate the sensor's reporting capabilities
 * @param lat [degE7] Target's latitude (WGS84)
 * @param lon [degE7] Target's longitude (WGS84)
 * @param alt [m] Target's altitude (AMSL)
 * @param vel [m/s] Target's velocity in its body frame
 * @param acc [m/s/s] Linear target's acceleration in its body frame
 * @param q_target  Quaternion of the target's orientation from its body frame to the vehicle's NED frame.
 * @param rates [rad/s] Target's roll, pitch and yaw rates
 * @param position_std [m] Standard deviation of horizontal (eph) and vertical (epv) position errors
 * @param vel_std [m/s] Standard deviation of the target's velocity in its body frame
 * @param acc_std [m/s/s] Standard deviation of the target's acceleration in its body frame
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_target_absolute_send(mavlink_channel_t chan, uint64_t timestamp, uint8_t id, uint8_t sensor_capabilities, int32_t lat, int32_t lon, float alt, const float *vel, const float *acc, const float *q_target, const float *rates, const float *position_std, const float *vel_std, const float *acc_std)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_TARGET_ABSOLUTE_LEN];
    _mav_put_uint64_t(buf, 0, timestamp);
    _mav_put_int32_t(buf, 8, lat);
    _mav_put_int32_t(buf, 12, lon);
    _mav_put_float(buf, 16, alt);
    _mav_put_uint8_t(buf, 104, id);
    _mav_put_uint8_t(buf, 105, sensor_capabilities);
    _mav_put_float_array(buf, 20, vel, 3);
    _mav_put_float_array(buf, 32, acc, 3);
    _mav_put_float_array(buf, 44, q_target, 4);
    _mav_put_float_array(buf, 60, rates, 3);
    _mav_put_float_array(buf, 72, position_std, 2);
    _mav_put_float_array(buf, 80, vel_std, 3);
    _mav_put_float_array(buf, 92, acc_std, 3);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_TARGET_ABSOLUTE, buf, MAVLINK_MSG_ID_TARGET_ABSOLUTE_MIN_LEN, MAVLINK_MSG_ID_TARGET_ABSOLUTE_LEN, MAVLINK_MSG_ID_TARGET_ABSOLUTE_CRC);
#else
    mavlink_target_absolute_t packet;
    packet.timestamp = timestamp;
    packet.lat = lat;
    packet.lon = lon;
    packet.alt = alt;
    packet.id = id;
    packet.sensor_capabilities = sensor_capabilities;
    mav_array_memcpy(packet.vel, vel, sizeof(float)*3);
    mav_array_memcpy(packet.acc, acc, sizeof(float)*3);
    mav_array_memcpy(packet.q_target, q_target, sizeof(float)*4);
    mav_array_memcpy(packet.rates, rates, sizeof(float)*3);
    mav_array_memcpy(packet.position_std, position_std, sizeof(float)*2);
    mav_array_memcpy(packet.vel_std, vel_std, sizeof(float)*3);
    mav_array_memcpy(packet.acc_std, acc_std, sizeof(float)*3);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_TARGET_ABSOLUTE, (const char *)&packet, MAVLINK_MSG_ID_TARGET_ABSOLUTE_MIN_LEN, MAVLINK_MSG_ID_TARGET_ABSOLUTE_LEN, MAVLINK_MSG_ID_TARGET_ABSOLUTE_CRC);
#endif
}

/**
 * @brief Send a target_absolute message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_target_absolute_send_struct(mavlink_channel_t chan, const mavlink_target_absolute_t* target_absolute)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_target_absolute_send(chan, target_absolute->timestamp, target_absolute->id, target_absolute->sensor_capabilities, target_absolute->lat, target_absolute->lon, target_absolute->alt, target_absolute->vel, target_absolute->acc, target_absolute->q_target, target_absolute->rates, target_absolute->position_std, target_absolute->vel_std, target_absolute->acc_std);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_TARGET_ABSOLUTE, (const char *)target_absolute, MAVLINK_MSG_ID_TARGET_ABSOLUTE_MIN_LEN, MAVLINK_MSG_ID_TARGET_ABSOLUTE_LEN, MAVLINK_MSG_ID_TARGET_ABSOLUTE_CRC);
#endif
}

#if MAVLINK_MSG_ID_TARGET_ABSOLUTE_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_target_absolute_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint64_t timestamp, uint8_t id, uint8_t sensor_capabilities, int32_t lat, int32_t lon, float alt, const float *vel, const float *acc, const float *q_target, const float *rates, const float *position_std, const float *vel_std, const float *acc_std)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint64_t(buf, 0, timestamp);
    _mav_put_int32_t(buf, 8, lat);
    _mav_put_int32_t(buf, 12, lon);
    _mav_put_float(buf, 16, alt);
    _mav_put_uint8_t(buf, 104, id);
    _mav_put_uint8_t(buf, 105, sensor_capabilities);
    _mav_put_float_array(buf, 20, vel, 3);
    _mav_put_float_array(buf, 32, acc, 3);
    _mav_put_float_array(buf, 44, q_target, 4);
    _mav_put_float_array(buf, 60, rates, 3);
    _mav_put_float_array(buf, 72, position_std, 2);
    _mav_put_float_array(buf, 80, vel_std, 3);
    _mav_put_float_array(buf, 92, acc_std, 3);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_TARGET_ABSOLUTE, buf, MAVLINK_MSG_ID_TARGET_ABSOLUTE_MIN_LEN, MAVLINK_MSG_ID_TARGET_ABSOLUTE_LEN, MAVLINK_MSG_ID_TARGET_ABSOLUTE_CRC);
#else
    mavlink_target_absolute_t *packet = (mavlink_target_absolute_t *)msgbuf;
    packet->timestamp = timestamp;
    packet->lat = lat;
    packet->lon = lon;
    packet->alt = alt;
    packet->id = id;
    packet->sensor_capabilities = sensor_capabilities;
    mav_array_memcpy(packet->vel, vel, sizeof(float)*3);
    mav_array_memcpy(packet->acc, acc, sizeof(float)*3);
    mav_array_memcpy(packet->q_target, q_target, sizeof(float)*4);
    mav_array_memcpy(packet->rates, rates, sizeof(float)*3);
    mav_array_memcpy(packet->position_std, position_std, sizeof(float)*2);
    mav_array_memcpy(packet->vel_std, vel_std, sizeof(float)*3);
    mav_array_memcpy(packet->acc_std, acc_std, sizeof(float)*3);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_TARGET_ABSOLUTE, (const char *)packet, MAVLINK_MSG_ID_TARGET_ABSOLUTE_MIN_LEN, MAVLINK_MSG_ID_TARGET_ABSOLUTE_LEN, MAVLINK_MSG_ID_TARGET_ABSOLUTE_CRC);
#endif
}
#endif

#endif

// MESSAGE TARGET_ABSOLUTE UNPACKING


/**
 * @brief Get field timestamp from target_absolute message
 *
 * @return [us] Timestamp (UNIX epoch time).
 */
static inline uint64_t mavlink_msg_target_absolute_get_timestamp(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint64_t(msg,  0);
}

/**
 * @brief Get field id from target_absolute message
 *
 * @return  The ID of the target if multiple targets are present
 */
static inline uint8_t mavlink_msg_target_absolute_get_id(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  104);
}

/**
 * @brief Get field sensor_capabilities from target_absolute message
 *
 * @return  Bitmap to indicate the sensor's reporting capabilities
 */
static inline uint8_t mavlink_msg_target_absolute_get_sensor_capabilities(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  105);
}

/**
 * @brief Get field lat from target_absolute message
 *
 * @return [degE7] Target's latitude (WGS84)
 */
static inline int32_t mavlink_msg_target_absolute_get_lat(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int32_t(msg,  8);
}

/**
 * @brief Get field lon from target_absolute message
 *
 * @return [degE7] Target's longitude (WGS84)
 */
static inline int32_t mavlink_msg_target_absolute_get_lon(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int32_t(msg,  12);
}

/**
 * @brief Get field alt from target_absolute message
 *
 * @return [m] Target's altitude (AMSL)
 */
static inline float mavlink_msg_target_absolute_get_alt(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  16);
}

/**
 * @brief Get field vel from target_absolute message
 *
 * @return [m/s] Target's velocity in its body frame
 */
static inline uint16_t mavlink_msg_target_absolute_get_vel(const mavlink_message_t* msg, float *vel)
{
    return _MAV_RETURN_float_array(msg, vel, 3,  20);
}

/**
 * @brief Get field acc from target_absolute message
 *
 * @return [m/s/s] Linear target's acceleration in its body frame
 */
static inline uint16_t mavlink_msg_target_absolute_get_acc(const mavlink_message_t* msg, float *acc)
{
    return _MAV_RETURN_float_array(msg, acc, 3,  32);
}

/**
 * @brief Get field q_target from target_absolute message
 *
 * @return  Quaternion of the target's orientation from its body frame to the vehicle's NED frame.
 */
static inline uint16_t mavlink_msg_target_absolute_get_q_target(const mavlink_message_t* msg, float *q_target)
{
    return _MAV_RETURN_float_array(msg, q_target, 4,  44);
}

/**
 * @brief Get field rates from target_absolute message
 *
 * @return [rad/s] Target's roll, pitch and yaw rates
 */
static inline uint16_t mavlink_msg_target_absolute_get_rates(const mavlink_message_t* msg, float *rates)
{
    return _MAV_RETURN_float_array(msg, rates, 3,  60);
}

/**
 * @brief Get field position_std from target_absolute message
 *
 * @return [m] Standard deviation of horizontal (eph) and vertical (epv) position errors
 */
static inline uint16_t mavlink_msg_target_absolute_get_position_std(const mavlink_message_t* msg, float *position_std)
{
    return _MAV_RETURN_float_array(msg, position_std, 2,  72);
}

/**
 * @brief Get field vel_std from target_absolute message
 *
 * @return [m/s] Standard deviation of the target's velocity in its body frame
 */
static inline uint16_t mavlink_msg_target_absolute_get_vel_std(const mavlink_message_t* msg, float *vel_std)
{
    return _MAV_RETURN_float_array(msg, vel_std, 3,  80);
}

/**
 * @brief Get field acc_std from target_absolute message
 *
 * @return [m/s/s] Standard deviation of the target's acceleration in its body frame
 */
static inline uint16_t mavlink_msg_target_absolute_get_acc_std(const mavlink_message_t* msg, float *acc_std)
{
    return _MAV_RETURN_float_array(msg, acc_std, 3,  92);
}

/**
 * @brief Decode a target_absolute message into a struct
 *
 * @param msg The message to decode
 * @param target_absolute C-struct to decode the message contents into
 */
static inline void mavlink_msg_target_absolute_decode(const mavlink_message_t* msg, mavlink_target_absolute_t* target_absolute)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    target_absolute->timestamp = mavlink_msg_target_absolute_get_timestamp(msg);
    target_absolute->lat = mavlink_msg_target_absolute_get_lat(msg);
    target_absolute->lon = mavlink_msg_target_absolute_get_lon(msg);
    target_absolute->alt = mavlink_msg_target_absolute_get_alt(msg);
    mavlink_msg_target_absolute_get_vel(msg, target_absolute->vel);
    mavlink_msg_target_absolute_get_acc(msg, target_absolute->acc);
    mavlink_msg_target_absolute_get_q_target(msg, target_absolute->q_target);
    mavlink_msg_target_absolute_get_rates(msg, target_absolute->rates);
    mavlink_msg_target_absolute_get_position_std(msg, target_absolute->position_std);
    mavlink_msg_target_absolute_get_vel_std(msg, target_absolute->vel_std);
    mavlink_msg_target_absolute_get_acc_std(msg, target_absolute->acc_std);
    target_absolute->id = mavlink_msg_target_absolute_get_id(msg);
    target_absolute->sensor_capabilities = mavlink_msg_target_absolute_get_sensor_capabilities(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_TARGET_ABSOLUTE_LEN? msg->len : MAVLINK_MSG_ID_TARGET_ABSOLUTE_LEN;
        memset(target_absolute, 0, MAVLINK_MSG_ID_TARGET_ABSOLUTE_LEN);
    memcpy(target_absolute, _MAV_PAYLOAD(msg), len);
#endif
}
