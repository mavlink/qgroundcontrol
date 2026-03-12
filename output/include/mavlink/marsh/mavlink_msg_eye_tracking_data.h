#pragma once
// MESSAGE EYE_TRACKING_DATA PACKING

#define MAVLINK_MSG_ID_EYE_TRACKING_DATA 52505


typedef struct __mavlink_eye_tracking_data_t {
 uint64_t time_usec; /*< [us] Timestamp (time since system boot).*/
 float gaze_origin_x; /*< [m] X axis of gaze origin point, NaN if unknown. The reference system depends on specific application.*/
 float gaze_origin_y; /*< [m] Y axis of gaze origin point, NaN if unknown. The reference system depends on specific application.*/
 float gaze_origin_z; /*< [m] Z axis of gaze origin point, NaN if unknown. The reference system depends on specific application.*/
 float gaze_direction_x; /*<  X axis of gaze direction vector, expected to be normalized to unit magnitude, NaN if unknown. The reference system should match origin point.*/
 float gaze_direction_y; /*<  Y axis of gaze direction vector, expected to be normalized to unit magnitude, NaN if unknown. The reference system should match origin point.*/
 float gaze_direction_z; /*<  Z axis of gaze direction vector, expected to be normalized to unit magnitude, NaN if unknown. The reference system should match origin point.*/
 float video_gaze_x; /*<  Gaze focal point on video feed x value (normalized 0..1, 0 is left, 1 is right), NaN if unknown*/
 float video_gaze_y; /*<  Gaze focal point on video feed y value (normalized 0..1, 0 is top, 1 is bottom), NaN if unknown*/
 float surface_gaze_x; /*<  Gaze focal point on surface x value (normalized 0..1, 0 is left, 1 is right), NaN if unknown*/
 float surface_gaze_y; /*<  Gaze focal point on surface y value (normalized 0..1, 0 is top, 1 is bottom), NaN if unknown*/
 uint8_t sensor_id; /*<  Sensor ID, used for identifying the device and/or person tracked. Set to zero if unknown/unused.*/
 uint8_t surface_id; /*<  Identifier of surface for 2D gaze point, or an identified region when surface point is invalid. Set to zero if unknown/unused.*/
} mavlink_eye_tracking_data_t;

#define MAVLINK_MSG_ID_EYE_TRACKING_DATA_LEN 50
#define MAVLINK_MSG_ID_EYE_TRACKING_DATA_MIN_LEN 50
#define MAVLINK_MSG_ID_52505_LEN 50
#define MAVLINK_MSG_ID_52505_MIN_LEN 50

#define MAVLINK_MSG_ID_EYE_TRACKING_DATA_CRC 215
#define MAVLINK_MSG_ID_52505_CRC 215



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_EYE_TRACKING_DATA { \
    52505, \
    "EYE_TRACKING_DATA", \
    13, \
    {  { "time_usec", NULL, MAVLINK_TYPE_UINT64_T, 0, 0, offsetof(mavlink_eye_tracking_data_t, time_usec) }, \
         { "sensor_id", NULL, MAVLINK_TYPE_UINT8_T, 0, 48, offsetof(mavlink_eye_tracking_data_t, sensor_id) }, \
         { "gaze_origin_x", NULL, MAVLINK_TYPE_FLOAT, 0, 8, offsetof(mavlink_eye_tracking_data_t, gaze_origin_x) }, \
         { "gaze_origin_y", NULL, MAVLINK_TYPE_FLOAT, 0, 12, offsetof(mavlink_eye_tracking_data_t, gaze_origin_y) }, \
         { "gaze_origin_z", NULL, MAVLINK_TYPE_FLOAT, 0, 16, offsetof(mavlink_eye_tracking_data_t, gaze_origin_z) }, \
         { "gaze_direction_x", NULL, MAVLINK_TYPE_FLOAT, 0, 20, offsetof(mavlink_eye_tracking_data_t, gaze_direction_x) }, \
         { "gaze_direction_y", NULL, MAVLINK_TYPE_FLOAT, 0, 24, offsetof(mavlink_eye_tracking_data_t, gaze_direction_y) }, \
         { "gaze_direction_z", NULL, MAVLINK_TYPE_FLOAT, 0, 28, offsetof(mavlink_eye_tracking_data_t, gaze_direction_z) }, \
         { "video_gaze_x", NULL, MAVLINK_TYPE_FLOAT, 0, 32, offsetof(mavlink_eye_tracking_data_t, video_gaze_x) }, \
         { "video_gaze_y", NULL, MAVLINK_TYPE_FLOAT, 0, 36, offsetof(mavlink_eye_tracking_data_t, video_gaze_y) }, \
         { "surface_id", NULL, MAVLINK_TYPE_UINT8_T, 0, 49, offsetof(mavlink_eye_tracking_data_t, surface_id) }, \
         { "surface_gaze_x", NULL, MAVLINK_TYPE_FLOAT, 0, 40, offsetof(mavlink_eye_tracking_data_t, surface_gaze_x) }, \
         { "surface_gaze_y", NULL, MAVLINK_TYPE_FLOAT, 0, 44, offsetof(mavlink_eye_tracking_data_t, surface_gaze_y) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_EYE_TRACKING_DATA { \
    "EYE_TRACKING_DATA", \
    13, \
    {  { "time_usec", NULL, MAVLINK_TYPE_UINT64_T, 0, 0, offsetof(mavlink_eye_tracking_data_t, time_usec) }, \
         { "sensor_id", NULL, MAVLINK_TYPE_UINT8_T, 0, 48, offsetof(mavlink_eye_tracking_data_t, sensor_id) }, \
         { "gaze_origin_x", NULL, MAVLINK_TYPE_FLOAT, 0, 8, offsetof(mavlink_eye_tracking_data_t, gaze_origin_x) }, \
         { "gaze_origin_y", NULL, MAVLINK_TYPE_FLOAT, 0, 12, offsetof(mavlink_eye_tracking_data_t, gaze_origin_y) }, \
         { "gaze_origin_z", NULL, MAVLINK_TYPE_FLOAT, 0, 16, offsetof(mavlink_eye_tracking_data_t, gaze_origin_z) }, \
         { "gaze_direction_x", NULL, MAVLINK_TYPE_FLOAT, 0, 20, offsetof(mavlink_eye_tracking_data_t, gaze_direction_x) }, \
         { "gaze_direction_y", NULL, MAVLINK_TYPE_FLOAT, 0, 24, offsetof(mavlink_eye_tracking_data_t, gaze_direction_y) }, \
         { "gaze_direction_z", NULL, MAVLINK_TYPE_FLOAT, 0, 28, offsetof(mavlink_eye_tracking_data_t, gaze_direction_z) }, \
         { "video_gaze_x", NULL, MAVLINK_TYPE_FLOAT, 0, 32, offsetof(mavlink_eye_tracking_data_t, video_gaze_x) }, \
         { "video_gaze_y", NULL, MAVLINK_TYPE_FLOAT, 0, 36, offsetof(mavlink_eye_tracking_data_t, video_gaze_y) }, \
         { "surface_id", NULL, MAVLINK_TYPE_UINT8_T, 0, 49, offsetof(mavlink_eye_tracking_data_t, surface_id) }, \
         { "surface_gaze_x", NULL, MAVLINK_TYPE_FLOAT, 0, 40, offsetof(mavlink_eye_tracking_data_t, surface_gaze_x) }, \
         { "surface_gaze_y", NULL, MAVLINK_TYPE_FLOAT, 0, 44, offsetof(mavlink_eye_tracking_data_t, surface_gaze_y) }, \
         } \
}
#endif

/**
 * @brief Pack a eye_tracking_data message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_usec [us] Timestamp (time since system boot).
 * @param sensor_id  Sensor ID, used for identifying the device and/or person tracked. Set to zero if unknown/unused.
 * @param gaze_origin_x [m] X axis of gaze origin point, NaN if unknown. The reference system depends on specific application.
 * @param gaze_origin_y [m] Y axis of gaze origin point, NaN if unknown. The reference system depends on specific application.
 * @param gaze_origin_z [m] Z axis of gaze origin point, NaN if unknown. The reference system depends on specific application.
 * @param gaze_direction_x  X axis of gaze direction vector, expected to be normalized to unit magnitude, NaN if unknown. The reference system should match origin point.
 * @param gaze_direction_y  Y axis of gaze direction vector, expected to be normalized to unit magnitude, NaN if unknown. The reference system should match origin point.
 * @param gaze_direction_z  Z axis of gaze direction vector, expected to be normalized to unit magnitude, NaN if unknown. The reference system should match origin point.
 * @param video_gaze_x  Gaze focal point on video feed x value (normalized 0..1, 0 is left, 1 is right), NaN if unknown
 * @param video_gaze_y  Gaze focal point on video feed y value (normalized 0..1, 0 is top, 1 is bottom), NaN if unknown
 * @param surface_id  Identifier of surface for 2D gaze point, or an identified region when surface point is invalid. Set to zero if unknown/unused.
 * @param surface_gaze_x  Gaze focal point on surface x value (normalized 0..1, 0 is left, 1 is right), NaN if unknown
 * @param surface_gaze_y  Gaze focal point on surface y value (normalized 0..1, 0 is top, 1 is bottom), NaN if unknown
 * @return length of the message in bytes (excluding serial stream start sign)
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_eye_tracking_data_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint64_t time_usec, uint8_t sensor_id, float gaze_origin_x, float gaze_origin_y, float gaze_origin_z, float gaze_direction_x, float gaze_direction_y, float gaze_direction_z, float video_gaze_x, float video_gaze_y, uint8_t surface_id, float surface_gaze_x, float surface_gaze_y)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_EYE_TRACKING_DATA_LEN];
    _mav_put_uint64_t(buf, 0, time_usec);
    _mav_put_float(buf, 8, gaze_origin_x);
    _mav_put_float(buf, 12, gaze_origin_y);
    _mav_put_float(buf, 16, gaze_origin_z);
    _mav_put_float(buf, 20, gaze_direction_x);
    _mav_put_float(buf, 24, gaze_direction_y);
    _mav_put_float(buf, 28, gaze_direction_z);
    _mav_put_float(buf, 32, video_gaze_x);
    _mav_put_float(buf, 36, video_gaze_y);
    _mav_put_float(buf, 40, surface_gaze_x);
    _mav_put_float(buf, 44, surface_gaze_y);
    _mav_put_uint8_t(buf, 48, sensor_id);
    _mav_put_uint8_t(buf, 49, surface_id);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_EYE_TRACKING_DATA_LEN);
#else
    mavlink_eye_tracking_data_t packet;
    packet.time_usec = time_usec;
    packet.gaze_origin_x = gaze_origin_x;
    packet.gaze_origin_y = gaze_origin_y;
    packet.gaze_origin_z = gaze_origin_z;
    packet.gaze_direction_x = gaze_direction_x;
    packet.gaze_direction_y = gaze_direction_y;
    packet.gaze_direction_z = gaze_direction_z;
    packet.video_gaze_x = video_gaze_x;
    packet.video_gaze_y = video_gaze_y;
    packet.surface_gaze_x = surface_gaze_x;
    packet.surface_gaze_y = surface_gaze_y;
    packet.sensor_id = sensor_id;
    packet.surface_id = surface_id;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_EYE_TRACKING_DATA_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_EYE_TRACKING_DATA;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_EYE_TRACKING_DATA_MIN_LEN, MAVLINK_MSG_ID_EYE_TRACKING_DATA_LEN, MAVLINK_MSG_ID_EYE_TRACKING_DATA_CRC);
}

/**
 * @brief Pack a eye_tracking_data message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_usec [us] Timestamp (time since system boot).
 * @param sensor_id  Sensor ID, used for identifying the device and/or person tracked. Set to zero if unknown/unused.
 * @param gaze_origin_x [m] X axis of gaze origin point, NaN if unknown. The reference system depends on specific application.
 * @param gaze_origin_y [m] Y axis of gaze origin point, NaN if unknown. The reference system depends on specific application.
 * @param gaze_origin_z [m] Z axis of gaze origin point, NaN if unknown. The reference system depends on specific application.
 * @param gaze_direction_x  X axis of gaze direction vector, expected to be normalized to unit magnitude, NaN if unknown. The reference system should match origin point.
 * @param gaze_direction_y  Y axis of gaze direction vector, expected to be normalized to unit magnitude, NaN if unknown. The reference system should match origin point.
 * @param gaze_direction_z  Z axis of gaze direction vector, expected to be normalized to unit magnitude, NaN if unknown. The reference system should match origin point.
 * @param video_gaze_x  Gaze focal point on video feed x value (normalized 0..1, 0 is left, 1 is right), NaN if unknown
 * @param video_gaze_y  Gaze focal point on video feed y value (normalized 0..1, 0 is top, 1 is bottom), NaN if unknown
 * @param surface_id  Identifier of surface for 2D gaze point, or an identified region when surface point is invalid. Set to zero if unknown/unused.
 * @param surface_gaze_x  Gaze focal point on surface x value (normalized 0..1, 0 is left, 1 is right), NaN if unknown
 * @param surface_gaze_y  Gaze focal point on surface y value (normalized 0..1, 0 is top, 1 is bottom), NaN if unknown
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_eye_tracking_data_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint64_t time_usec, uint8_t sensor_id, float gaze_origin_x, float gaze_origin_y, float gaze_origin_z, float gaze_direction_x, float gaze_direction_y, float gaze_direction_z, float video_gaze_x, float video_gaze_y, uint8_t surface_id, float surface_gaze_x, float surface_gaze_y)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_EYE_TRACKING_DATA_LEN];
    _mav_put_uint64_t(buf, 0, time_usec);
    _mav_put_float(buf, 8, gaze_origin_x);
    _mav_put_float(buf, 12, gaze_origin_y);
    _mav_put_float(buf, 16, gaze_origin_z);
    _mav_put_float(buf, 20, gaze_direction_x);
    _mav_put_float(buf, 24, gaze_direction_y);
    _mav_put_float(buf, 28, gaze_direction_z);
    _mav_put_float(buf, 32, video_gaze_x);
    _mav_put_float(buf, 36, video_gaze_y);
    _mav_put_float(buf, 40, surface_gaze_x);
    _mav_put_float(buf, 44, surface_gaze_y);
    _mav_put_uint8_t(buf, 48, sensor_id);
    _mav_put_uint8_t(buf, 49, surface_id);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_EYE_TRACKING_DATA_LEN);
#else
    mavlink_eye_tracking_data_t packet;
    packet.time_usec = time_usec;
    packet.gaze_origin_x = gaze_origin_x;
    packet.gaze_origin_y = gaze_origin_y;
    packet.gaze_origin_z = gaze_origin_z;
    packet.gaze_direction_x = gaze_direction_x;
    packet.gaze_direction_y = gaze_direction_y;
    packet.gaze_direction_z = gaze_direction_z;
    packet.video_gaze_x = video_gaze_x;
    packet.video_gaze_y = video_gaze_y;
    packet.surface_gaze_x = surface_gaze_x;
    packet.surface_gaze_y = surface_gaze_y;
    packet.sensor_id = sensor_id;
    packet.surface_id = surface_id;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_EYE_TRACKING_DATA_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_EYE_TRACKING_DATA;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_EYE_TRACKING_DATA_MIN_LEN, MAVLINK_MSG_ID_EYE_TRACKING_DATA_LEN, MAVLINK_MSG_ID_EYE_TRACKING_DATA_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_EYE_TRACKING_DATA_MIN_LEN, MAVLINK_MSG_ID_EYE_TRACKING_DATA_LEN);
#endif
}

/**
 * @brief Pack a eye_tracking_data message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param time_usec [us] Timestamp (time since system boot).
 * @param sensor_id  Sensor ID, used for identifying the device and/or person tracked. Set to zero if unknown/unused.
 * @param gaze_origin_x [m] X axis of gaze origin point, NaN if unknown. The reference system depends on specific application.
 * @param gaze_origin_y [m] Y axis of gaze origin point, NaN if unknown. The reference system depends on specific application.
 * @param gaze_origin_z [m] Z axis of gaze origin point, NaN if unknown. The reference system depends on specific application.
 * @param gaze_direction_x  X axis of gaze direction vector, expected to be normalized to unit magnitude, NaN if unknown. The reference system should match origin point.
 * @param gaze_direction_y  Y axis of gaze direction vector, expected to be normalized to unit magnitude, NaN if unknown. The reference system should match origin point.
 * @param gaze_direction_z  Z axis of gaze direction vector, expected to be normalized to unit magnitude, NaN if unknown. The reference system should match origin point.
 * @param video_gaze_x  Gaze focal point on video feed x value (normalized 0..1, 0 is left, 1 is right), NaN if unknown
 * @param video_gaze_y  Gaze focal point on video feed y value (normalized 0..1, 0 is top, 1 is bottom), NaN if unknown
 * @param surface_id  Identifier of surface for 2D gaze point, or an identified region when surface point is invalid. Set to zero if unknown/unused.
 * @param surface_gaze_x  Gaze focal point on surface x value (normalized 0..1, 0 is left, 1 is right), NaN if unknown
 * @param surface_gaze_y  Gaze focal point on surface y value (normalized 0..1, 0 is top, 1 is bottom), NaN if unknown
 * @return length of the message in bytes (excluding serial stream start sign)
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_eye_tracking_data_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint64_t time_usec,uint8_t sensor_id,float gaze_origin_x,float gaze_origin_y,float gaze_origin_z,float gaze_direction_x,float gaze_direction_y,float gaze_direction_z,float video_gaze_x,float video_gaze_y,uint8_t surface_id,float surface_gaze_x,float surface_gaze_y)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_EYE_TRACKING_DATA_LEN];
    _mav_put_uint64_t(buf, 0, time_usec);
    _mav_put_float(buf, 8, gaze_origin_x);
    _mav_put_float(buf, 12, gaze_origin_y);
    _mav_put_float(buf, 16, gaze_origin_z);
    _mav_put_float(buf, 20, gaze_direction_x);
    _mav_put_float(buf, 24, gaze_direction_y);
    _mav_put_float(buf, 28, gaze_direction_z);
    _mav_put_float(buf, 32, video_gaze_x);
    _mav_put_float(buf, 36, video_gaze_y);
    _mav_put_float(buf, 40, surface_gaze_x);
    _mav_put_float(buf, 44, surface_gaze_y);
    _mav_put_uint8_t(buf, 48, sensor_id);
    _mav_put_uint8_t(buf, 49, surface_id);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_EYE_TRACKING_DATA_LEN);
#else
    mavlink_eye_tracking_data_t packet;
    packet.time_usec = time_usec;
    packet.gaze_origin_x = gaze_origin_x;
    packet.gaze_origin_y = gaze_origin_y;
    packet.gaze_origin_z = gaze_origin_z;
    packet.gaze_direction_x = gaze_direction_x;
    packet.gaze_direction_y = gaze_direction_y;
    packet.gaze_direction_z = gaze_direction_z;
    packet.video_gaze_x = video_gaze_x;
    packet.video_gaze_y = video_gaze_y;
    packet.surface_gaze_x = surface_gaze_x;
    packet.surface_gaze_y = surface_gaze_y;
    packet.sensor_id = sensor_id;
    packet.surface_id = surface_id;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_EYE_TRACKING_DATA_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_EYE_TRACKING_DATA;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_EYE_TRACKING_DATA_MIN_LEN, MAVLINK_MSG_ID_EYE_TRACKING_DATA_LEN, MAVLINK_MSG_ID_EYE_TRACKING_DATA_CRC);
}

/**
 * @brief Encode a eye_tracking_data struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param eye_tracking_data C-struct to read the message contents from
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_eye_tracking_data_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_eye_tracking_data_t* eye_tracking_data)
{
    return mavlink_msg_eye_tracking_data_pack(system_id, component_id, msg, eye_tracking_data->time_usec, eye_tracking_data->sensor_id, eye_tracking_data->gaze_origin_x, eye_tracking_data->gaze_origin_y, eye_tracking_data->gaze_origin_z, eye_tracking_data->gaze_direction_x, eye_tracking_data->gaze_direction_y, eye_tracking_data->gaze_direction_z, eye_tracking_data->video_gaze_x, eye_tracking_data->video_gaze_y, eye_tracking_data->surface_id, eye_tracking_data->surface_gaze_x, eye_tracking_data->surface_gaze_y);
}

/**
 * @brief Encode a eye_tracking_data struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param eye_tracking_data C-struct to read the message contents from
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_eye_tracking_data_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_eye_tracking_data_t* eye_tracking_data)
{
    return mavlink_msg_eye_tracking_data_pack_chan(system_id, component_id, chan, msg, eye_tracking_data->time_usec, eye_tracking_data->sensor_id, eye_tracking_data->gaze_origin_x, eye_tracking_data->gaze_origin_y, eye_tracking_data->gaze_origin_z, eye_tracking_data->gaze_direction_x, eye_tracking_data->gaze_direction_y, eye_tracking_data->gaze_direction_z, eye_tracking_data->video_gaze_x, eye_tracking_data->video_gaze_y, eye_tracking_data->surface_id, eye_tracking_data->surface_gaze_x, eye_tracking_data->surface_gaze_y);
}

/**
 * @brief Encode a eye_tracking_data struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param eye_tracking_data C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_eye_tracking_data_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_eye_tracking_data_t* eye_tracking_data)
{
    return mavlink_msg_eye_tracking_data_pack_status(system_id, component_id, _status, msg,  eye_tracking_data->time_usec, eye_tracking_data->sensor_id, eye_tracking_data->gaze_origin_x, eye_tracking_data->gaze_origin_y, eye_tracking_data->gaze_origin_z, eye_tracking_data->gaze_direction_x, eye_tracking_data->gaze_direction_y, eye_tracking_data->gaze_direction_z, eye_tracking_data->video_gaze_x, eye_tracking_data->video_gaze_y, eye_tracking_data->surface_id, eye_tracking_data->surface_gaze_x, eye_tracking_data->surface_gaze_y);
}

/**
 * @brief Send a eye_tracking_data message
 * @param chan MAVLink channel to send the message
 *
 * @param time_usec [us] Timestamp (time since system boot).
 * @param sensor_id  Sensor ID, used for identifying the device and/or person tracked. Set to zero if unknown/unused.
 * @param gaze_origin_x [m] X axis of gaze origin point, NaN if unknown. The reference system depends on specific application.
 * @param gaze_origin_y [m] Y axis of gaze origin point, NaN if unknown. The reference system depends on specific application.
 * @param gaze_origin_z [m] Z axis of gaze origin point, NaN if unknown. The reference system depends on specific application.
 * @param gaze_direction_x  X axis of gaze direction vector, expected to be normalized to unit magnitude, NaN if unknown. The reference system should match origin point.
 * @param gaze_direction_y  Y axis of gaze direction vector, expected to be normalized to unit magnitude, NaN if unknown. The reference system should match origin point.
 * @param gaze_direction_z  Z axis of gaze direction vector, expected to be normalized to unit magnitude, NaN if unknown. The reference system should match origin point.
 * @param video_gaze_x  Gaze focal point on video feed x value (normalized 0..1, 0 is left, 1 is right), NaN if unknown
 * @param video_gaze_y  Gaze focal point on video feed y value (normalized 0..1, 0 is top, 1 is bottom), NaN if unknown
 * @param surface_id  Identifier of surface for 2D gaze point, or an identified region when surface point is invalid. Set to zero if unknown/unused.
 * @param surface_gaze_x  Gaze focal point on surface x value (normalized 0..1, 0 is left, 1 is right), NaN if unknown
 * @param surface_gaze_y  Gaze focal point on surface y value (normalized 0..1, 0 is top, 1 is bottom), NaN if unknown
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

MAVLINK_WIP
static inline void mavlink_msg_eye_tracking_data_send(mavlink_channel_t chan, uint64_t time_usec, uint8_t sensor_id, float gaze_origin_x, float gaze_origin_y, float gaze_origin_z, float gaze_direction_x, float gaze_direction_y, float gaze_direction_z, float video_gaze_x, float video_gaze_y, uint8_t surface_id, float surface_gaze_x, float surface_gaze_y)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_EYE_TRACKING_DATA_LEN];
    _mav_put_uint64_t(buf, 0, time_usec);
    _mav_put_float(buf, 8, gaze_origin_x);
    _mav_put_float(buf, 12, gaze_origin_y);
    _mav_put_float(buf, 16, gaze_origin_z);
    _mav_put_float(buf, 20, gaze_direction_x);
    _mav_put_float(buf, 24, gaze_direction_y);
    _mav_put_float(buf, 28, gaze_direction_z);
    _mav_put_float(buf, 32, video_gaze_x);
    _mav_put_float(buf, 36, video_gaze_y);
    _mav_put_float(buf, 40, surface_gaze_x);
    _mav_put_float(buf, 44, surface_gaze_y);
    _mav_put_uint8_t(buf, 48, sensor_id);
    _mav_put_uint8_t(buf, 49, surface_id);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_EYE_TRACKING_DATA, buf, MAVLINK_MSG_ID_EYE_TRACKING_DATA_MIN_LEN, MAVLINK_MSG_ID_EYE_TRACKING_DATA_LEN, MAVLINK_MSG_ID_EYE_TRACKING_DATA_CRC);
#else
    mavlink_eye_tracking_data_t packet;
    packet.time_usec = time_usec;
    packet.gaze_origin_x = gaze_origin_x;
    packet.gaze_origin_y = gaze_origin_y;
    packet.gaze_origin_z = gaze_origin_z;
    packet.gaze_direction_x = gaze_direction_x;
    packet.gaze_direction_y = gaze_direction_y;
    packet.gaze_direction_z = gaze_direction_z;
    packet.video_gaze_x = video_gaze_x;
    packet.video_gaze_y = video_gaze_y;
    packet.surface_gaze_x = surface_gaze_x;
    packet.surface_gaze_y = surface_gaze_y;
    packet.sensor_id = sensor_id;
    packet.surface_id = surface_id;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_EYE_TRACKING_DATA, (const char *)&packet, MAVLINK_MSG_ID_EYE_TRACKING_DATA_MIN_LEN, MAVLINK_MSG_ID_EYE_TRACKING_DATA_LEN, MAVLINK_MSG_ID_EYE_TRACKING_DATA_CRC);
#endif
}

/**
 * @brief Send a eye_tracking_data message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
MAVLINK_WIP
static inline void mavlink_msg_eye_tracking_data_send_struct(mavlink_channel_t chan, const mavlink_eye_tracking_data_t* eye_tracking_data)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_eye_tracking_data_send(chan, eye_tracking_data->time_usec, eye_tracking_data->sensor_id, eye_tracking_data->gaze_origin_x, eye_tracking_data->gaze_origin_y, eye_tracking_data->gaze_origin_z, eye_tracking_data->gaze_direction_x, eye_tracking_data->gaze_direction_y, eye_tracking_data->gaze_direction_z, eye_tracking_data->video_gaze_x, eye_tracking_data->video_gaze_y, eye_tracking_data->surface_id, eye_tracking_data->surface_gaze_x, eye_tracking_data->surface_gaze_y);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_EYE_TRACKING_DATA, (const char *)eye_tracking_data, MAVLINK_MSG_ID_EYE_TRACKING_DATA_MIN_LEN, MAVLINK_MSG_ID_EYE_TRACKING_DATA_LEN, MAVLINK_MSG_ID_EYE_TRACKING_DATA_CRC);
#endif
}

#if MAVLINK_MSG_ID_EYE_TRACKING_DATA_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
MAVLINK_WIP
static inline void mavlink_msg_eye_tracking_data_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint64_t time_usec, uint8_t sensor_id, float gaze_origin_x, float gaze_origin_y, float gaze_origin_z, float gaze_direction_x, float gaze_direction_y, float gaze_direction_z, float video_gaze_x, float video_gaze_y, uint8_t surface_id, float surface_gaze_x, float surface_gaze_y)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint64_t(buf, 0, time_usec);
    _mav_put_float(buf, 8, gaze_origin_x);
    _mav_put_float(buf, 12, gaze_origin_y);
    _mav_put_float(buf, 16, gaze_origin_z);
    _mav_put_float(buf, 20, gaze_direction_x);
    _mav_put_float(buf, 24, gaze_direction_y);
    _mav_put_float(buf, 28, gaze_direction_z);
    _mav_put_float(buf, 32, video_gaze_x);
    _mav_put_float(buf, 36, video_gaze_y);
    _mav_put_float(buf, 40, surface_gaze_x);
    _mav_put_float(buf, 44, surface_gaze_y);
    _mav_put_uint8_t(buf, 48, sensor_id);
    _mav_put_uint8_t(buf, 49, surface_id);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_EYE_TRACKING_DATA, buf, MAVLINK_MSG_ID_EYE_TRACKING_DATA_MIN_LEN, MAVLINK_MSG_ID_EYE_TRACKING_DATA_LEN, MAVLINK_MSG_ID_EYE_TRACKING_DATA_CRC);
#else
    mavlink_eye_tracking_data_t *packet = (mavlink_eye_tracking_data_t *)msgbuf;
    packet->time_usec = time_usec;
    packet->gaze_origin_x = gaze_origin_x;
    packet->gaze_origin_y = gaze_origin_y;
    packet->gaze_origin_z = gaze_origin_z;
    packet->gaze_direction_x = gaze_direction_x;
    packet->gaze_direction_y = gaze_direction_y;
    packet->gaze_direction_z = gaze_direction_z;
    packet->video_gaze_x = video_gaze_x;
    packet->video_gaze_y = video_gaze_y;
    packet->surface_gaze_x = surface_gaze_x;
    packet->surface_gaze_y = surface_gaze_y;
    packet->sensor_id = sensor_id;
    packet->surface_id = surface_id;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_EYE_TRACKING_DATA, (const char *)packet, MAVLINK_MSG_ID_EYE_TRACKING_DATA_MIN_LEN, MAVLINK_MSG_ID_EYE_TRACKING_DATA_LEN, MAVLINK_MSG_ID_EYE_TRACKING_DATA_CRC);
#endif
}
#endif

#endif

// MESSAGE EYE_TRACKING_DATA UNPACKING


/**
 * @brief Get field time_usec from eye_tracking_data message
 *
 * @return [us] Timestamp (time since system boot).
 */
MAVLINK_WIP
static inline uint64_t mavlink_msg_eye_tracking_data_get_time_usec(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint64_t(msg,  0);
}

/**
 * @brief Get field sensor_id from eye_tracking_data message
 *
 * @return  Sensor ID, used for identifying the device and/or person tracked. Set to zero if unknown/unused.
 */
MAVLINK_WIP
static inline uint8_t mavlink_msg_eye_tracking_data_get_sensor_id(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  48);
}

/**
 * @brief Get field gaze_origin_x from eye_tracking_data message
 *
 * @return [m] X axis of gaze origin point, NaN if unknown. The reference system depends on specific application.
 */
MAVLINK_WIP
static inline float mavlink_msg_eye_tracking_data_get_gaze_origin_x(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  8);
}

/**
 * @brief Get field gaze_origin_y from eye_tracking_data message
 *
 * @return [m] Y axis of gaze origin point, NaN if unknown. The reference system depends on specific application.
 */
MAVLINK_WIP
static inline float mavlink_msg_eye_tracking_data_get_gaze_origin_y(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  12);
}

/**
 * @brief Get field gaze_origin_z from eye_tracking_data message
 *
 * @return [m] Z axis of gaze origin point, NaN if unknown. The reference system depends on specific application.
 */
MAVLINK_WIP
static inline float mavlink_msg_eye_tracking_data_get_gaze_origin_z(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  16);
}

/**
 * @brief Get field gaze_direction_x from eye_tracking_data message
 *
 * @return  X axis of gaze direction vector, expected to be normalized to unit magnitude, NaN if unknown. The reference system should match origin point.
 */
MAVLINK_WIP
static inline float mavlink_msg_eye_tracking_data_get_gaze_direction_x(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  20);
}

/**
 * @brief Get field gaze_direction_y from eye_tracking_data message
 *
 * @return  Y axis of gaze direction vector, expected to be normalized to unit magnitude, NaN if unknown. The reference system should match origin point.
 */
MAVLINK_WIP
static inline float mavlink_msg_eye_tracking_data_get_gaze_direction_y(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  24);
}

/**
 * @brief Get field gaze_direction_z from eye_tracking_data message
 *
 * @return  Z axis of gaze direction vector, expected to be normalized to unit magnitude, NaN if unknown. The reference system should match origin point.
 */
MAVLINK_WIP
static inline float mavlink_msg_eye_tracking_data_get_gaze_direction_z(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  28);
}

/**
 * @brief Get field video_gaze_x from eye_tracking_data message
 *
 * @return  Gaze focal point on video feed x value (normalized 0..1, 0 is left, 1 is right), NaN if unknown
 */
MAVLINK_WIP
static inline float mavlink_msg_eye_tracking_data_get_video_gaze_x(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  32);
}

/**
 * @brief Get field video_gaze_y from eye_tracking_data message
 *
 * @return  Gaze focal point on video feed y value (normalized 0..1, 0 is top, 1 is bottom), NaN if unknown
 */
MAVLINK_WIP
static inline float mavlink_msg_eye_tracking_data_get_video_gaze_y(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  36);
}

/**
 * @brief Get field surface_id from eye_tracking_data message
 *
 * @return  Identifier of surface for 2D gaze point, or an identified region when surface point is invalid. Set to zero if unknown/unused.
 */
MAVLINK_WIP
static inline uint8_t mavlink_msg_eye_tracking_data_get_surface_id(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  49);
}

/**
 * @brief Get field surface_gaze_x from eye_tracking_data message
 *
 * @return  Gaze focal point on surface x value (normalized 0..1, 0 is left, 1 is right), NaN if unknown
 */
MAVLINK_WIP
static inline float mavlink_msg_eye_tracking_data_get_surface_gaze_x(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  40);
}

/**
 * @brief Get field surface_gaze_y from eye_tracking_data message
 *
 * @return  Gaze focal point on surface y value (normalized 0..1, 0 is top, 1 is bottom), NaN if unknown
 */
MAVLINK_WIP
static inline float mavlink_msg_eye_tracking_data_get_surface_gaze_y(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  44);
}

/**
 * @brief Decode a eye_tracking_data message into a struct
 *
 * @param msg The message to decode
 * @param eye_tracking_data C-struct to decode the message contents into
 */
MAVLINK_WIP
static inline void mavlink_msg_eye_tracking_data_decode(const mavlink_message_t* msg, mavlink_eye_tracking_data_t* eye_tracking_data)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    eye_tracking_data->time_usec = mavlink_msg_eye_tracking_data_get_time_usec(msg);
    eye_tracking_data->gaze_origin_x = mavlink_msg_eye_tracking_data_get_gaze_origin_x(msg);
    eye_tracking_data->gaze_origin_y = mavlink_msg_eye_tracking_data_get_gaze_origin_y(msg);
    eye_tracking_data->gaze_origin_z = mavlink_msg_eye_tracking_data_get_gaze_origin_z(msg);
    eye_tracking_data->gaze_direction_x = mavlink_msg_eye_tracking_data_get_gaze_direction_x(msg);
    eye_tracking_data->gaze_direction_y = mavlink_msg_eye_tracking_data_get_gaze_direction_y(msg);
    eye_tracking_data->gaze_direction_z = mavlink_msg_eye_tracking_data_get_gaze_direction_z(msg);
    eye_tracking_data->video_gaze_x = mavlink_msg_eye_tracking_data_get_video_gaze_x(msg);
    eye_tracking_data->video_gaze_y = mavlink_msg_eye_tracking_data_get_video_gaze_y(msg);
    eye_tracking_data->surface_gaze_x = mavlink_msg_eye_tracking_data_get_surface_gaze_x(msg);
    eye_tracking_data->surface_gaze_y = mavlink_msg_eye_tracking_data_get_surface_gaze_y(msg);
    eye_tracking_data->sensor_id = mavlink_msg_eye_tracking_data_get_sensor_id(msg);
    eye_tracking_data->surface_id = mavlink_msg_eye_tracking_data_get_surface_id(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_EYE_TRACKING_DATA_LEN? msg->len : MAVLINK_MSG_ID_EYE_TRACKING_DATA_LEN;
        memset(eye_tracking_data, 0, MAVLINK_MSG_ID_EYE_TRACKING_DATA_LEN);
    memcpy(eye_tracking_data, _MAV_PAYLOAD(msg), len);
#endif
}
