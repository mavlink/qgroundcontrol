#pragma once
// MESSAGE NAMED_VALUE_STRING PACKING

#define MAVLINK_MSG_ID_NAMED_VALUE_STRING 11060


typedef struct __mavlink_named_value_string_t {
 uint32_t time_boot_ms; /*< [ms] Timestamp (time since system boot).*/
 char name[10]; /*<  Name of the debug variable*/
 char value[64]; /*<  Value of the debug variable*/
} mavlink_named_value_string_t;

#define MAVLINK_MSG_ID_NAMED_VALUE_STRING_LEN 78
#define MAVLINK_MSG_ID_NAMED_VALUE_STRING_MIN_LEN 78
#define MAVLINK_MSG_ID_11060_LEN 78
#define MAVLINK_MSG_ID_11060_MIN_LEN 78

#define MAVLINK_MSG_ID_NAMED_VALUE_STRING_CRC 162
#define MAVLINK_MSG_ID_11060_CRC 162

#define MAVLINK_MSG_NAMED_VALUE_STRING_FIELD_NAME_LEN 10
#define MAVLINK_MSG_NAMED_VALUE_STRING_FIELD_VALUE_LEN 64

#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_NAMED_VALUE_STRING { \
    11060, \
    "NAMED_VALUE_STRING", \
    3, \
    {  { "time_boot_ms", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_named_value_string_t, time_boot_ms) }, \
         { "name", NULL, MAVLINK_TYPE_CHAR, 10, 4, offsetof(mavlink_named_value_string_t, name) }, \
         { "value", NULL, MAVLINK_TYPE_CHAR, 64, 14, offsetof(mavlink_named_value_string_t, value) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_NAMED_VALUE_STRING { \
    "NAMED_VALUE_STRING", \
    3, \
    {  { "time_boot_ms", NULL, MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_named_value_string_t, time_boot_ms) }, \
         { "name", NULL, MAVLINK_TYPE_CHAR, 10, 4, offsetof(mavlink_named_value_string_t, name) }, \
         { "value", NULL, MAVLINK_TYPE_CHAR, 64, 14, offsetof(mavlink_named_value_string_t, value) }, \
         } \
}
#endif

/**
 * @brief Pack a named_value_string message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_boot_ms [ms] Timestamp (time since system boot).
 * @param name  Name of the debug variable
 * @param value  Value of the debug variable
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_named_value_string_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint32_t time_boot_ms, const char *name, const char *value)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_NAMED_VALUE_STRING_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_char_array(buf, 4, name, 10);
    _mav_put_char_array(buf, 14, value, 64);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_NAMED_VALUE_STRING_LEN);
#else
    mavlink_named_value_string_t packet;
    packet.time_boot_ms = time_boot_ms;
    mav_array_memcpy(packet.name, name, sizeof(char)*10);
    mav_array_memcpy(packet.value, value, sizeof(char)*64);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_NAMED_VALUE_STRING_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_NAMED_VALUE_STRING;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_NAMED_VALUE_STRING_MIN_LEN, MAVLINK_MSG_ID_NAMED_VALUE_STRING_LEN, MAVLINK_MSG_ID_NAMED_VALUE_STRING_CRC);
}

/**
 * @brief Pack a named_value_string message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_boot_ms [ms] Timestamp (time since system boot).
 * @param name  Name of the debug variable
 * @param value  Value of the debug variable
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_named_value_string_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint32_t time_boot_ms, const char *name, const char *value)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_NAMED_VALUE_STRING_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_char_array(buf, 4, name, 10);
    _mav_put_char_array(buf, 14, value, 64);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_NAMED_VALUE_STRING_LEN);
#else
    mavlink_named_value_string_t packet;
    packet.time_boot_ms = time_boot_ms;
    mav_array_memcpy(packet.name, name, sizeof(char)*10);
    mav_array_memcpy(packet.value, value, sizeof(char)*64);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_NAMED_VALUE_STRING_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_NAMED_VALUE_STRING;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_NAMED_VALUE_STRING_MIN_LEN, MAVLINK_MSG_ID_NAMED_VALUE_STRING_LEN, MAVLINK_MSG_ID_NAMED_VALUE_STRING_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_NAMED_VALUE_STRING_MIN_LEN, MAVLINK_MSG_ID_NAMED_VALUE_STRING_LEN);
#endif
}

/**
 * @brief Pack a named_value_string message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param time_boot_ms [ms] Timestamp (time since system boot).
 * @param name  Name of the debug variable
 * @param value  Value of the debug variable
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_named_value_string_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint32_t time_boot_ms,const char *name,const char *value)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_NAMED_VALUE_STRING_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_char_array(buf, 4, name, 10);
    _mav_put_char_array(buf, 14, value, 64);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_NAMED_VALUE_STRING_LEN);
#else
    mavlink_named_value_string_t packet;
    packet.time_boot_ms = time_boot_ms;
    mav_array_memcpy(packet.name, name, sizeof(char)*10);
    mav_array_memcpy(packet.value, value, sizeof(char)*64);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_NAMED_VALUE_STRING_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_NAMED_VALUE_STRING;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_NAMED_VALUE_STRING_MIN_LEN, MAVLINK_MSG_ID_NAMED_VALUE_STRING_LEN, MAVLINK_MSG_ID_NAMED_VALUE_STRING_CRC);
}

/**
 * @brief Encode a named_value_string struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param named_value_string C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_named_value_string_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_named_value_string_t* named_value_string)
{
    return mavlink_msg_named_value_string_pack(system_id, component_id, msg, named_value_string->time_boot_ms, named_value_string->name, named_value_string->value);
}

/**
 * @brief Encode a named_value_string struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param named_value_string C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_named_value_string_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_named_value_string_t* named_value_string)
{
    return mavlink_msg_named_value_string_pack_chan(system_id, component_id, chan, msg, named_value_string->time_boot_ms, named_value_string->name, named_value_string->value);
}

/**
 * @brief Encode a named_value_string struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param named_value_string C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_named_value_string_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_named_value_string_t* named_value_string)
{
    return mavlink_msg_named_value_string_pack_status(system_id, component_id, _status, msg,  named_value_string->time_boot_ms, named_value_string->name, named_value_string->value);
}

/**
 * @brief Send a named_value_string message
 * @param chan MAVLink channel to send the message
 *
 * @param time_boot_ms [ms] Timestamp (time since system boot).
 * @param name  Name of the debug variable
 * @param value  Value of the debug variable
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_named_value_string_send(mavlink_channel_t chan, uint32_t time_boot_ms, const char *name, const char *value)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_NAMED_VALUE_STRING_LEN];
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_char_array(buf, 4, name, 10);
    _mav_put_char_array(buf, 14, value, 64);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_NAMED_VALUE_STRING, buf, MAVLINK_MSG_ID_NAMED_VALUE_STRING_MIN_LEN, MAVLINK_MSG_ID_NAMED_VALUE_STRING_LEN, MAVLINK_MSG_ID_NAMED_VALUE_STRING_CRC);
#else
    mavlink_named_value_string_t packet;
    packet.time_boot_ms = time_boot_ms;
    mav_array_memcpy(packet.name, name, sizeof(char)*10);
    mav_array_memcpy(packet.value, value, sizeof(char)*64);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_NAMED_VALUE_STRING, (const char *)&packet, MAVLINK_MSG_ID_NAMED_VALUE_STRING_MIN_LEN, MAVLINK_MSG_ID_NAMED_VALUE_STRING_LEN, MAVLINK_MSG_ID_NAMED_VALUE_STRING_CRC);
#endif
}

/**
 * @brief Send a named_value_string message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_named_value_string_send_struct(mavlink_channel_t chan, const mavlink_named_value_string_t* named_value_string)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_named_value_string_send(chan, named_value_string->time_boot_ms, named_value_string->name, named_value_string->value);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_NAMED_VALUE_STRING, (const char *)named_value_string, MAVLINK_MSG_ID_NAMED_VALUE_STRING_MIN_LEN, MAVLINK_MSG_ID_NAMED_VALUE_STRING_LEN, MAVLINK_MSG_ID_NAMED_VALUE_STRING_CRC);
#endif
}

#if MAVLINK_MSG_ID_NAMED_VALUE_STRING_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_named_value_string_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint32_t time_boot_ms, const char *name, const char *value)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint32_t(buf, 0, time_boot_ms);
    _mav_put_char_array(buf, 4, name, 10);
    _mav_put_char_array(buf, 14, value, 64);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_NAMED_VALUE_STRING, buf, MAVLINK_MSG_ID_NAMED_VALUE_STRING_MIN_LEN, MAVLINK_MSG_ID_NAMED_VALUE_STRING_LEN, MAVLINK_MSG_ID_NAMED_VALUE_STRING_CRC);
#else
    mavlink_named_value_string_t *packet = (mavlink_named_value_string_t *)msgbuf;
    packet->time_boot_ms = time_boot_ms;
    mav_array_memcpy(packet->name, name, sizeof(char)*10);
    mav_array_memcpy(packet->value, value, sizeof(char)*64);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_NAMED_VALUE_STRING, (const char *)packet, MAVLINK_MSG_ID_NAMED_VALUE_STRING_MIN_LEN, MAVLINK_MSG_ID_NAMED_VALUE_STRING_LEN, MAVLINK_MSG_ID_NAMED_VALUE_STRING_CRC);
#endif
}
#endif

#endif

// MESSAGE NAMED_VALUE_STRING UNPACKING


/**
 * @brief Get field time_boot_ms from named_value_string message
 *
 * @return [ms] Timestamp (time since system boot).
 */
static inline uint32_t mavlink_msg_named_value_string_get_time_boot_ms(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  0);
}

/**
 * @brief Get field name from named_value_string message
 *
 * @return  Name of the debug variable
 */
static inline uint16_t mavlink_msg_named_value_string_get_name(const mavlink_message_t* msg, char *name)
{
    return _MAV_RETURN_char_array(msg, name, 10,  4);
}

/**
 * @brief Get field value from named_value_string message
 *
 * @return  Value of the debug variable
 */
static inline uint16_t mavlink_msg_named_value_string_get_value(const mavlink_message_t* msg, char *value)
{
    return _MAV_RETURN_char_array(msg, value, 64,  14);
}

/**
 * @brief Decode a named_value_string message into a struct
 *
 * @param msg The message to decode
 * @param named_value_string C-struct to decode the message contents into
 */
static inline void mavlink_msg_named_value_string_decode(const mavlink_message_t* msg, mavlink_named_value_string_t* named_value_string)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    named_value_string->time_boot_ms = mavlink_msg_named_value_string_get_time_boot_ms(msg);
    mavlink_msg_named_value_string_get_name(msg, named_value_string->name);
    mavlink_msg_named_value_string_get_value(msg, named_value_string->value);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_NAMED_VALUE_STRING_LEN? msg->len : MAVLINK_MSG_ID_NAMED_VALUE_STRING_LEN;
        memset(named_value_string, 0, MAVLINK_MSG_ID_NAMED_VALUE_STRING_LEN);
    memcpy(named_value_string, _MAV_PAYLOAD(msg), len);
#endif
}
