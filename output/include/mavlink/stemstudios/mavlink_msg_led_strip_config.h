#pragma once
// MESSAGE LED_STRIP_CONFIG PACKING

#define MAVLINK_MSG_ID_LED_STRIP_CONFIG 52600


typedef struct __mavlink_led_strip_config_t {
 uint32_t colors[8]; /*<  Array of 32-bit color values (0xWWRRGGBB).*/
 uint8_t target_system; /*<  System ID.*/
 uint8_t target_component; /*<  Component ID (Normally 134 for an LED Strip Controller).*/
 uint8_t mode; /*<  How to configure LEDs.*/
 uint8_t index; /*<  Set LEDs starting from this index.*/
 uint8_t length; /*<  The number of LEDs to set (up to 8).*/
 uint8_t id; /*<  Which strip to configure. UINT8_MAX for all strips.*/
} mavlink_led_strip_config_t;

#define MAVLINK_MSG_ID_LED_STRIP_CONFIG_LEN 38
#define MAVLINK_MSG_ID_LED_STRIP_CONFIG_MIN_LEN 38
#define MAVLINK_MSG_ID_52600_LEN 38
#define MAVLINK_MSG_ID_52600_MIN_LEN 38

#define MAVLINK_MSG_ID_LED_STRIP_CONFIG_CRC 181
#define MAVLINK_MSG_ID_52600_CRC 181

#define MAVLINK_MSG_LED_STRIP_CONFIG_FIELD_COLORS_LEN 8

#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_LED_STRIP_CONFIG { \
    52600, \
    "LED_STRIP_CONFIG", \
    7, \
    {  { "target_system", NULL, MAVLINK_TYPE_UINT8_T, 0, 32, offsetof(mavlink_led_strip_config_t, target_system) }, \
         { "target_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 33, offsetof(mavlink_led_strip_config_t, target_component) }, \
         { "mode", NULL, MAVLINK_TYPE_UINT8_T, 0, 34, offsetof(mavlink_led_strip_config_t, mode) }, \
         { "index", NULL, MAVLINK_TYPE_UINT8_T, 0, 35, offsetof(mavlink_led_strip_config_t, index) }, \
         { "length", NULL, MAVLINK_TYPE_UINT8_T, 0, 36, offsetof(mavlink_led_strip_config_t, length) }, \
         { "id", NULL, MAVLINK_TYPE_UINT8_T, 0, 37, offsetof(mavlink_led_strip_config_t, id) }, \
         { "colors", NULL, MAVLINK_TYPE_UINT32_T, 8, 0, offsetof(mavlink_led_strip_config_t, colors) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_LED_STRIP_CONFIG { \
    "LED_STRIP_CONFIG", \
    7, \
    {  { "target_system", NULL, MAVLINK_TYPE_UINT8_T, 0, 32, offsetof(mavlink_led_strip_config_t, target_system) }, \
         { "target_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 33, offsetof(mavlink_led_strip_config_t, target_component) }, \
         { "mode", NULL, MAVLINK_TYPE_UINT8_T, 0, 34, offsetof(mavlink_led_strip_config_t, mode) }, \
         { "index", NULL, MAVLINK_TYPE_UINT8_T, 0, 35, offsetof(mavlink_led_strip_config_t, index) }, \
         { "length", NULL, MAVLINK_TYPE_UINT8_T, 0, 36, offsetof(mavlink_led_strip_config_t, length) }, \
         { "id", NULL, MAVLINK_TYPE_UINT8_T, 0, 37, offsetof(mavlink_led_strip_config_t, id) }, \
         { "colors", NULL, MAVLINK_TYPE_UINT32_T, 8, 0, offsetof(mavlink_led_strip_config_t, colors) }, \
         } \
}
#endif

/**
 * @brief Pack a led_strip_config message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system  System ID.
 * @param target_component  Component ID (Normally 134 for an LED Strip Controller).
 * @param mode  How to configure LEDs.
 * @param index  Set LEDs starting from this index.
 * @param length  The number of LEDs to set (up to 8).
 * @param id  Which strip to configure. UINT8_MAX for all strips.
 * @param colors  Array of 32-bit color values (0xWWRRGGBB).
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_led_strip_config_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint8_t target_system, uint8_t target_component, uint8_t mode, uint8_t index, uint8_t length, uint8_t id, const uint32_t *colors)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_LED_STRIP_CONFIG_LEN];
    _mav_put_uint8_t(buf, 32, target_system);
    _mav_put_uint8_t(buf, 33, target_component);
    _mav_put_uint8_t(buf, 34, mode);
    _mav_put_uint8_t(buf, 35, index);
    _mav_put_uint8_t(buf, 36, length);
    _mav_put_uint8_t(buf, 37, id);
    _mav_put_uint32_t_array(buf, 0, colors, 8);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_LED_STRIP_CONFIG_LEN);
#else
    mavlink_led_strip_config_t packet;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.mode = mode;
    packet.index = index;
    packet.length = length;
    packet.id = id;
    mav_array_memcpy(packet.colors, colors, sizeof(uint32_t)*8);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_LED_STRIP_CONFIG_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_LED_STRIP_CONFIG;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_LED_STRIP_CONFIG_MIN_LEN, MAVLINK_MSG_ID_LED_STRIP_CONFIG_LEN, MAVLINK_MSG_ID_LED_STRIP_CONFIG_CRC);
}

/**
 * @brief Pack a led_strip_config message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system  System ID.
 * @param target_component  Component ID (Normally 134 for an LED Strip Controller).
 * @param mode  How to configure LEDs.
 * @param index  Set LEDs starting from this index.
 * @param length  The number of LEDs to set (up to 8).
 * @param id  Which strip to configure. UINT8_MAX for all strips.
 * @param colors  Array of 32-bit color values (0xWWRRGGBB).
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_led_strip_config_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint8_t target_system, uint8_t target_component, uint8_t mode, uint8_t index, uint8_t length, uint8_t id, const uint32_t *colors)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_LED_STRIP_CONFIG_LEN];
    _mav_put_uint8_t(buf, 32, target_system);
    _mav_put_uint8_t(buf, 33, target_component);
    _mav_put_uint8_t(buf, 34, mode);
    _mav_put_uint8_t(buf, 35, index);
    _mav_put_uint8_t(buf, 36, length);
    _mav_put_uint8_t(buf, 37, id);
    _mav_put_uint32_t_array(buf, 0, colors, 8);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_LED_STRIP_CONFIG_LEN);
#else
    mavlink_led_strip_config_t packet;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.mode = mode;
    packet.index = index;
    packet.length = length;
    packet.id = id;
    mav_array_memcpy(packet.colors, colors, sizeof(uint32_t)*8);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_LED_STRIP_CONFIG_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_LED_STRIP_CONFIG;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_LED_STRIP_CONFIG_MIN_LEN, MAVLINK_MSG_ID_LED_STRIP_CONFIG_LEN, MAVLINK_MSG_ID_LED_STRIP_CONFIG_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_LED_STRIP_CONFIG_MIN_LEN, MAVLINK_MSG_ID_LED_STRIP_CONFIG_LEN);
#endif
}

/**
 * @brief Pack a led_strip_config message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system  System ID.
 * @param target_component  Component ID (Normally 134 for an LED Strip Controller).
 * @param mode  How to configure LEDs.
 * @param index  Set LEDs starting from this index.
 * @param length  The number of LEDs to set (up to 8).
 * @param id  Which strip to configure. UINT8_MAX for all strips.
 * @param colors  Array of 32-bit color values (0xWWRRGGBB).
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_led_strip_config_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint8_t target_system,uint8_t target_component,uint8_t mode,uint8_t index,uint8_t length,uint8_t id,const uint32_t *colors)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_LED_STRIP_CONFIG_LEN];
    _mav_put_uint8_t(buf, 32, target_system);
    _mav_put_uint8_t(buf, 33, target_component);
    _mav_put_uint8_t(buf, 34, mode);
    _mav_put_uint8_t(buf, 35, index);
    _mav_put_uint8_t(buf, 36, length);
    _mav_put_uint8_t(buf, 37, id);
    _mav_put_uint32_t_array(buf, 0, colors, 8);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_LED_STRIP_CONFIG_LEN);
#else
    mavlink_led_strip_config_t packet;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.mode = mode;
    packet.index = index;
    packet.length = length;
    packet.id = id;
    mav_array_memcpy(packet.colors, colors, sizeof(uint32_t)*8);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_LED_STRIP_CONFIG_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_LED_STRIP_CONFIG;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_LED_STRIP_CONFIG_MIN_LEN, MAVLINK_MSG_ID_LED_STRIP_CONFIG_LEN, MAVLINK_MSG_ID_LED_STRIP_CONFIG_CRC);
}

/**
 * @brief Encode a led_strip_config struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param led_strip_config C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_led_strip_config_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_led_strip_config_t* led_strip_config)
{
    return mavlink_msg_led_strip_config_pack(system_id, component_id, msg, led_strip_config->target_system, led_strip_config->target_component, led_strip_config->mode, led_strip_config->index, led_strip_config->length, led_strip_config->id, led_strip_config->colors);
}

/**
 * @brief Encode a led_strip_config struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param led_strip_config C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_led_strip_config_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_led_strip_config_t* led_strip_config)
{
    return mavlink_msg_led_strip_config_pack_chan(system_id, component_id, chan, msg, led_strip_config->target_system, led_strip_config->target_component, led_strip_config->mode, led_strip_config->index, led_strip_config->length, led_strip_config->id, led_strip_config->colors);
}

/**
 * @brief Encode a led_strip_config struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param led_strip_config C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_led_strip_config_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_led_strip_config_t* led_strip_config)
{
    return mavlink_msg_led_strip_config_pack_status(system_id, component_id, _status, msg,  led_strip_config->target_system, led_strip_config->target_component, led_strip_config->mode, led_strip_config->index, led_strip_config->length, led_strip_config->id, led_strip_config->colors);
}

/**
 * @brief Send a led_strip_config message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system  System ID.
 * @param target_component  Component ID (Normally 134 for an LED Strip Controller).
 * @param mode  How to configure LEDs.
 * @param index  Set LEDs starting from this index.
 * @param length  The number of LEDs to set (up to 8).
 * @param id  Which strip to configure. UINT8_MAX for all strips.
 * @param colors  Array of 32-bit color values (0xWWRRGGBB).
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_led_strip_config_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, uint8_t mode, uint8_t index, uint8_t length, uint8_t id, const uint32_t *colors)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_LED_STRIP_CONFIG_LEN];
    _mav_put_uint8_t(buf, 32, target_system);
    _mav_put_uint8_t(buf, 33, target_component);
    _mav_put_uint8_t(buf, 34, mode);
    _mav_put_uint8_t(buf, 35, index);
    _mav_put_uint8_t(buf, 36, length);
    _mav_put_uint8_t(buf, 37, id);
    _mav_put_uint32_t_array(buf, 0, colors, 8);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_LED_STRIP_CONFIG, buf, MAVLINK_MSG_ID_LED_STRIP_CONFIG_MIN_LEN, MAVLINK_MSG_ID_LED_STRIP_CONFIG_LEN, MAVLINK_MSG_ID_LED_STRIP_CONFIG_CRC);
#else
    mavlink_led_strip_config_t packet;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.mode = mode;
    packet.index = index;
    packet.length = length;
    packet.id = id;
    mav_array_memcpy(packet.colors, colors, sizeof(uint32_t)*8);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_LED_STRIP_CONFIG, (const char *)&packet, MAVLINK_MSG_ID_LED_STRIP_CONFIG_MIN_LEN, MAVLINK_MSG_ID_LED_STRIP_CONFIG_LEN, MAVLINK_MSG_ID_LED_STRIP_CONFIG_CRC);
#endif
}

/**
 * @brief Send a led_strip_config message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_led_strip_config_send_struct(mavlink_channel_t chan, const mavlink_led_strip_config_t* led_strip_config)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_led_strip_config_send(chan, led_strip_config->target_system, led_strip_config->target_component, led_strip_config->mode, led_strip_config->index, led_strip_config->length, led_strip_config->id, led_strip_config->colors);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_LED_STRIP_CONFIG, (const char *)led_strip_config, MAVLINK_MSG_ID_LED_STRIP_CONFIG_MIN_LEN, MAVLINK_MSG_ID_LED_STRIP_CONFIG_LEN, MAVLINK_MSG_ID_LED_STRIP_CONFIG_CRC);
#endif
}

#if MAVLINK_MSG_ID_LED_STRIP_CONFIG_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_led_strip_config_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint8_t target_system, uint8_t target_component, uint8_t mode, uint8_t index, uint8_t length, uint8_t id, const uint32_t *colors)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint8_t(buf, 32, target_system);
    _mav_put_uint8_t(buf, 33, target_component);
    _mav_put_uint8_t(buf, 34, mode);
    _mav_put_uint8_t(buf, 35, index);
    _mav_put_uint8_t(buf, 36, length);
    _mav_put_uint8_t(buf, 37, id);
    _mav_put_uint32_t_array(buf, 0, colors, 8);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_LED_STRIP_CONFIG, buf, MAVLINK_MSG_ID_LED_STRIP_CONFIG_MIN_LEN, MAVLINK_MSG_ID_LED_STRIP_CONFIG_LEN, MAVLINK_MSG_ID_LED_STRIP_CONFIG_CRC);
#else
    mavlink_led_strip_config_t *packet = (mavlink_led_strip_config_t *)msgbuf;
    packet->target_system = target_system;
    packet->target_component = target_component;
    packet->mode = mode;
    packet->index = index;
    packet->length = length;
    packet->id = id;
    mav_array_memcpy(packet->colors, colors, sizeof(uint32_t)*8);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_LED_STRIP_CONFIG, (const char *)packet, MAVLINK_MSG_ID_LED_STRIP_CONFIG_MIN_LEN, MAVLINK_MSG_ID_LED_STRIP_CONFIG_LEN, MAVLINK_MSG_ID_LED_STRIP_CONFIG_CRC);
#endif
}
#endif

#endif

// MESSAGE LED_STRIP_CONFIG UNPACKING


/**
 * @brief Get field target_system from led_strip_config message
 *
 * @return  System ID.
 */
static inline uint8_t mavlink_msg_led_strip_config_get_target_system(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  32);
}

/**
 * @brief Get field target_component from led_strip_config message
 *
 * @return  Component ID (Normally 134 for an LED Strip Controller).
 */
static inline uint8_t mavlink_msg_led_strip_config_get_target_component(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  33);
}

/**
 * @brief Get field mode from led_strip_config message
 *
 * @return  How to configure LEDs.
 */
static inline uint8_t mavlink_msg_led_strip_config_get_mode(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  34);
}

/**
 * @brief Get field index from led_strip_config message
 *
 * @return  Set LEDs starting from this index.
 */
static inline uint8_t mavlink_msg_led_strip_config_get_index(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  35);
}

/**
 * @brief Get field length from led_strip_config message
 *
 * @return  The number of LEDs to set (up to 8).
 */
static inline uint8_t mavlink_msg_led_strip_config_get_length(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  36);
}

/**
 * @brief Get field id from led_strip_config message
 *
 * @return  Which strip to configure. UINT8_MAX for all strips.
 */
static inline uint8_t mavlink_msg_led_strip_config_get_id(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  37);
}

/**
 * @brief Get field colors from led_strip_config message
 *
 * @return  Array of 32-bit color values (0xWWRRGGBB).
 */
static inline uint16_t mavlink_msg_led_strip_config_get_colors(const mavlink_message_t* msg, uint32_t *colors)
{
    return _MAV_RETURN_uint32_t_array(msg, colors, 8,  0);
}

/**
 * @brief Decode a led_strip_config message into a struct
 *
 * @param msg The message to decode
 * @param led_strip_config C-struct to decode the message contents into
 */
static inline void mavlink_msg_led_strip_config_decode(const mavlink_message_t* msg, mavlink_led_strip_config_t* led_strip_config)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_led_strip_config_get_colors(msg, led_strip_config->colors);
    led_strip_config->target_system = mavlink_msg_led_strip_config_get_target_system(msg);
    led_strip_config->target_component = mavlink_msg_led_strip_config_get_target_component(msg);
    led_strip_config->mode = mavlink_msg_led_strip_config_get_mode(msg);
    led_strip_config->index = mavlink_msg_led_strip_config_get_index(msg);
    led_strip_config->length = mavlink_msg_led_strip_config_get_length(msg);
    led_strip_config->id = mavlink_msg_led_strip_config_get_id(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_LED_STRIP_CONFIG_LEN? msg->len : MAVLINK_MSG_ID_LED_STRIP_CONFIG_LEN;
        memset(led_strip_config, 0, MAVLINK_MSG_ID_LED_STRIP_CONFIG_LEN);
    memcpy(led_strip_config, _MAV_PAYLOAD(msg), len);
#endif
}
