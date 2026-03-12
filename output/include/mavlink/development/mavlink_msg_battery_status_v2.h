#pragma once
// MESSAGE BATTERY_STATUS_V2 PACKING

#define MAVLINK_MSG_ID_BATTERY_STATUS_V2 369


typedef struct __mavlink_battery_status_v2_t {
 float voltage; /*< [V] Battery voltage (total). NaN: field not provided.*/
 float current; /*< [A] Battery current (through all cells/loads). Positive value when discharging and negative if charging. NaN: field not provided.*/
 float capacity_consumed; /*< [Ah] Consumed charge. NaN: field not provided. This is either the consumption since power-on or since the battery was full, depending on the value of MAV_BATTERY_STATUS_FLAGS_CAPACITY_RELATIVE_TO_FULL.*/
 float capacity_remaining; /*< [Ah] Remaining charge (until empty). NaN: field not provided. Note: If MAV_BATTERY_STATUS_FLAGS_CAPACITY_RELATIVE_TO_FULL is unset, this value is based on the assumption the battery was full when the system was powered.*/
 uint32_t status_flags; /*<  Fault, health, readiness, and other status indications.*/
 int16_t temperature; /*< [cdegC] Temperature of the whole battery pack (not internal electronics). INT16_MAX field not provided.*/
 uint8_t id; /*<  Battery ID*/
 uint8_t percent_remaining; /*< [%] Remaining battery energy. Values: [0-100], UINT8_MAX: field not provided.*/
} mavlink_battery_status_v2_t;

#define MAVLINK_MSG_ID_BATTERY_STATUS_V2_LEN 24
#define MAVLINK_MSG_ID_BATTERY_STATUS_V2_MIN_LEN 24
#define MAVLINK_MSG_ID_369_LEN 24
#define MAVLINK_MSG_ID_369_MIN_LEN 24

#define MAVLINK_MSG_ID_BATTERY_STATUS_V2_CRC 151
#define MAVLINK_MSG_ID_369_CRC 151



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_BATTERY_STATUS_V2 { \
    369, \
    "BATTERY_STATUS_V2", \
    8, \
    {  { "id", NULL, MAVLINK_TYPE_UINT8_T, 0, 22, offsetof(mavlink_battery_status_v2_t, id) }, \
         { "temperature", NULL, MAVLINK_TYPE_INT16_T, 0, 20, offsetof(mavlink_battery_status_v2_t, temperature) }, \
         { "voltage", NULL, MAVLINK_TYPE_FLOAT, 0, 0, offsetof(mavlink_battery_status_v2_t, voltage) }, \
         { "current", NULL, MAVLINK_TYPE_FLOAT, 0, 4, offsetof(mavlink_battery_status_v2_t, current) }, \
         { "capacity_consumed", NULL, MAVLINK_TYPE_FLOAT, 0, 8, offsetof(mavlink_battery_status_v2_t, capacity_consumed) }, \
         { "capacity_remaining", NULL, MAVLINK_TYPE_FLOAT, 0, 12, offsetof(mavlink_battery_status_v2_t, capacity_remaining) }, \
         { "percent_remaining", NULL, MAVLINK_TYPE_UINT8_T, 0, 23, offsetof(mavlink_battery_status_v2_t, percent_remaining) }, \
         { "status_flags", NULL, MAVLINK_TYPE_UINT32_T, 0, 16, offsetof(mavlink_battery_status_v2_t, status_flags) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_BATTERY_STATUS_V2 { \
    "BATTERY_STATUS_V2", \
    8, \
    {  { "id", NULL, MAVLINK_TYPE_UINT8_T, 0, 22, offsetof(mavlink_battery_status_v2_t, id) }, \
         { "temperature", NULL, MAVLINK_TYPE_INT16_T, 0, 20, offsetof(mavlink_battery_status_v2_t, temperature) }, \
         { "voltage", NULL, MAVLINK_TYPE_FLOAT, 0, 0, offsetof(mavlink_battery_status_v2_t, voltage) }, \
         { "current", NULL, MAVLINK_TYPE_FLOAT, 0, 4, offsetof(mavlink_battery_status_v2_t, current) }, \
         { "capacity_consumed", NULL, MAVLINK_TYPE_FLOAT, 0, 8, offsetof(mavlink_battery_status_v2_t, capacity_consumed) }, \
         { "capacity_remaining", NULL, MAVLINK_TYPE_FLOAT, 0, 12, offsetof(mavlink_battery_status_v2_t, capacity_remaining) }, \
         { "percent_remaining", NULL, MAVLINK_TYPE_UINT8_T, 0, 23, offsetof(mavlink_battery_status_v2_t, percent_remaining) }, \
         { "status_flags", NULL, MAVLINK_TYPE_UINT32_T, 0, 16, offsetof(mavlink_battery_status_v2_t, status_flags) }, \
         } \
}
#endif

/**
 * @brief Pack a battery_status_v2 message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param id  Battery ID
 * @param temperature [cdegC] Temperature of the whole battery pack (not internal electronics). INT16_MAX field not provided.
 * @param voltage [V] Battery voltage (total). NaN: field not provided.
 * @param current [A] Battery current (through all cells/loads). Positive value when discharging and negative if charging. NaN: field not provided.
 * @param capacity_consumed [Ah] Consumed charge. NaN: field not provided. This is either the consumption since power-on or since the battery was full, depending on the value of MAV_BATTERY_STATUS_FLAGS_CAPACITY_RELATIVE_TO_FULL.
 * @param capacity_remaining [Ah] Remaining charge (until empty). NaN: field not provided. Note: If MAV_BATTERY_STATUS_FLAGS_CAPACITY_RELATIVE_TO_FULL is unset, this value is based on the assumption the battery was full when the system was powered.
 * @param percent_remaining [%] Remaining battery energy. Values: [0-100], UINT8_MAX: field not provided.
 * @param status_flags  Fault, health, readiness, and other status indications.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_battery_status_v2_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint8_t id, int16_t temperature, float voltage, float current, float capacity_consumed, float capacity_remaining, uint8_t percent_remaining, uint32_t status_flags)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_BATTERY_STATUS_V2_LEN];
    _mav_put_float(buf, 0, voltage);
    _mav_put_float(buf, 4, current);
    _mav_put_float(buf, 8, capacity_consumed);
    _mav_put_float(buf, 12, capacity_remaining);
    _mav_put_uint32_t(buf, 16, status_flags);
    _mav_put_int16_t(buf, 20, temperature);
    _mav_put_uint8_t(buf, 22, id);
    _mav_put_uint8_t(buf, 23, percent_remaining);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_BATTERY_STATUS_V2_LEN);
#else
    mavlink_battery_status_v2_t packet;
    packet.voltage = voltage;
    packet.current = current;
    packet.capacity_consumed = capacity_consumed;
    packet.capacity_remaining = capacity_remaining;
    packet.status_flags = status_flags;
    packet.temperature = temperature;
    packet.id = id;
    packet.percent_remaining = percent_remaining;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_BATTERY_STATUS_V2_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_BATTERY_STATUS_V2;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_BATTERY_STATUS_V2_MIN_LEN, MAVLINK_MSG_ID_BATTERY_STATUS_V2_LEN, MAVLINK_MSG_ID_BATTERY_STATUS_V2_CRC);
}

/**
 * @brief Pack a battery_status_v2 message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param id  Battery ID
 * @param temperature [cdegC] Temperature of the whole battery pack (not internal electronics). INT16_MAX field not provided.
 * @param voltage [V] Battery voltage (total). NaN: field not provided.
 * @param current [A] Battery current (through all cells/loads). Positive value when discharging and negative if charging. NaN: field not provided.
 * @param capacity_consumed [Ah] Consumed charge. NaN: field not provided. This is either the consumption since power-on or since the battery was full, depending on the value of MAV_BATTERY_STATUS_FLAGS_CAPACITY_RELATIVE_TO_FULL.
 * @param capacity_remaining [Ah] Remaining charge (until empty). NaN: field not provided. Note: If MAV_BATTERY_STATUS_FLAGS_CAPACITY_RELATIVE_TO_FULL is unset, this value is based on the assumption the battery was full when the system was powered.
 * @param percent_remaining [%] Remaining battery energy. Values: [0-100], UINT8_MAX: field not provided.
 * @param status_flags  Fault, health, readiness, and other status indications.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_battery_status_v2_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint8_t id, int16_t temperature, float voltage, float current, float capacity_consumed, float capacity_remaining, uint8_t percent_remaining, uint32_t status_flags)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_BATTERY_STATUS_V2_LEN];
    _mav_put_float(buf, 0, voltage);
    _mav_put_float(buf, 4, current);
    _mav_put_float(buf, 8, capacity_consumed);
    _mav_put_float(buf, 12, capacity_remaining);
    _mav_put_uint32_t(buf, 16, status_flags);
    _mav_put_int16_t(buf, 20, temperature);
    _mav_put_uint8_t(buf, 22, id);
    _mav_put_uint8_t(buf, 23, percent_remaining);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_BATTERY_STATUS_V2_LEN);
#else
    mavlink_battery_status_v2_t packet;
    packet.voltage = voltage;
    packet.current = current;
    packet.capacity_consumed = capacity_consumed;
    packet.capacity_remaining = capacity_remaining;
    packet.status_flags = status_flags;
    packet.temperature = temperature;
    packet.id = id;
    packet.percent_remaining = percent_remaining;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_BATTERY_STATUS_V2_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_BATTERY_STATUS_V2;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_BATTERY_STATUS_V2_MIN_LEN, MAVLINK_MSG_ID_BATTERY_STATUS_V2_LEN, MAVLINK_MSG_ID_BATTERY_STATUS_V2_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_BATTERY_STATUS_V2_MIN_LEN, MAVLINK_MSG_ID_BATTERY_STATUS_V2_LEN);
#endif
}

/**
 * @brief Pack a battery_status_v2 message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param id  Battery ID
 * @param temperature [cdegC] Temperature of the whole battery pack (not internal electronics). INT16_MAX field not provided.
 * @param voltage [V] Battery voltage (total). NaN: field not provided.
 * @param current [A] Battery current (through all cells/loads). Positive value when discharging and negative if charging. NaN: field not provided.
 * @param capacity_consumed [Ah] Consumed charge. NaN: field not provided. This is either the consumption since power-on or since the battery was full, depending on the value of MAV_BATTERY_STATUS_FLAGS_CAPACITY_RELATIVE_TO_FULL.
 * @param capacity_remaining [Ah] Remaining charge (until empty). NaN: field not provided. Note: If MAV_BATTERY_STATUS_FLAGS_CAPACITY_RELATIVE_TO_FULL is unset, this value is based on the assumption the battery was full when the system was powered.
 * @param percent_remaining [%] Remaining battery energy. Values: [0-100], UINT8_MAX: field not provided.
 * @param status_flags  Fault, health, readiness, and other status indications.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_battery_status_v2_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint8_t id,int16_t temperature,float voltage,float current,float capacity_consumed,float capacity_remaining,uint8_t percent_remaining,uint32_t status_flags)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_BATTERY_STATUS_V2_LEN];
    _mav_put_float(buf, 0, voltage);
    _mav_put_float(buf, 4, current);
    _mav_put_float(buf, 8, capacity_consumed);
    _mav_put_float(buf, 12, capacity_remaining);
    _mav_put_uint32_t(buf, 16, status_flags);
    _mav_put_int16_t(buf, 20, temperature);
    _mav_put_uint8_t(buf, 22, id);
    _mav_put_uint8_t(buf, 23, percent_remaining);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_BATTERY_STATUS_V2_LEN);
#else
    mavlink_battery_status_v2_t packet;
    packet.voltage = voltage;
    packet.current = current;
    packet.capacity_consumed = capacity_consumed;
    packet.capacity_remaining = capacity_remaining;
    packet.status_flags = status_flags;
    packet.temperature = temperature;
    packet.id = id;
    packet.percent_remaining = percent_remaining;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_BATTERY_STATUS_V2_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_BATTERY_STATUS_V2;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_BATTERY_STATUS_V2_MIN_LEN, MAVLINK_MSG_ID_BATTERY_STATUS_V2_LEN, MAVLINK_MSG_ID_BATTERY_STATUS_V2_CRC);
}

/**
 * @brief Encode a battery_status_v2 struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param battery_status_v2 C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_battery_status_v2_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_battery_status_v2_t* battery_status_v2)
{
    return mavlink_msg_battery_status_v2_pack(system_id, component_id, msg, battery_status_v2->id, battery_status_v2->temperature, battery_status_v2->voltage, battery_status_v2->current, battery_status_v2->capacity_consumed, battery_status_v2->capacity_remaining, battery_status_v2->percent_remaining, battery_status_v2->status_flags);
}

/**
 * @brief Encode a battery_status_v2 struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param battery_status_v2 C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_battery_status_v2_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_battery_status_v2_t* battery_status_v2)
{
    return mavlink_msg_battery_status_v2_pack_chan(system_id, component_id, chan, msg, battery_status_v2->id, battery_status_v2->temperature, battery_status_v2->voltage, battery_status_v2->current, battery_status_v2->capacity_consumed, battery_status_v2->capacity_remaining, battery_status_v2->percent_remaining, battery_status_v2->status_flags);
}

/**
 * @brief Encode a battery_status_v2 struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param battery_status_v2 C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_battery_status_v2_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_battery_status_v2_t* battery_status_v2)
{
    return mavlink_msg_battery_status_v2_pack_status(system_id, component_id, _status, msg,  battery_status_v2->id, battery_status_v2->temperature, battery_status_v2->voltage, battery_status_v2->current, battery_status_v2->capacity_consumed, battery_status_v2->capacity_remaining, battery_status_v2->percent_remaining, battery_status_v2->status_flags);
}

/**
 * @brief Send a battery_status_v2 message
 * @param chan MAVLink channel to send the message
 *
 * @param id  Battery ID
 * @param temperature [cdegC] Temperature of the whole battery pack (not internal electronics). INT16_MAX field not provided.
 * @param voltage [V] Battery voltage (total). NaN: field not provided.
 * @param current [A] Battery current (through all cells/loads). Positive value when discharging and negative if charging. NaN: field not provided.
 * @param capacity_consumed [Ah] Consumed charge. NaN: field not provided. This is either the consumption since power-on or since the battery was full, depending on the value of MAV_BATTERY_STATUS_FLAGS_CAPACITY_RELATIVE_TO_FULL.
 * @param capacity_remaining [Ah] Remaining charge (until empty). NaN: field not provided. Note: If MAV_BATTERY_STATUS_FLAGS_CAPACITY_RELATIVE_TO_FULL is unset, this value is based on the assumption the battery was full when the system was powered.
 * @param percent_remaining [%] Remaining battery energy. Values: [0-100], UINT8_MAX: field not provided.
 * @param status_flags  Fault, health, readiness, and other status indications.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_battery_status_v2_send(mavlink_channel_t chan, uint8_t id, int16_t temperature, float voltage, float current, float capacity_consumed, float capacity_remaining, uint8_t percent_remaining, uint32_t status_flags)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_BATTERY_STATUS_V2_LEN];
    _mav_put_float(buf, 0, voltage);
    _mav_put_float(buf, 4, current);
    _mav_put_float(buf, 8, capacity_consumed);
    _mav_put_float(buf, 12, capacity_remaining);
    _mav_put_uint32_t(buf, 16, status_flags);
    _mav_put_int16_t(buf, 20, temperature);
    _mav_put_uint8_t(buf, 22, id);
    _mav_put_uint8_t(buf, 23, percent_remaining);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_BATTERY_STATUS_V2, buf, MAVLINK_MSG_ID_BATTERY_STATUS_V2_MIN_LEN, MAVLINK_MSG_ID_BATTERY_STATUS_V2_LEN, MAVLINK_MSG_ID_BATTERY_STATUS_V2_CRC);
#else
    mavlink_battery_status_v2_t packet;
    packet.voltage = voltage;
    packet.current = current;
    packet.capacity_consumed = capacity_consumed;
    packet.capacity_remaining = capacity_remaining;
    packet.status_flags = status_flags;
    packet.temperature = temperature;
    packet.id = id;
    packet.percent_remaining = percent_remaining;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_BATTERY_STATUS_V2, (const char *)&packet, MAVLINK_MSG_ID_BATTERY_STATUS_V2_MIN_LEN, MAVLINK_MSG_ID_BATTERY_STATUS_V2_LEN, MAVLINK_MSG_ID_BATTERY_STATUS_V2_CRC);
#endif
}

/**
 * @brief Send a battery_status_v2 message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_battery_status_v2_send_struct(mavlink_channel_t chan, const mavlink_battery_status_v2_t* battery_status_v2)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_battery_status_v2_send(chan, battery_status_v2->id, battery_status_v2->temperature, battery_status_v2->voltage, battery_status_v2->current, battery_status_v2->capacity_consumed, battery_status_v2->capacity_remaining, battery_status_v2->percent_remaining, battery_status_v2->status_flags);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_BATTERY_STATUS_V2, (const char *)battery_status_v2, MAVLINK_MSG_ID_BATTERY_STATUS_V2_MIN_LEN, MAVLINK_MSG_ID_BATTERY_STATUS_V2_LEN, MAVLINK_MSG_ID_BATTERY_STATUS_V2_CRC);
#endif
}

#if MAVLINK_MSG_ID_BATTERY_STATUS_V2_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_battery_status_v2_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint8_t id, int16_t temperature, float voltage, float current, float capacity_consumed, float capacity_remaining, uint8_t percent_remaining, uint32_t status_flags)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_float(buf, 0, voltage);
    _mav_put_float(buf, 4, current);
    _mav_put_float(buf, 8, capacity_consumed);
    _mav_put_float(buf, 12, capacity_remaining);
    _mav_put_uint32_t(buf, 16, status_flags);
    _mav_put_int16_t(buf, 20, temperature);
    _mav_put_uint8_t(buf, 22, id);
    _mav_put_uint8_t(buf, 23, percent_remaining);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_BATTERY_STATUS_V2, buf, MAVLINK_MSG_ID_BATTERY_STATUS_V2_MIN_LEN, MAVLINK_MSG_ID_BATTERY_STATUS_V2_LEN, MAVLINK_MSG_ID_BATTERY_STATUS_V2_CRC);
#else
    mavlink_battery_status_v2_t *packet = (mavlink_battery_status_v2_t *)msgbuf;
    packet->voltage = voltage;
    packet->current = current;
    packet->capacity_consumed = capacity_consumed;
    packet->capacity_remaining = capacity_remaining;
    packet->status_flags = status_flags;
    packet->temperature = temperature;
    packet->id = id;
    packet->percent_remaining = percent_remaining;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_BATTERY_STATUS_V2, (const char *)packet, MAVLINK_MSG_ID_BATTERY_STATUS_V2_MIN_LEN, MAVLINK_MSG_ID_BATTERY_STATUS_V2_LEN, MAVLINK_MSG_ID_BATTERY_STATUS_V2_CRC);
#endif
}
#endif

#endif

// MESSAGE BATTERY_STATUS_V2 UNPACKING


/**
 * @brief Get field id from battery_status_v2 message
 *
 * @return  Battery ID
 */
static inline uint8_t mavlink_msg_battery_status_v2_get_id(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  22);
}

/**
 * @brief Get field temperature from battery_status_v2 message
 *
 * @return [cdegC] Temperature of the whole battery pack (not internal electronics). INT16_MAX field not provided.
 */
static inline int16_t mavlink_msg_battery_status_v2_get_temperature(const mavlink_message_t* msg)
{
    return _MAV_RETURN_int16_t(msg,  20);
}

/**
 * @brief Get field voltage from battery_status_v2 message
 *
 * @return [V] Battery voltage (total). NaN: field not provided.
 */
static inline float mavlink_msg_battery_status_v2_get_voltage(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  0);
}

/**
 * @brief Get field current from battery_status_v2 message
 *
 * @return [A] Battery current (through all cells/loads). Positive value when discharging and negative if charging. NaN: field not provided.
 */
static inline float mavlink_msg_battery_status_v2_get_current(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  4);
}

/**
 * @brief Get field capacity_consumed from battery_status_v2 message
 *
 * @return [Ah] Consumed charge. NaN: field not provided. This is either the consumption since power-on or since the battery was full, depending on the value of MAV_BATTERY_STATUS_FLAGS_CAPACITY_RELATIVE_TO_FULL.
 */
static inline float mavlink_msg_battery_status_v2_get_capacity_consumed(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  8);
}

/**
 * @brief Get field capacity_remaining from battery_status_v2 message
 *
 * @return [Ah] Remaining charge (until empty). NaN: field not provided. Note: If MAV_BATTERY_STATUS_FLAGS_CAPACITY_RELATIVE_TO_FULL is unset, this value is based on the assumption the battery was full when the system was powered.
 */
static inline float mavlink_msg_battery_status_v2_get_capacity_remaining(const mavlink_message_t* msg)
{
    return _MAV_RETURN_float(msg,  12);
}

/**
 * @brief Get field percent_remaining from battery_status_v2 message
 *
 * @return [%] Remaining battery energy. Values: [0-100], UINT8_MAX: field not provided.
 */
static inline uint8_t mavlink_msg_battery_status_v2_get_percent_remaining(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  23);
}

/**
 * @brief Get field status_flags from battery_status_v2 message
 *
 * @return  Fault, health, readiness, and other status indications.
 */
static inline uint32_t mavlink_msg_battery_status_v2_get_status_flags(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  16);
}

/**
 * @brief Decode a battery_status_v2 message into a struct
 *
 * @param msg The message to decode
 * @param battery_status_v2 C-struct to decode the message contents into
 */
static inline void mavlink_msg_battery_status_v2_decode(const mavlink_message_t* msg, mavlink_battery_status_v2_t* battery_status_v2)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    battery_status_v2->voltage = mavlink_msg_battery_status_v2_get_voltage(msg);
    battery_status_v2->current = mavlink_msg_battery_status_v2_get_current(msg);
    battery_status_v2->capacity_consumed = mavlink_msg_battery_status_v2_get_capacity_consumed(msg);
    battery_status_v2->capacity_remaining = mavlink_msg_battery_status_v2_get_capacity_remaining(msg);
    battery_status_v2->status_flags = mavlink_msg_battery_status_v2_get_status_flags(msg);
    battery_status_v2->temperature = mavlink_msg_battery_status_v2_get_temperature(msg);
    battery_status_v2->id = mavlink_msg_battery_status_v2_get_id(msg);
    battery_status_v2->percent_remaining = mavlink_msg_battery_status_v2_get_percent_remaining(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_BATTERY_STATUS_V2_LEN? msg->len : MAVLINK_MSG_ID_BATTERY_STATUS_V2_LEN;
        memset(battery_status_v2, 0, MAVLINK_MSG_ID_BATTERY_STATUS_V2_LEN);
    memcpy(battery_status_v2, _MAV_PAYLOAD(msg), len);
#endif
}
