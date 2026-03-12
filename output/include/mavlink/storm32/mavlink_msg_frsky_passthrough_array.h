#pragma once
// MESSAGE FRSKY_PASSTHROUGH_ARRAY PACKING

#define MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY 60040


typedef struct __mavlink_frsky_passthrough_array_t {
 uint32_t time_boot_ms; /*< [ms] Timestamp (time since system boot).*/
 uint8_t count; /*<  Number of passthrough packets in this message.*/
 uint8_t packet_buf[240]; /*<  Passthrough packet buffer. A packet has 6 bytes: uint16_t id + uint32_t data. The array has space for 40 packets.*/
} mavlink_frsky_passthrough_array_t;

#define MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_LEN 245
#define MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_MIN_LEN 245
#define MAVLINK_MSG_ID_60040_LEN 245
#define MAVLINK_MSG_ID_60040_MIN_LEN 245

#define MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_CRC 156
#define MAVLINK_MSG_ID_60040_CRC 156

#define MAVLINK_MSG_FRSKY_PASSTHROUGH_ARRAY_FIELD_PACKET_BUF_LEN 240

#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_FRSKY_PASSTHROUGH_ARRAY { \
    60040, \
    "FRSKY_PASSTHROUGH_ARRAY", \
    3, \
    {  { "time_boot_ms", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_frsky_passthrough_array_t, time_boot_ms) }, \
         { "count", NULL, MAVLINK_TYPE_UINT8_T, 0, 4, offsetof(mavlink_frsky_passthrough_array_t, count) }, \
         { "packet_buf", NULL, MAVLINK_TYPE_UINT8_T, 240, 5, offsetof(mavlink_frsky_passthrough_array_t, packet_buf) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_FRSKY_PASSTHROUGH_ARRAY { \
    "FRSKY_PASSTHROUGH_ARRAY", \
    3, \
    {  { "time_boot_ms", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_frsky_passthrough_array_t, time_boot_ms) }, \
         { "count", NULL, MAVLINK_TYPE_UINT8_T, 0, 4, offsetof(mavlink_frsky_passthrough_array_t, count) }, \
         { "packet_buf", NULL, MAVLINK_TYPE_UINT8_T, 240, 5, offsetof(mavlink_frsky_passthrough_array_t, packet_buf) }, \
         } \
}
#endif

/**
 * @brief Pack a frsky_passthrough_array message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_boot_ms [ms] Timestamp (time since system boot).
 * @param count  Number of passthrough packets in this message.
 * @param packet_buf  Passthrough packet buffer. A packet has 6 bytes: uint16_t id + uint32_t data. The array has space for 40 packets.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_frsky_passthrough_array_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint32_t time_boot_ms, uint8_t count, const uint8_t *packet_buf)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_uint8_t(buf, 4, count);
    _mav_put_uint8_t_array(buf, 5, packet_buf, 240);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_LEN);
#else
    mavlink_frsky_passthrough_array_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.count = count;
    mav_array_memcpy(packet.packet_buf, packet_buf, sizeof(uint8_t)*240);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_MIN_LEN, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_LEN, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_CRC);
}

/**
 * @brief Pack a frsky_passthrough_array message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_boot_ms [ms] Timestamp (time since system boot).
 * @param count  Number of passthrough packets in this message.
 * @param packet_buf  Passthrough packet buffer. A packet has 6 bytes: uint16_t id + uint32_t data. The array has space for 40 packets.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_frsky_passthrough_array_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint32_t time_boot_ms, uint8_t count, const uint8_t *packet_buf)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_uint8_t(buf, 4, count);
    _mav_put_uint8_t_array(buf, 5, packet_buf, 240);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_LEN);
#else
    mavlink_frsky_passthrough_array_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.count = count;
    mav_array_memcpy(packet.packet_buf, packet_buf, sizeof(uint8_t)*240);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_MIN_LEN, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_LEN, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_MIN_LEN, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_LEN);
#endif
}

/**
 * @brief Pack a frsky_passthrough_array message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param time_boot_ms [ms] Timestamp (time since system boot).
 * @param count  Number of passthrough packets in this message.
 * @param packet_buf  Passthrough packet buffer. A packet has 6 bytes: uint16_t id + uint32_t data. The array has space for 40 packets.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_frsky_passthrough_array_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint32_t time_boot_ms,uint8_t count,const uint8_t *packet_buf)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_uint8_t(buf, 4, count);
    _mav_put_uint8_t_array(buf, 5, packet_buf, 240);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_LEN);
#else
    mavlink_frsky_passthrough_array_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.count = count;
    mav_array_memcpy(packet.packet_buf, packet_buf, sizeof(uint8_t)*240);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_MIN_LEN, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_LEN, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_CRC);
}

/**
 * @brief Encode a frsky_passthrough_array struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param frsky_passthrough_array C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_frsky_passthrough_array_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_frsky_passthrough_array_t* frsky_passthrough_array)
{
    return mavlink_msg_frsky_passthrough_array_pack(system_id, component_id, msg, frsky_passthrough_array->time_boot_ms, frsky_passthrough_array->count, frsky_passthrough_array->packet_buf);
}

/**
 * @brief Encode a frsky_passthrough_array struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param frsky_passthrough_array C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_frsky_passthrough_array_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_frsky_passthrough_array_t* frsky_passthrough_array)
{
    return mavlink_msg_frsky_passthrough_array_pack_chan(system_id, component_id, chan, msg, frsky_passthrough_array->time_boot_ms, frsky_passthrough_array->count, frsky_passthrough_array->packet_buf);
}

/**
 * @brief Encode a frsky_passthrough_array struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param frsky_passthrough_array C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_frsky_passthrough_array_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_frsky_passthrough_array_t* frsky_passthrough_array)
{
    return mavlink_msg_frsky_passthrough_array_pack_status(system_id, component_id, _status, msg,  frsky_passthrough_array->time_boot_ms, frsky_passthrough_array->count, frsky_passthrough_array->packet_buf);
}

/**
 * @brief Send a frsky_passthrough_array message
 * @param chan MAVLink channel to send the message
 *
 * @param time_boot_ms [ms] Timestamp (time since system boot).
 * @param count  Number of passthrough packets in this message.
 * @param packet_buf  Passthrough packet buffer. A packet has 6 bytes: uint16_t id + uint32_t data. The array has space for 40 packets.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_frsky_passthrough_array_send(mavlink_channel_t chan, uint32_t time_boot_ms, uint8_t count, const uint8_t *packet_buf)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_uint8_t(buf, 4, count);
    _mav_put_uint8_t_array(buf, 5, packet_buf, 240);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY, buf, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_MIN_LEN, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_LEN, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_CRC);
#else
    mavlink_frsky_passthrough_array_t packet;
    packet.time_boot_ms = time_boot_ms;
    packet.count = count;
    mav_array_memcpy(packet.packet_buf, packet_buf, sizeof(uint8_t)*240);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY, (const char *)&packet, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_MIN_LEN, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_LEN, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_CRC);
#endif
}

/**
 * @brief Send a frsky_passthrough_array message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_frsky_passthrough_array_send_struct(mavlink_channel_t chan, const mavlink_frsky_passthrough_array_t* frsky_passthrough_array)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_frsky_passthrough_array_send(chan, frsky_passthrough_array->time_boot_ms, frsky_passthrough_array->count, frsky_passthrough_array->packet_buf);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY, (const char *)frsky_passthrough_array, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_MIN_LEN, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_LEN, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_CRC);
#endif
}

#if MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_frsky_passthrough_array_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint32_t time_boot_ms, uint8_t count, const uint8_t *packet_buf)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_uint8_t(buf, 4, count);
    _mav_put_uint8_t_array(buf, 5, packet_buf, 240);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY, buf, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_MIN_LEN, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_LEN, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_CRC);
#else
    mavlink_frsky_passthrough_array_t *packet = (mavlink_frsky_passthrough_array_t *)msgbuf;
    packet->time_boot_ms = time_boot_ms;
    packet->count = count;
    mav_array_memcpy(packet->packet_buf, packet_buf, sizeof(uint8_t)*240);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY, (const char *)packet, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_MIN_LEN, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_LEN, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_CRC);
#endif
}
#endif

#endif

// MESSAGE FRSKY_PASSTHROUGH_ARRAY UNPACKING


/**
 * @brief Get field time_boot_ms from frsky_passthrough_array message
 *
 * @return [ms] Timestamp (time since system boot).
 */
static inline uint32_t mavlink_msg_frsky_passthrough_array_get_time_boot_ms(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  0);
}

/**
 * @brief Get field count from frsky_passthrough_array message
 *
 * @return  Number of passthrough packets in this message.
 */
static inline uint8_t mavlink_msg_frsky_passthrough_array_get_count(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  4);
}

/**
 * @brief Get field packet_buf from frsky_passthrough_array message
 *
 * @return  Passthrough packet buffer. A packet has 6 bytes: uint16_t id + uint32_t data. The array has space for 40 packets.
 */
static inline uint16_t mavlink_msg_frsky_passthrough_array_get_packet_buf(const mavlink_message_t* msg, uint8_t *packet_buf)
{
    return _MAV_RETURN_uint8_t_array(msg, packet_buf, 240,  5);
}

/**
 * @brief Decode a frsky_passthrough_array message into a struct
 *
 * @param msg The message to decode
 * @param frsky_passthrough_array C-struct to decode the message contents into
 */
static inline void mavlink_msg_frsky_passthrough_array_decode(const mavlink_message_t* msg, mavlink_frsky_passthrough_array_t* frsky_passthrough_array)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    frsky_passthrough_array->time_boot_ms = mavlink_msg_frsky_passthrough_array_get_time_boot_ms(msg);
    frsky_passthrough_array->count = mavlink_msg_frsky_passthrough_array_get_count(msg);
    mavlink_msg_frsky_passthrough_array_get_packet_buf(msg, frsky_passthrough_array->packet_buf);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_LEN? msg->len : MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_LEN;
        memset(frsky_passthrough_array, 0, MAVLINK_MSG_ID_FRSKY_PASSTHROUGH_ARRAY_LEN);
    memcpy(frsky_passthrough_array, _MAV_PAYLOAD(msg), len);
#endif
}
