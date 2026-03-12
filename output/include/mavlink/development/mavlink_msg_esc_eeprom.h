#pragma once
// MESSAGE ESC_EEPROM PACKING

#define MAVLINK_MSG_ID_ESC_EEPROM 292


typedef struct __mavlink_esc_eeprom_t {
 uint32_t write_mask[6]; /*<  Bitmask indicating which bytes in the data array should be written. Each bit corresponds to a byte index in the data array (bit 0 of write_mask[0] = data[0], bit 31 of write_mask[0] = data[31], bit 0 of write_mask[1] = data[32], etc.). Set bits indicate bytes to write, cleared bits indicate bytes to skip. This allows precise updates of individual parameters without overwriting the entire EEPROM.*/
 uint8_t target_system; /*<  System ID (ID of target system, normally flight controller).*/
 uint8_t target_component; /*<  Component ID (normally 0 for broadcast).*/
 uint8_t firmware; /*<  ESC firmware type.*/
 uint8_t msg_index; /*<  Zero-indexed sequence number of this message when multiple messages are required to transfer the complete EEPROM data. The first message has index 0. For single-message transfers, set to 0.*/
 uint8_t msg_count; /*<  Total number of messages required to transfer the complete EEPROM data. For single-message transfers, set to 1. Receivers should collect all messages from index 0 to msg_count-1 before reconstructing the complete data.*/
 uint8_t esc_index; /*<  Index of the ESC (0 = ESC1, 1 = ESC2, etc.).*/
 uint8_t length; /*<  Number of valid bytes in data array.*/
 uint8_t data[192]; /*<  Raw ESC EEPROM data. Unused bytes should be set to zero.*/
} mavlink_esc_eeprom_t;

#define MAVLINK_MSG_ID_ESC_EEPROM_LEN 223
#define MAVLINK_MSG_ID_ESC_EEPROM_MIN_LEN 223
#define MAVLINK_MSG_ID_292_LEN 223
#define MAVLINK_MSG_ID_292_MIN_LEN 223

#define MAVLINK_MSG_ID_ESC_EEPROM_CRC 227
#define MAVLINK_MSG_ID_292_CRC 227

#define MAVLINK_MSG_ESC_EEPROM_FIELD_WRITE_MASK_LEN 6
#define MAVLINK_MSG_ESC_EEPROM_FIELD_DATA_LEN 192

#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_ESC_EEPROM { \
    292, \
    "ESC_EEPROM", \
    9, \
    {  { "target_system", NULL, MAVLINK_TYPE_UINT8_T, 0, 24, offsetof(mavlink_esc_eeprom_t, target_system) }, \
         { "target_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 25, offsetof(mavlink_esc_eeprom_t, target_component) }, \
         { "firmware", NULL, MAVLINK_TYPE_UINT8_T, 0, 26, offsetof(mavlink_esc_eeprom_t, firmware) }, \
         { "msg_index", NULL, MAVLINK_TYPE_UINT8_T, 0, 27, offsetof(mavlink_esc_eeprom_t, msg_index) }, \
         { "msg_count", NULL, MAVLINK_TYPE_UINT8_T, 0, 28, offsetof(mavlink_esc_eeprom_t, msg_count) }, \
         { "esc_index", NULL, MAVLINK_TYPE_UINT8_T, 0, 29, offsetof(mavlink_esc_eeprom_t, esc_index) }, \
         { "write_mask", NULL, MAVLINK_TYPE_UINT32_T, 6, 0, offsetof(mavlink_esc_eeprom_t, write_mask) }, \
         { "length", NULL, MAVLINK_TYPE_UINT8_T, 0, 30, offsetof(mavlink_esc_eeprom_t, length) }, \
         { "data", NULL, MAVLINK_TYPE_UINT8_T, 192, 31, offsetof(mavlink_esc_eeprom_t, data) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_ESC_EEPROM { \
    "ESC_EEPROM", \
    9, \
    {  { "target_system", NULL, MAVLINK_TYPE_UINT8_T, 0, 24, offsetof(mavlink_esc_eeprom_t, target_system) }, \
         { "target_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 25, offsetof(mavlink_esc_eeprom_t, target_component) }, \
         { "firmware", NULL, MAVLINK_TYPE_UINT8_T, 0, 26, offsetof(mavlink_esc_eeprom_t, firmware) }, \
         { "msg_index", NULL, MAVLINK_TYPE_UINT8_T, 0, 27, offsetof(mavlink_esc_eeprom_t, msg_index) }, \
         { "msg_count", NULL, MAVLINK_TYPE_UINT8_T, 0, 28, offsetof(mavlink_esc_eeprom_t, msg_count) }, \
         { "esc_index", NULL, MAVLINK_TYPE_UINT8_T, 0, 29, offsetof(mavlink_esc_eeprom_t, esc_index) }, \
         { "write_mask", NULL, MAVLINK_TYPE_UINT32_T, 6, 0, offsetof(mavlink_esc_eeprom_t, write_mask) }, \
         { "length", NULL, MAVLINK_TYPE_UINT8_T, 0, 30, offsetof(mavlink_esc_eeprom_t, length) }, \
         { "data", NULL, MAVLINK_TYPE_UINT8_T, 192, 31, offsetof(mavlink_esc_eeprom_t, data) }, \
         } \
}
#endif

/**
 * @brief Pack a esc_eeprom message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system  System ID (ID of target system, normally flight controller).
 * @param target_component  Component ID (normally 0 for broadcast).
 * @param firmware  ESC firmware type.
 * @param msg_index  Zero-indexed sequence number of this message when multiple messages are required to transfer the complete EEPROM data. The first message has index 0. For single-message transfers, set to 0.
 * @param msg_count  Total number of messages required to transfer the complete EEPROM data. For single-message transfers, set to 1. Receivers should collect all messages from index 0 to msg_count-1 before reconstructing the complete data.
 * @param esc_index  Index of the ESC (0 = ESC1, 1 = ESC2, etc.).
 * @param write_mask  Bitmask indicating which bytes in the data array should be written. Each bit corresponds to a byte index in the data array (bit 0 of write_mask[0] = data[0], bit 31 of write_mask[0] = data[31], bit 0 of write_mask[1] = data[32], etc.). Set bits indicate bytes to write, cleared bits indicate bytes to skip. This allows precise updates of individual parameters without overwriting the entire EEPROM.
 * @param length  Number of valid bytes in data array.
 * @param data  Raw ESC EEPROM data. Unused bytes should be set to zero.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_esc_eeprom_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint8_t target_system, uint8_t target_component, uint8_t firmware, uint8_t msg_index, uint8_t msg_count, uint8_t esc_index, const uint32_t *write_mask, uint8_t length, const uint8_t *data)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ESC_EEPROM_LEN];
    _mav_put_uint8_t(buf, 24, target_system);
    _mav_put_uint8_t(buf, 25, target_component);
    _mav_put_uint8_t(buf, 26, firmware);
    _mav_put_uint8_t(buf, 27, msg_index);
    _mav_put_uint8_t(buf, 28, msg_count);
    _mav_put_uint8_t(buf, 29, esc_index);
    _mav_put_uint8_t(buf, 30, length);
    _mav_put_uint32_t_array(buf, 0, write_mask, 6);
    _mav_put_uint8_t_array(buf, 31, data, 192);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ESC_EEPROM_LEN);
#else
    mavlink_esc_eeprom_t packet;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.firmware = firmware;
    packet.msg_index = msg_index;
    packet.msg_count = msg_count;
    packet.esc_index = esc_index;
    packet.length = length;
    mav_array_memcpy(packet.write_mask, write_mask, sizeof(uint32_t)*6);
    mav_array_memcpy(packet.data, data, sizeof(uint8_t)*192);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ESC_EEPROM_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ESC_EEPROM;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_ESC_EEPROM_MIN_LEN, MAVLINK_MSG_ID_ESC_EEPROM_LEN, MAVLINK_MSG_ID_ESC_EEPROM_CRC);
}

/**
 * @brief Pack a esc_eeprom message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system  System ID (ID of target system, normally flight controller).
 * @param target_component  Component ID (normally 0 for broadcast).
 * @param firmware  ESC firmware type.
 * @param msg_index  Zero-indexed sequence number of this message when multiple messages are required to transfer the complete EEPROM data. The first message has index 0. For single-message transfers, set to 0.
 * @param msg_count  Total number of messages required to transfer the complete EEPROM data. For single-message transfers, set to 1. Receivers should collect all messages from index 0 to msg_count-1 before reconstructing the complete data.
 * @param esc_index  Index of the ESC (0 = ESC1, 1 = ESC2, etc.).
 * @param write_mask  Bitmask indicating which bytes in the data array should be written. Each bit corresponds to a byte index in the data array (bit 0 of write_mask[0] = data[0], bit 31 of write_mask[0] = data[31], bit 0 of write_mask[1] = data[32], etc.). Set bits indicate bytes to write, cleared bits indicate bytes to skip. This allows precise updates of individual parameters without overwriting the entire EEPROM.
 * @param length  Number of valid bytes in data array.
 * @param data  Raw ESC EEPROM data. Unused bytes should be set to zero.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_esc_eeprom_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint8_t target_system, uint8_t target_component, uint8_t firmware, uint8_t msg_index, uint8_t msg_count, uint8_t esc_index, const uint32_t *write_mask, uint8_t length, const uint8_t *data)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ESC_EEPROM_LEN];
    _mav_put_uint8_t(buf, 24, target_system);
    _mav_put_uint8_t(buf, 25, target_component);
    _mav_put_uint8_t(buf, 26, firmware);
    _mav_put_uint8_t(buf, 27, msg_index);
    _mav_put_uint8_t(buf, 28, msg_count);
    _mav_put_uint8_t(buf, 29, esc_index);
    _mav_put_uint8_t(buf, 30, length);
    _mav_put_uint32_t_array(buf, 0, write_mask, 6);
    _mav_put_uint8_t_array(buf, 31, data, 192);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ESC_EEPROM_LEN);
#else
    mavlink_esc_eeprom_t packet;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.firmware = firmware;
    packet.msg_index = msg_index;
    packet.msg_count = msg_count;
    packet.esc_index = esc_index;
    packet.length = length;
    mav_array_memcpy(packet.write_mask, write_mask, sizeof(uint32_t)*6);
    mav_array_memcpy(packet.data, data, sizeof(uint8_t)*192);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ESC_EEPROM_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ESC_EEPROM;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_ESC_EEPROM_MIN_LEN, MAVLINK_MSG_ID_ESC_EEPROM_LEN, MAVLINK_MSG_ID_ESC_EEPROM_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_ESC_EEPROM_MIN_LEN, MAVLINK_MSG_ID_ESC_EEPROM_LEN);
#endif
}

/**
 * @brief Pack a esc_eeprom message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system  System ID (ID of target system, normally flight controller).
 * @param target_component  Component ID (normally 0 for broadcast).
 * @param firmware  ESC firmware type.
 * @param msg_index  Zero-indexed sequence number of this message when multiple messages are required to transfer the complete EEPROM data. The first message has index 0. For single-message transfers, set to 0.
 * @param msg_count  Total number of messages required to transfer the complete EEPROM data. For single-message transfers, set to 1. Receivers should collect all messages from index 0 to msg_count-1 before reconstructing the complete data.
 * @param esc_index  Index of the ESC (0 = ESC1, 1 = ESC2, etc.).
 * @param write_mask  Bitmask indicating which bytes in the data array should be written. Each bit corresponds to a byte index in the data array (bit 0 of write_mask[0] = data[0], bit 31 of write_mask[0] = data[31], bit 0 of write_mask[1] = data[32], etc.). Set bits indicate bytes to write, cleared bits indicate bytes to skip. This allows precise updates of individual parameters without overwriting the entire EEPROM.
 * @param length  Number of valid bytes in data array.
 * @param data  Raw ESC EEPROM data. Unused bytes should be set to zero.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_esc_eeprom_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint8_t target_system,uint8_t target_component,uint8_t firmware,uint8_t msg_index,uint8_t msg_count,uint8_t esc_index,const uint32_t *write_mask,uint8_t length,const uint8_t *data)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ESC_EEPROM_LEN];
    _mav_put_uint8_t(buf, 24, target_system);
    _mav_put_uint8_t(buf, 25, target_component);
    _mav_put_uint8_t(buf, 26, firmware);
    _mav_put_uint8_t(buf, 27, msg_index);
    _mav_put_uint8_t(buf, 28, msg_count);
    _mav_put_uint8_t(buf, 29, esc_index);
    _mav_put_uint8_t(buf, 30, length);
    _mav_put_uint32_t_array(buf, 0, write_mask, 6);
    _mav_put_uint8_t_array(buf, 31, data, 192);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_ESC_EEPROM_LEN);
#else
    mavlink_esc_eeprom_t packet;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.firmware = firmware;
    packet.msg_index = msg_index;
    packet.msg_count = msg_count;
    packet.esc_index = esc_index;
    packet.length = length;
    mav_array_memcpy(packet.write_mask, write_mask, sizeof(uint32_t)*6);
    mav_array_memcpy(packet.data, data, sizeof(uint8_t)*192);
        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_ESC_EEPROM_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_ESC_EEPROM;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_ESC_EEPROM_MIN_LEN, MAVLINK_MSG_ID_ESC_EEPROM_LEN, MAVLINK_MSG_ID_ESC_EEPROM_CRC);
}

/**
 * @brief Encode a esc_eeprom struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param esc_eeprom C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_esc_eeprom_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_esc_eeprom_t* esc_eeprom)
{
    return mavlink_msg_esc_eeprom_pack(system_id, component_id, msg, esc_eeprom->target_system, esc_eeprom->target_component, esc_eeprom->firmware, esc_eeprom->msg_index, esc_eeprom->msg_count, esc_eeprom->esc_index, esc_eeprom->write_mask, esc_eeprom->length, esc_eeprom->data);
}

/**
 * @brief Encode a esc_eeprom struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param esc_eeprom C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_esc_eeprom_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_esc_eeprom_t* esc_eeprom)
{
    return mavlink_msg_esc_eeprom_pack_chan(system_id, component_id, chan, msg, esc_eeprom->target_system, esc_eeprom->target_component, esc_eeprom->firmware, esc_eeprom->msg_index, esc_eeprom->msg_count, esc_eeprom->esc_index, esc_eeprom->write_mask, esc_eeprom->length, esc_eeprom->data);
}

/**
 * @brief Encode a esc_eeprom struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param esc_eeprom C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_esc_eeprom_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_esc_eeprom_t* esc_eeprom)
{
    return mavlink_msg_esc_eeprom_pack_status(system_id, component_id, _status, msg,  esc_eeprom->target_system, esc_eeprom->target_component, esc_eeprom->firmware, esc_eeprom->msg_index, esc_eeprom->msg_count, esc_eeprom->esc_index, esc_eeprom->write_mask, esc_eeprom->length, esc_eeprom->data);
}

/**
 * @brief Send a esc_eeprom message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system  System ID (ID of target system, normally flight controller).
 * @param target_component  Component ID (normally 0 for broadcast).
 * @param firmware  ESC firmware type.
 * @param msg_index  Zero-indexed sequence number of this message when multiple messages are required to transfer the complete EEPROM data. The first message has index 0. For single-message transfers, set to 0.
 * @param msg_count  Total number of messages required to transfer the complete EEPROM data. For single-message transfers, set to 1. Receivers should collect all messages from index 0 to msg_count-1 before reconstructing the complete data.
 * @param esc_index  Index of the ESC (0 = ESC1, 1 = ESC2, etc.).
 * @param write_mask  Bitmask indicating which bytes in the data array should be written. Each bit corresponds to a byte index in the data array (bit 0 of write_mask[0] = data[0], bit 31 of write_mask[0] = data[31], bit 0 of write_mask[1] = data[32], etc.). Set bits indicate bytes to write, cleared bits indicate bytes to skip. This allows precise updates of individual parameters without overwriting the entire EEPROM.
 * @param length  Number of valid bytes in data array.
 * @param data  Raw ESC EEPROM data. Unused bytes should be set to zero.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_esc_eeprom_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, uint8_t firmware, uint8_t msg_index, uint8_t msg_count, uint8_t esc_index, const uint32_t *write_mask, uint8_t length, const uint8_t *data)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_ESC_EEPROM_LEN];
    _mav_put_uint8_t(buf, 24, target_system);
    _mav_put_uint8_t(buf, 25, target_component);
    _mav_put_uint8_t(buf, 26, firmware);
    _mav_put_uint8_t(buf, 27, msg_index);
    _mav_put_uint8_t(buf, 28, msg_count);
    _mav_put_uint8_t(buf, 29, esc_index);
    _mav_put_uint8_t(buf, 30, length);
    _mav_put_uint32_t_array(buf, 0, write_mask, 6);
    _mav_put_uint8_t_array(buf, 31, data, 192);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ESC_EEPROM, buf, MAVLINK_MSG_ID_ESC_EEPROM_MIN_LEN, MAVLINK_MSG_ID_ESC_EEPROM_LEN, MAVLINK_MSG_ID_ESC_EEPROM_CRC);
#else
    mavlink_esc_eeprom_t packet;
    packet.target_system = target_system;
    packet.target_component = target_component;
    packet.firmware = firmware;
    packet.msg_index = msg_index;
    packet.msg_count = msg_count;
    packet.esc_index = esc_index;
    packet.length = length;
    mav_array_memcpy(packet.write_mask, write_mask, sizeof(uint32_t)*6);
    mav_array_memcpy(packet.data, data, sizeof(uint8_t)*192);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ESC_EEPROM, (const char *)&packet, MAVLINK_MSG_ID_ESC_EEPROM_MIN_LEN, MAVLINK_MSG_ID_ESC_EEPROM_LEN, MAVLINK_MSG_ID_ESC_EEPROM_CRC);
#endif
}

/**
 * @brief Send a esc_eeprom message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_esc_eeprom_send_struct(mavlink_channel_t chan, const mavlink_esc_eeprom_t* esc_eeprom)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_esc_eeprom_send(chan, esc_eeprom->target_system, esc_eeprom->target_component, esc_eeprom->firmware, esc_eeprom->msg_index, esc_eeprom->msg_count, esc_eeprom->esc_index, esc_eeprom->write_mask, esc_eeprom->length, esc_eeprom->data);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ESC_EEPROM, (const char *)esc_eeprom, MAVLINK_MSG_ID_ESC_EEPROM_MIN_LEN, MAVLINK_MSG_ID_ESC_EEPROM_LEN, MAVLINK_MSG_ID_ESC_EEPROM_CRC);
#endif
}

#if MAVLINK_MSG_ID_ESC_EEPROM_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_esc_eeprom_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint8_t target_system, uint8_t target_component, uint8_t firmware, uint8_t msg_index, uint8_t msg_count, uint8_t esc_index, const uint32_t *write_mask, uint8_t length, const uint8_t *data)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint8_t(buf, 24, target_system);
    _mav_put_uint8_t(buf, 25, target_component);
    _mav_put_uint8_t(buf, 26, firmware);
    _mav_put_uint8_t(buf, 27, msg_index);
    _mav_put_uint8_t(buf, 28, msg_count);
    _mav_put_uint8_t(buf, 29, esc_index);
    _mav_put_uint8_t(buf, 30, length);
    _mav_put_uint32_t_array(buf, 0, write_mask, 6);
    _mav_put_uint8_t_array(buf, 31, data, 192);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ESC_EEPROM, buf, MAVLINK_MSG_ID_ESC_EEPROM_MIN_LEN, MAVLINK_MSG_ID_ESC_EEPROM_LEN, MAVLINK_MSG_ID_ESC_EEPROM_CRC);
#else
    mavlink_esc_eeprom_t *packet = (mavlink_esc_eeprom_t *)msgbuf;
    packet->target_system = target_system;
    packet->target_component = target_component;
    packet->firmware = firmware;
    packet->msg_index = msg_index;
    packet->msg_count = msg_count;
    packet->esc_index = esc_index;
    packet->length = length;
    mav_array_memcpy(packet->write_mask, write_mask, sizeof(uint32_t)*6);
    mav_array_memcpy(packet->data, data, sizeof(uint8_t)*192);
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_ESC_EEPROM, (const char *)packet, MAVLINK_MSG_ID_ESC_EEPROM_MIN_LEN, MAVLINK_MSG_ID_ESC_EEPROM_LEN, MAVLINK_MSG_ID_ESC_EEPROM_CRC);
#endif
}
#endif

#endif

// MESSAGE ESC_EEPROM UNPACKING


/**
 * @brief Get field target_system from esc_eeprom message
 *
 * @return  System ID (ID of target system, normally flight controller).
 */
static inline uint8_t mavlink_msg_esc_eeprom_get_target_system(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  24);
}

/**
 * @brief Get field target_component from esc_eeprom message
 *
 * @return  Component ID (normally 0 for broadcast).
 */
static inline uint8_t mavlink_msg_esc_eeprom_get_target_component(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  25);
}

/**
 * @brief Get field firmware from esc_eeprom message
 *
 * @return  ESC firmware type.
 */
static inline uint8_t mavlink_msg_esc_eeprom_get_firmware(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  26);
}

/**
 * @brief Get field msg_index from esc_eeprom message
 *
 * @return  Zero-indexed sequence number of this message when multiple messages are required to transfer the complete EEPROM data. The first message has index 0. For single-message transfers, set to 0.
 */
static inline uint8_t mavlink_msg_esc_eeprom_get_msg_index(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  27);
}

/**
 * @brief Get field msg_count from esc_eeprom message
 *
 * @return  Total number of messages required to transfer the complete EEPROM data. For single-message transfers, set to 1. Receivers should collect all messages from index 0 to msg_count-1 before reconstructing the complete data.
 */
static inline uint8_t mavlink_msg_esc_eeprom_get_msg_count(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  28);
}

/**
 * @brief Get field esc_index from esc_eeprom message
 *
 * @return  Index of the ESC (0 = ESC1, 1 = ESC2, etc.).
 */
static inline uint8_t mavlink_msg_esc_eeprom_get_esc_index(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  29);
}

/**
 * @brief Get field write_mask from esc_eeprom message
 *
 * @return  Bitmask indicating which bytes in the data array should be written. Each bit corresponds to a byte index in the data array (bit 0 of write_mask[0] = data[0], bit 31 of write_mask[0] = data[31], bit 0 of write_mask[1] = data[32], etc.). Set bits indicate bytes to write, cleared bits indicate bytes to skip. This allows precise updates of individual parameters without overwriting the entire EEPROM.
 */
static inline uint16_t mavlink_msg_esc_eeprom_get_write_mask(const mavlink_message_t* msg, uint32_t *write_mask)
{
    return _MAV_RETURN_uint32_t_array(msg, write_mask, 6,  0);
}

/**
 * @brief Get field length from esc_eeprom message
 *
 * @return  Number of valid bytes in data array.
 */
static inline uint8_t mavlink_msg_esc_eeprom_get_length(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint8_t(msg,  30);
}

/**
 * @brief Get field data from esc_eeprom message
 *
 * @return  Raw ESC EEPROM data. Unused bytes should be set to zero.
 */
static inline uint16_t mavlink_msg_esc_eeprom_get_data(const mavlink_message_t* msg, uint8_t *data)
{
    return _MAV_RETURN_uint8_t_array(msg, data, 192,  31);
}

/**
 * @brief Decode a esc_eeprom message into a struct
 *
 * @param msg The message to decode
 * @param esc_eeprom C-struct to decode the message contents into
 */
static inline void mavlink_msg_esc_eeprom_decode(const mavlink_message_t* msg, mavlink_esc_eeprom_t* esc_eeprom)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_esc_eeprom_get_write_mask(msg, esc_eeprom->write_mask);
    esc_eeprom->target_system = mavlink_msg_esc_eeprom_get_target_system(msg);
    esc_eeprom->target_component = mavlink_msg_esc_eeprom_get_target_component(msg);
    esc_eeprom->firmware = mavlink_msg_esc_eeprom_get_firmware(msg);
    esc_eeprom->msg_index = mavlink_msg_esc_eeprom_get_msg_index(msg);
    esc_eeprom->msg_count = mavlink_msg_esc_eeprom_get_msg_count(msg);
    esc_eeprom->esc_index = mavlink_msg_esc_eeprom_get_esc_index(msg);
    esc_eeprom->length = mavlink_msg_esc_eeprom_get_length(msg);
    mavlink_msg_esc_eeprom_get_data(msg, esc_eeprom->data);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_ESC_EEPROM_LEN? msg->len : MAVLINK_MSG_ID_ESC_EEPROM_LEN;
        memset(esc_eeprom, 0, MAVLINK_MSG_ID_ESC_EEPROM_LEN);
    memcpy(esc_eeprom, _MAV_PAYLOAD(msg), len);
#endif
}
