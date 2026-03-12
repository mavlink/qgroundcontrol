#pragma once
// MESSAGE PARAM_VALUE_ARRAY PACKING

#define MAVLINK_MSG_ID_PARAM_VALUE_ARRAY 60041


typedef struct __mavlink_param_value_array_t {
 uint16_t param_count; /*<  Total number of onboard parameters.*/
 uint16_t param_index_first; /*<  Index of the first onboard parameter in this array.*/
 uint16_t flags; /*<  Flags.*/
 uint8_t param_array_len; /*<  Number of onboard parameters in this array.*/
 uint8_t packet_buf[248]; /*<  Parameters buffer. Contains a series of variable length parameter blocks, one per parameter, with format as specified elsewhere.*/
} mavlink_param_value_array_t;

#define MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_LEN 255
#define MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_MIN_LEN 255
#define MAVLINK_MSG_ID_60041_LEN 255
#define MAVLINK_MSG_ID_60041_MIN_LEN 255

#define MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_CRC 191
#define MAVLINK_MSG_ID_60041_CRC 191

#define MAVLINK_MSG_PARAM_VALUE_ARRAY_FIELD_PACKET_BUF_LEN 248

#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_PARAM_VALUE_ARRAY { \
    60041, \
    "PARAM_VALUE_ARRAY", \
    5, \
    {  { "param_count", NULL, MAVLINK_TYPE_UINT16_T, 0, 0, offsetof(mavlink_param_value_array_t, param_count) }, \
         { "param_index_first", NULL, MAVLINK_TYPE_UINT16_T, 0, 2, offsetof(mavlink_param_value_array_t, param_index_first) }, \
         { "param_array_len", NULL, MAVLINK_TYPE_UINT8_T, 0, 6, offsetof(mavlink_param_value_array_t, param_array_len) }, \
         { "flags", NULL, MAVLINK_TYPE_UINT16_T, 0, 4, offsetof(mavlink_param_value_array_t, flags) }, \
         { "packet_buf", NULL, MAVLINK_TYPE_UINT8_T, 248, 7, offsetof(mavlink_param_value_array_t, packet_buf) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_PARAM_VALUE_ARRAY { \
    "PARAM_VALUE_ARRAY", \
    5, \
    {  { "param_count", NULL, MAVLINK_TYPE_UINT16_T, 0, 0, offsetof(mavlink_param_value_array_t, param_count) }, \
         { "param_index_first", NULL, MAVLINK_TYPE_UINT16_T, 0, 2, offsetof(mavlink_param_value_array_t, param_index_first) }, \
         { "param_array_len", NULL, MAVLINK_TYPE_UINT8_T, 0, 6, offsetof(mavlink_param_value_array_t, param_array_len) }, \
         { "flags", NULL, MAVLINK_TYPE_UINT16_T, 0, 4, offsetof(mavlink_param_value_array_t, flags) }, \
         { "packet_buf", NULL, MAVLINK_TYPE_UINT8_T, 248, 7, offsetof(mavlink_param_value_array_t, packet_buf) }, \
         } \
}
#endif

/**
 * @brief Pack a param_value_array message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param param_count  Total number of onboard parameters.
 * @param param_index_first  Index of the first onboard parameter in this array.
 * @param param_array_len  Number of onboard parameters in this array.
 * @param flags  Flags.
 * @param packet_buf  Parameters buffer. Contains a series of variable length parameter blocks, one per parameter, with format as specified elsewhere.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_param_value_array_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint16_t param_count, uint16_t param_index_first, uint8_t param_array_len, uint16_t flags, const uint8_t *packet_buf)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_LEN];
    _mav_put_uint16_t(buf, 0, param_count);
    _mav_put_uint16_t(buf, 2, param_index_first);
    _mav_put_uint16_t(buf, 4, flags);
    _mav_put_uint8_t(buf, 6, param_array_len);
    _mav_put_uint8_t_array(buf, 7, packet_buf, 248);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_LEN);
#else
    mavlink_param_value_array_t packet;
    packet.param_count = param_count;
    packet.param_index_first = param_index_first;
    packet.flags = flags;
    packet.param_array_len = param_array_len;
    mav_array_memcpy(packet.packet_buf, packet_buf, sizeof(uint8_t)*248);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_PARAM_VALUE_ARRAY;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_MIN_LEN, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_LEN, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_CRC);
}

/**
 * @brief Pack a param_value_array message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param param_count  Total number of onboard parameters.
 * @param param_index_first  Index of the first onboard parameter in this array.
 * @param param_array_len  Number of onboard parameters in this array.
 * @param flags  Flags.
 * @param packet_buf  Parameters buffer. Contains a series of variable length parameter blocks, one per parameter, with format as specified elsewhere.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_param_value_array_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint16_t param_count, uint16_t param_index_first, uint8_t param_array_len, uint16_t flags, const uint8_t *packet_buf)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_LEN];
    _mav_put_uint16_t(buf, 0, param_count);
    _mav_put_uint16_t(buf, 2, param_index_first);
    _mav_put_uint16_t(buf, 4, flags);
    _mav_put_uint8_t(buf, 6, param_array_len);
    _mav_put_uint8_t_array(buf, 7, packet_buf, 248);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_LEN);
#else
    mavlink_param_value_array_t packet;
    packet.param_count = param_count;
    packet.param_index_first = param_index_first;
    packet.flags = flags;
    packet.param_array_len = param_array_len;
    mav_array_memcpy(packet.packet_buf, packet_buf, sizeof(uint8_t)*248);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_PARAM_VALUE_ARRAY;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_MIN_LEN, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_LEN, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_MIN_LEN, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_LEN);
#endif
}

/**
 * @brief Pack a param_value_array message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param param_count  Total number of onboard parameters.
 * @param param_index_first  Index of the first onboard parameter in this array.
 * @param param_array_len  Number of onboard parameters in this array.
 * @param flags  Flags.
 * @param packet_buf  Parameters buffer. Contains a series of variable length parameter blocks, one per parameter, with format as specified elsewhere.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_param_value_array_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint16_t param_count,uint16_t param_index_first,uint8_t param_array_len,uint16_t flags,const uint8_t *packet_buf)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_LEN];
    _mav_put_uint16_t(buf, 0, param_count);
    _mav_put_uint16_t(buf, 2, param_index_first);
    _mav_put_uint16_t(buf, 4, flags);
    _mav_put_uint8_t(buf, 6, param_array_len);
    _mav_put_uint8_t_array(buf, 7, packet_buf, 248);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_LEN);
#else
    mavlink_param_value_array_t packet;
    packet.param_count = param_count;
    packet.param_index_first = param_index_first;
    packet.flags = flags;
    packet.param_array_len = param_array_len;
    mav_array_memcpy(packet.packet_buf, packet_buf, sizeof(uint8_t)*248);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_PARAM_VALUE_ARRAY;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_MIN_LEN, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_LEN, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_CRC);
}

/**
 * @brief Encode a param_value_array struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param param_value_array C-struct to read the message contents from
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_param_value_array_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_param_value_array_t* param_value_array)
{
    return mavlink_msg_param_value_array_pack(system_id, component_id, msg, param_value_array->param_count, param_value_array->param_index_first, param_value_array->param_array_len, param_value_array->flags, param_value_array->packet_buf);
}

/**
 * @brief Encode a param_value_array struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param param_value_array C-struct to read the message contents from
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_param_value_array_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_param_value_array_t* param_value_array)
{
    return mavlink_msg_param_value_array_pack_chan(system_id, component_id, chan, msg, param_value_array->param_count, param_value_array->param_index_first, param_value_array->param_array_len, param_value_array->flags, param_value_array->packet_buf);
}

/**
 * @brief Encode a param_value_array struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param param_value_array C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_param_value_array_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_param_value_array_t* param_value_array)
{
    return mavlink_msg_param_value_array_pack_status(system_id, component_id, _status, msg,  param_value_array->param_count, param_value_array->param_index_first, param_value_array->param_array_len, param_value_array->flags, param_value_array->packet_buf);
}

/**
 * @brief Send a param_value_array message
 * @param chan MAVLink channel to send the message
 *
 * @param param_count  Total number of onboard parameters.
 * @param param_index_first  Index of the first onboard parameter in this array.
 * @param param_array_len  Number of onboard parameters in this array.
 * @param flags  Flags.
 * @param packet_buf  Parameters buffer. Contains a series of variable length parameter blocks, one per parameter, with format as specified elsewhere.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

MAVLINK_WIP
static inline void mavlink_msg_param_value_array_send(mavlink_channel_t chan, uint16_t param_count, uint16_t param_index_first, uint8_t param_array_len, uint16_t flags, const uint8_t *packet_buf)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_LEN];
    _mav_put_uint16_t(buf, 0, param_count);
    _mav_put_uint16_t(buf, 2, param_index_first);
    _mav_put_uint16_t(buf, 4, flags);
    _mav_put_uint8_t(buf, 6, param_array_len);
    _mav_put_uint8_t_array(buf, 7, packet_buf, 248);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY, buf, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_MIN_LEN, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_LEN, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_CRC);
#else
    mavlink_param_value_array_t packet;
    packet.param_count = param_count;
    packet.param_index_first = param_index_first;
    packet.flags = flags;
    packet.param_array_len = param_array_len;
    mav_array_memcpy(packet.packet_buf, packet_buf, sizeof(uint8_t)*248);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY, (const char *)&packet, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_MIN_LEN, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_LEN, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_CRC);
#endif
}

/**
 * @brief Send a param_value_array message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
MAVLINK_WIP
static inline void mavlink_msg_param_value_array_send_struct(mavlink_channel_t chan, const mavlink_param_value_array_t* param_value_array)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_param_value_array_send(chan, param_value_array->param_count, param_value_array->param_index_first, param_value_array->param_array_len, param_value_array->flags, param_value_array->packet_buf);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY, (const char *)param_value_array, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_MIN_LEN, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_LEN, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_CRC);
#endif
}

#if MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
MAVLINK_WIP
static inline void mavlink_msg_param_value_array_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint16_t param_count, uint16_t param_index_first, uint8_t param_array_len, uint16_t flags, const uint8_t *packet_buf)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint16_t(buf, 0, param_count);
    _mav_put_uint16_t(buf, 2, param_index_first);
    _mav_put_uint16_t(buf, 4, flags);
    _mav_put_uint8_t(buf, 6, param_array_len);
    _mav_put_uint8_t_array(buf, 7, packet_buf, 248);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY, buf, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_MIN_LEN, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_LEN, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_CRC);
#else
    mavlink_param_value_array_t *packet = (mavlink_param_value_array_t *)msgbuf;
    packet->param_count = param_count;
    packet->param_index_first = param_index_first;
    packet->flags = flags;
    packet->param_array_len = param_array_len;
    mav_array_memcpy(packet->packet_buf, packet_buf, sizeof(uint8_t)*248);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY, (const char *)packet, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_MIN_LEN, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_LEN, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_CRC);
#endif
}
#endif

#endif

// MESSAGE PARAM_VALUE_ARRAY UNPACKING


/**
 * @brief Get field param_count from param_value_array message
 *
 * @return  Total number of onboard parameters.
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_param_value_array_get_param_count(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  0);
}

/**
 * @brief Get field param_index_first from param_value_array message
 *
 * @return  Index of the first onboard parameter in this array.
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_param_value_array_get_param_index_first(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  2);
}

/**
 * @brief Get field param_array_len from param_value_array message
 *
 * @return  Number of onboard parameters in this array.
 */
MAVLINK_WIP
static inline uint8_t mavlink_msg_param_value_array_get_param_array_len(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  6);
}

/**
 * @brief Get field flags from param_value_array message
 *
 * @return  Flags.
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_param_value_array_get_flags(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint16_t(msg,  4);
}

/**
 * @brief Get field packet_buf from param_value_array message
 *
 * @return  Parameters buffer. Contains a series of variable length parameter blocks, one per parameter, with format as specified elsewhere.
 */
MAVLINK_WIP
static inline uint16_t mavlink_msg_param_value_array_get_packet_buf(const mavlink_message_t* msg, uint8_t *packet_buf)
{
    return _MAV_RETURN_uint8_t_array(msg, packet_buf, 248,  7);
}

/**
 * @brief Decode a param_value_array message into a struct
 *
 * @param msg The message to decode
 * @param param_value_array C-struct to decode the message contents into
 */
MAVLINK_WIP
static inline void mavlink_msg_param_value_array_decode(const mavlink_message_t* msg, mavlink_param_value_array_t* param_value_array)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    param_value_array->param_count = mavlink_msg_param_value_array_get_param_count(msg);
    param_value_array->param_index_first = mavlink_msg_param_value_array_get_param_index_first(msg);
    param_value_array->flags = mavlink_msg_param_value_array_get_flags(msg);
    param_value_array->param_array_len = mavlink_msg_param_value_array_get_param_array_len(msg);
    mavlink_msg_param_value_array_get_packet_buf(msg, param_value_array->packet_buf);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_LEN? msg->len : MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_LEN;
        memset(param_value_array, 0, MAVLINK_MSG_ID_PARAM_VALUE_ARRAY_LEN);
    memcpy(param_value_array, _MAV_PAYLOAD(msg), len);
#endif
}
