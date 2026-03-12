#pragma once
// MESSAGE LED_STRIP_STATE PACKING

#define MAVLINK_MSG_ID_LED_STRIP_STATE 52601


typedef struct __mavlink_led_strip_state_t {
 uint32_t colors[8]; /*<  Array of 32-bit color values (0xWWRRGGBB).*/
 uint8_t length; /*<  How many LEDs are being reported in this message.*/
 uint8_t index; /*<  Index of first LED being reported.*/
 uint8_t id; /*<  Which strip is being reported.*/
 uint8_t following_flight_mode; /*<  Are the LED colors changing according to the flight mode (1) or not (0).*/
} mavlink_led_strip_state_t;

#define MAVLINK_MSG_ID_LED_STRIP_STATE_LEN 36
#define MAVLINK_MSG_ID_LED_STRIP_STATE_MIN_LEN 36
#define MAVLINK_MSG_ID_52601_LEN 36
#define MAVLINK_MSG_ID_52601_MIN_LEN 36

#define MAVLINK_MSG_ID_LED_STRIP_STATE_CRC 102
#define MAVLINK_MSG_ID_52601_CRC 102

#define MAVLINK_MSG_LED_STRIP_STATE_FIELD_COLORS_LEN 8

#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_LED_STRIP_STATE { \
    52601, \
    "LED_STRIP_STATE", \
    5, \
    {  { "length", NULL, MAVLINK_TYPE_UINT8_T, 0, 32, offsetof(mavlink_led_strip_state_t, length) }, \
         { "index", NULL, MAVLINK_TYPE_UINT8_T, 0, 33, offsetof(mavlink_led_strip_state_t, index) }, \
         { "id", NULL, MAVLINK_TYPE_UINT8_T, 0, 34, offsetof(mavlink_led_strip_state_t, id) }, \
         { "following_flight_mode", NULL, MAVLINK_TYPE_UINT8_T, 0, 35, offsetof(mavlink_led_strip_state_t, following_flight_mode) }, \
         { "colors", NULL, MAVLINK_TYPE_UINT32_T, 8, 0, offsetof(mavlink_led_strip_state_t, colors) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_LED_STRIP_STATE { \
    "LED_STRIP_STATE", \
    5, \
    {  { "length", NULL, MAVLINK_TYPE_UINT8_T, 0, 32, offsetof(mavlink_led_strip_state_t, length) }, \
         { "index", NULL, MAVLINK_TYPE_UINT8_T, 0, 33, offsetof(mavlink_led_strip_state_t, index) }, \
         { "id", NULL, MAVLINK_TYPE_UINT8_T, 0, 34, offsetof(mavlink_led_strip_state_t, id) }, \
         { "following_flight_mode", NULL, MAVLINK_TYPE_UINT8_T, 0, 35, offsetof(mavlink_led_strip_state_t, following_flight_mode) }, \
         { "colors", NULL, MAVLINK_TYPE_UINT32_T, 8, 0, offsetof(mavlink_led_strip_state_t, colors) }, \
         } \
}
#endif

/**
 * @brief Pack a led_strip_state message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param length  How many LEDs are being reported in this message.
 * @param index  Index of first LED being reported.
 * @param id  Which strip is being reported.
 * @param following_flight_mode  Are the LED colors changing according to the flight mode (1) or not (0).
 * @param colors  Array of 32-bit color values (0xWWRRGGBB).
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_led_strip_state_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint8_t length, uint8_t index, uint8_t id, uint8_t following_flight_mode, const uint32_t *colors)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_LED_STRIP_STATE_LEN];
    _mav_put_uint8_t(buf, 32, length);
    _mav_put_uint8_t(buf, 33, index);
    _mav_put_uint8_t(buf, 34, id);
    _mav_put_uint8_t(buf, 35, following_flight_mode);
    _mav_put_uint32_t_array(buf, 0, colors, 8);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_LED_STRIP_STATE_LEN);
#else
    mavlink_led_strip_state_t packet;
    packet.length = length;
    packet.index = index;
    packet.id = id;
    packet.following_flight_mode = following_flight_mode;
    mav_array_memcpy(packet.colors, colors, sizeof(uint32_t)*8);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_LED_STRIP_STATE_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_LED_STRIP_STATE;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_LED_STRIP_STATE_MIN_LEN, MAVLINK_MSG_ID_LED_STRIP_STATE_LEN, MAVLINK_MSG_ID_LED_STRIP_STATE_CRC);
}

/**
 * @brief Pack a led_strip_state message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param length  How many LEDs are being reported in this message.
 * @param index  Index of first LED being reported.
 * @param id  Which strip is being reported.
 * @param following_flight_mode  Are the LED colors changing according to the flight mode (1) or not (0).
 * @param colors  Array of 32-bit color values (0xWWRRGGBB).
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_led_strip_state_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint8_t length, uint8_t index, uint8_t id, uint8_t following_flight_mode, const uint32_t *colors)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_LED_STRIP_STATE_LEN];
    _mav_put_uint8_t(buf, 32, length);
    _mav_put_uint8_t(buf, 33, index);
    _mav_put_uint8_t(buf, 34, id);
    _mav_put_uint8_t(buf, 35, following_flight_mode);
    _mav_put_uint32_t_array(buf, 0, colors, 8);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_LED_STRIP_STATE_LEN);
#else
    mavlink_led_strip_state_t packet;
    packet.length = length;
    packet.index = index;
    packet.id = id;
    packet.following_flight_mode = following_flight_mode;
    mav_array_memcpy(packet.colors, colors, sizeof(uint32_t)*8);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_LED_STRIP_STATE_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_LED_STRIP_STATE;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_LED_STRIP_STATE_MIN_LEN, MAVLINK_MSG_ID_LED_STRIP_STATE_LEN, MAVLINK_MSG_ID_LED_STRIP_STATE_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_LED_STRIP_STATE_MIN_LEN, MAVLINK_MSG_ID_LED_STRIP_STATE_LEN);
#endif
}

/**
 * @brief Pack a led_strip_state message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param length  How many LEDs are being reported in this message.
 * @param index  Index of first LED being reported.
 * @param id  Which strip is being reported.
 * @param following_flight_mode  Are the LED colors changing according to the flight mode (1) or not (0).
 * @param colors  Array of 32-bit color values (0xWWRRGGBB).
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_led_strip_state_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint8_t length,uint8_t index,uint8_t id,uint8_t following_flight_mode,const uint32_t *colors)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_LED_STRIP_STATE_LEN];
    _mav_put_uint8_t(buf, 32, length);
    _mav_put_uint8_t(buf, 33, index);
    _mav_put_uint8_t(buf, 34, id);
    _mav_put_uint8_t(buf, 35, following_flight_mode);
    _mav_put_uint32_t_array(buf, 0, colors, 8);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_LED_STRIP_STATE_LEN);
#else
    mavlink_led_strip_state_t packet;
    packet.length = length;
    packet.index = index;
    packet.id = id;
    packet.following_flight_mode = following_flight_mode;
    mav_array_memcpy(packet.colors, colors, sizeof(uint32_t)*8);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_LED_STRIP_STATE_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_LED_STRIP_STATE;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_LED_STRIP_STATE_MIN_LEN, MAVLINK_MSG_ID_LED_STRIP_STATE_LEN, MAVLINK_MSG_ID_LED_STRIP_STATE_CRC);
}

/**
 * @brief Encode a led_strip_state struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param led_strip_state C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_led_strip_state_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_led_strip_state_t* led_strip_state)
{
    return mavlink_msg_led_strip_state_pack(system_id, component_id, msg, led_strip_state->length, led_strip_state->index, led_strip_state->id, led_strip_state->following_flight_mode, led_strip_state->colors);
}

/**
 * @brief Encode a led_strip_state struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param led_strip_state C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_led_strip_state_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_led_strip_state_t* led_strip_state)
{
    return mavlink_msg_led_strip_state_pack_chan(system_id, component_id, chan, msg, led_strip_state->length, led_strip_state->index, led_strip_state->id, led_strip_state->following_flight_mode, led_strip_state->colors);
}

/**
 * @brief Encode a led_strip_state struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param led_strip_state C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_led_strip_state_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_led_strip_state_t* led_strip_state)
{
    return mavlink_msg_led_strip_state_pack_status(system_id, component_id, _status, msg,  led_strip_state->length, led_strip_state->index, led_strip_state->id, led_strip_state->following_flight_mode, led_strip_state->colors);
}

/**
 * @brief Send a led_strip_state message
 * @param chan MAVLink channel to send the message
 *
 * @param length  How many LEDs are being reported in this message.
 * @param index  Index of first LED being reported.
 * @param id  Which strip is being reported.
 * @param following_flight_mode  Are the LED colors changing according to the flight mode (1) or not (0).
 * @param colors  Array of 32-bit color values (0xWWRRGGBB).
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_led_strip_state_send(mavlink_channel_t chan, uint8_t length, uint8_t index, uint8_t id, uint8_t following_flight_mode, const uint32_t *colors)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_LED_STRIP_STATE_LEN];
    _mav_put_uint8_t(buf, 32, length);
    _mav_put_uint8_t(buf, 33, index);
    _mav_put_uint8_t(buf, 34, id);
    _mav_put_uint8_t(buf, 35, following_flight_mode);
    _mav_put_uint32_t_array(buf, 0, colors, 8);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_LED_STRIP_STATE, buf, MAVLINK_MSG_ID_LED_STRIP_STATE_MIN_LEN, MAVLINK_MSG_ID_LED_STRIP_STATE_LEN, MAVLINK_MSG_ID_LED_STRIP_STATE_CRC);
#else
    mavlink_led_strip_state_t packet;
    packet.length = length;
    packet.index = index;
    packet.id = id;
    packet.following_flight_mode = following_flight_mode;
    mav_array_memcpy(packet.colors, colors, sizeof(uint32_t)*8);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_LED_STRIP_STATE, (const char *)&packet, MAVLINK_MSG_ID_LED_STRIP_STATE_MIN_LEN, MAVLINK_MSG_ID_LED_STRIP_STATE_LEN, MAVLINK_MSG_ID_LED_STRIP_STATE_CRC);
#endif
}

/**
 * @brief Send a led_strip_state message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_led_strip_state_send_struct(mavlink_channel_t chan, const mavlink_led_strip_state_t* led_strip_state)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_led_strip_state_send(chan, led_strip_state->length, led_strip_state->index, led_strip_state->id, led_strip_state->following_flight_mode, led_strip_state->colors);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_LED_STRIP_STATE, (const char *)led_strip_state, MAVLINK_MSG_ID_LED_STRIP_STATE_MIN_LEN, MAVLINK_MSG_ID_LED_STRIP_STATE_LEN, MAVLINK_MSG_ID_LED_STRIP_STATE_CRC);
#endif
}

#if MAVLINK_MSG_ID_LED_STRIP_STATE_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_led_strip_state_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint8_t length, uint8_t index, uint8_t id, uint8_t following_flight_mode, const uint32_t *colors)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint8_t(buf, 32, length);
    _mav_put_uint8_t(buf, 33, index);
    _mav_put_uint8_t(buf, 34, id);
    _mav_put_uint8_t(buf, 35, following_flight_mode);
    _mav_put_uint32_t_array(buf, 0, colors, 8);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_LED_STRIP_STATE, buf, MAVLINK_MSG_ID_LED_STRIP_STATE_MIN_LEN, MAVLINK_MSG_ID_LED_STRIP_STATE_LEN, MAVLINK_MSG_ID_LED_STRIP_STATE_CRC);
#else
    mavlink_led_strip_state_t *packet = (mavlink_led_strip_state_t *)msgbuf;
    packet->length = length;
    packet->index = index;
    packet->id = id;
    packet->following_flight_mode = following_flight_mode;
    mav_array_memcpy(packet->colors, colors, sizeof(uint32_t)*8);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_LED_STRIP_STATE, (const char *)packet, MAVLINK_MSG_ID_LED_STRIP_STATE_MIN_LEN, MAVLINK_MSG_ID_LED_STRIP_STATE_LEN, MAVLINK_MSG_ID_LED_STRIP_STATE_CRC);
#endif
}
#endif

#endif

// MESSAGE LED_STRIP_STATE UNPACKING


/**
 * @brief Get field length from led_strip_state message
 *
 * @return  How many LEDs are being reported in this message.
 */
static inline uint8_t mavlink_msg_led_strip_state_get_length(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  32);
}

/**
 * @brief Get field index from led_strip_state message
 *
 * @return  Index of first LED being reported.
 */
static inline uint8_t mavlink_msg_led_strip_state_get_index(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  33);
}

/**
 * @brief Get field id from led_strip_state message
 *
 * @return  Which strip is being reported.
 */
static inline uint8_t mavlink_msg_led_strip_state_get_id(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  34);
}

/**
 * @brief Get field following_flight_mode from led_strip_state message
 *
 * @return  Are the LED colors changing according to the flight mode (1) or not (0).
 */
static inline uint8_t mavlink_msg_led_strip_state_get_following_flight_mode(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  35);
}

/**
 * @brief Get field colors from led_strip_state message
 *
 * @return  Array of 32-bit color values (0xWWRRGGBB).
 */
static inline uint16_t mavlink_msg_led_strip_state_get_colors(const mavlink_message_t* msg, uint32_t *colors)
{
    return _MAV_RETURN_uint32_t_array(msg, colors, 8,  0);
}

/**
 * @brief Decode a led_strip_state message into a struct
 *
 * @param msg The message to decode
 * @param led_strip_state C-struct to decode the message contents into
 */
static inline void mavlink_msg_led_strip_state_decode(const mavlink_message_t* msg, mavlink_led_strip_state_t* led_strip_state)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_led_strip_state_get_colors(msg, led_strip_state->colors);
    led_strip_state->length = mavlink_msg_led_strip_state_get_length(msg);
    led_strip_state->index = mavlink_msg_led_strip_state_get_index(msg);
    led_strip_state->id = mavlink_msg_led_strip_state_get_id(msg);
    led_strip_state->following_flight_mode = mavlink_msg_led_strip_state_get_following_flight_mode(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_LED_STRIP_STATE_LEN? msg->len : MAVLINK_MSG_ID_LED_STRIP_STATE_LEN;
        memset(led_strip_state, 0, MAVLINK_MSG_ID_LED_STRIP_STATE_LEN);
    memcpy(led_strip_state, _MAV_PAYLOAD(msg), len);
#endif
}
