#pragma once
// MESSAGE GROUP_START PACKING

#define MAVLINK_MSG_ID_GROUP_START 414


typedef struct __mavlink_group_start_t {
 uint64_t time_usec; /*< [us] Timestamp (UNIX Epoch time or time since system boot).
        The receiving end can infer timestamp format (since 1.1.1970 or since system boot) by checking for the magnitude of the number.*/
 uint32_t group_id; /*<  Mission-unique group id (from MAV_CMD_GROUP_START).*/
 uint32_t mission_checksum; /*<  CRC32 checksum of current plan for MAV_MISSION_TYPE_ALL. As defined in MISSION_CHECKSUM message.*/
} mavlink_group_start_t;

#define MAVLINK_MSG_ID_GROUP_START_LEN 16
#define MAVLINK_MSG_ID_GROUP_START_MIN_LEN 16
#define MAVLINK_MSG_ID_414_LEN 16
#define MAVLINK_MSG_ID_414_MIN_LEN 16

#define MAVLINK_MSG_ID_GROUP_START_CRC 109
#define MAVLINK_MSG_ID_414_CRC 109



#if MAVLINK_COMMAND_24BIT
#define MAVLINK_MESSAGE_INFO_GROUP_START { \
    414, \
    "GROUP_START", \
    3, \
    {  { "group_id", NULL, MAVLINK_TYPE_UINT32_T, 0, 8, offsetof(mavlink_group_start_t, group_id) }, \
         { "mission_checksum", NULL, MAVLINK_TYPE_UINT32_T, 0, 12, offsetof(mavlink_group_start_t, mission_checksum) }, \
         { "time_usec", NULL, MAVLINK_TYPE_UINT64_T, 0, 0, offsetof(mavlink_group_start_t, time_usec) }, \
         } \
}
#else
#define MAVLINK_MESSAGE_INFO_GROUP_START { \
    "GROUP_START", \
    3, \
    {  { "group_id", NULL, MAVLINK_TYPE_UINT32_T, 0, 8, offsetof(mavlink_group_start_t, group_id) }, \
         { "mission_checksum", NULL, MAVLINK_TYPE_UINT32_T, 0, 12, offsetof(mavlink_group_start_t, mission_checksum) }, \
         { "time_usec", NULL, MAVLINK_TYPE_UINT64_T, 0, 0, offsetof(mavlink_group_start_t, time_usec) }, \
         } \
}
#endif

/**
 * @brief Pack a group_start message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param group_id  Mission-unique group id (from MAV_CMD_GROUP_START).
 * @param mission_checksum  CRC32 checksum of current plan for MAV_MISSION_TYPE_ALL. As defined in MISSION_CHECKSUM message.
 * @param time_usec [us] Timestamp (UNIX Epoch time or time since system boot).
        The receiving end can infer timestamp format (since 1.1.1970 or since system boot) by checking for the magnitude of the number.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_group_start_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
                               uint32_t group_id, uint32_t mission_checksum, uint64_t time_usec)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_GROUP_START_LEN];
    _mav_put_uint64_t(buf, 0, time_usec);
    _mav_put_uint32_t(buf, 8, group_id);
    _mav_put_uint32_t(buf, 12, mission_checksum);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_GROUP_START_LEN);
#else
    mavlink_group_start_t packet;
    packet.time_usec = time_usec;
    packet.group_id = group_id;
    packet.mission_checksum = mission_checksum;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_GROUP_START_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_GROUP_START;
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_GROUP_START_MIN_LEN, MAVLINK_MSG_ID_GROUP_START_LEN, MAVLINK_MSG_ID_GROUP_START_CRC);
}

/**
 * @brief Pack a group_start message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 *
 * @param group_id  Mission-unique group id (from MAV_CMD_GROUP_START).
 * @param mission_checksum  CRC32 checksum of current plan for MAV_MISSION_TYPE_ALL. As defined in MISSION_CHECKSUM message.
 * @param time_usec [us] Timestamp (UNIX Epoch time or time since system boot).
        The receiving end can infer timestamp format (since 1.1.1970 or since system boot) by checking for the magnitude of the number.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_group_start_pack_status(uint8_t system_id, uint8_t component_id, mavlink_status_t *_status, mavlink_message_t* msg,
                               uint32_t group_id, uint32_t mission_checksum, uint64_t time_usec)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_GROUP_START_LEN];
    _mav_put_uint64_t(buf, 0, time_usec);
    _mav_put_uint32_t(buf, 8, group_id);
    _mav_put_uint32_t(buf, 12, mission_checksum);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_GROUP_START_LEN);
#else
    mavlink_group_start_t packet;
    packet.time_usec = time_usec;
    packet.group_id = group_id;
    packet.mission_checksum = mission_checksum;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_GROUP_START_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_GROUP_START;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_GROUP_START_MIN_LEN, MAVLINK_MSG_ID_GROUP_START_LEN, MAVLINK_MSG_ID_GROUP_START_CRC);
#else
    return mavlink_finalize_message_buffer(msg, system_id, component_id, _status, MAVLINK_MSG_ID_GROUP_START_MIN_LEN, MAVLINK_MSG_ID_GROUP_START_LEN);
#endif
}

/**
 * @brief Pack a group_start message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param group_id  Mission-unique group id (from MAV_CMD_GROUP_START).
 * @param mission_checksum  CRC32 checksum of current plan for MAV_MISSION_TYPE_ALL. As defined in MISSION_CHECKSUM message.
 * @param time_usec [us] Timestamp (UNIX Epoch time or time since system boot).
        The receiving end can infer timestamp format (since 1.1.1970 or since system boot) by checking for the magnitude of the number.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_group_start_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
                               mavlink_message_t* msg,
                                   uint32_t group_id,uint32_t mission_checksum,uint64_t time_usec)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_GROUP_START_LEN];
    _mav_put_uint64_t(buf, 0, time_usec);
    _mav_put_uint32_t(buf, 8, group_id);
    _mav_put_uint32_t(buf, 12, mission_checksum);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_GROUP_START_LEN);
#else
    mavlink_group_start_t packet;
    packet.time_usec = time_usec;
    packet.group_id = group_id;
    packet.mission_checksum = mission_checksum;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_GROUP_START_LEN);
#endif

    msg->msgid = MAVLINK_MSG_ID_GROUP_START;
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_GROUP_START_MIN_LEN, MAVLINK_MSG_ID_GROUP_START_LEN, MAVLINK_MSG_ID_GROUP_START_CRC);
}

/**
 * @brief Encode a group_start struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param group_start C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_group_start_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_group_start_t* group_start)
{
    return mavlink_msg_group_start_pack(system_id, component_id, msg, group_start->group_id, group_start->mission_checksum, group_start->time_usec);
}

/**
 * @brief Encode a group_start struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param group_start C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_group_start_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_group_start_t* group_start)
{
    return mavlink_msg_group_start_pack_chan(system_id, component_id, chan, msg, group_start->group_id, group_start->mission_checksum, group_start->time_usec);
}

/**
 * @brief Encode a group_start struct with provided status structure
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param status MAVLink status structure
 * @param msg The MAVLink message to compress the data into
 * @param group_start C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_group_start_encode_status(uint8_t system_id, uint8_t component_id, mavlink_status_t* _status, mavlink_message_t* msg, const mavlink_group_start_t* group_start)
{
    return mavlink_msg_group_start_pack_status(system_id, component_id, _status, msg,  group_start->group_id, group_start->mission_checksum, group_start->time_usec);
}

/**
 * @brief Send a group_start message
 * @param chan MAVLink channel to send the message
 *
 * @param group_id  Mission-unique group id (from MAV_CMD_GROUP_START).
 * @param mission_checksum  CRC32 checksum of current plan for MAV_MISSION_TYPE_ALL. As defined in MISSION_CHECKSUM message.
 * @param time_usec [us] Timestamp (UNIX Epoch time or time since system boot).
        The receiving end can infer timestamp format (since 1.1.1970 or since system boot) by checking for the magnitude of the number.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_group_start_send(mavlink_channel_t chan, uint32_t group_id, uint32_t mission_checksum, uint64_t time_usec)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char buf[MAVLINK_MSG_ID_GROUP_START_LEN];
    _mav_put_uint64_t(buf, 0, time_usec);
    _mav_put_uint32_t(buf, 8, group_id);
    _mav_put_uint32_t(buf, 12, mission_checksum);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_GROUP_START, buf, MAVLINK_MSG_ID_GROUP_START_MIN_LEN, MAVLINK_MSG_ID_GROUP_START_LEN, MAVLINK_MSG_ID_GROUP_START_CRC);
#else
    mavlink_group_start_t packet;
    packet.time_usec = time_usec;
    packet.group_id = group_id;
    packet.mission_checksum = mission_checksum;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_GROUP_START, (const char *)&packet, MAVLINK_MSG_ID_GROUP_START_MIN_LEN, MAVLINK_MSG_ID_GROUP_START_LEN, MAVLINK_MSG_ID_GROUP_START_CRC);
#endif
}

/**
 * @brief Send a group_start message
 * @param chan MAVLink channel to send the message
 * @param struct The MAVLink struct to serialize
 */
static inline void mavlink_msg_group_start_send_struct(mavlink_channel_t chan, const mavlink_group_start_t* group_start)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    mavlink_msg_group_start_send(chan, group_start->group_id, group_start->mission_checksum, group_start->time_usec);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_GROUP_START, (const char *)group_start, MAVLINK_MSG_ID_GROUP_START_MIN_LEN, MAVLINK_MSG_ID_GROUP_START_LEN, MAVLINK_MSG_ID_GROUP_START_CRC);
#endif
}

#if MAVLINK_MSG_ID_GROUP_START_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This variant of _send() can be used to save stack space by reusing
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_group_start_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint32_t group_id, uint32_t mission_checksum, uint64_t time_usec)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    char *buf = (char *)msgbuf;
    _mav_put_uint64_t(buf, 0, time_usec);
    _mav_put_uint32_t(buf, 8, group_id);
    _mav_put_uint32_t(buf, 12, mission_checksum);

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_GROUP_START, buf, MAVLINK_MSG_ID_GROUP_START_MIN_LEN, MAVLINK_MSG_ID_GROUP_START_LEN, MAVLINK_MSG_ID_GROUP_START_CRC);
#else
    mavlink_group_start_t *packet = (mavlink_group_start_t *)msgbuf;
    packet->time_usec = time_usec;
    packet->group_id = group_id;
    packet->mission_checksum = mission_checksum;

    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_GROUP_START, (const char *)packet, MAVLINK_MSG_ID_GROUP_START_MIN_LEN, MAVLINK_MSG_ID_GROUP_START_LEN, MAVLINK_MSG_ID_GROUP_START_CRC);
#endif
}
#endif

#endif

// MESSAGE GROUP_START UNPACKING


/**
 * @brief Get field group_id from group_start message
 *
 * @return  Mission-unique group id (from MAV_CMD_GROUP_START).
 */
static inline uint32_t mavlink_msg_group_start_get_group_id(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  8);
}

/**
 * @brief Get field mission_checksum from group_start message
 *
 * @return  CRC32 checksum of current plan for MAV_MISSION_TYPE_ALL. As defined in MISSION_CHECKSUM message.
 */
static inline uint32_t mavlink_msg_group_start_get_mission_checksum(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint32_t(msg,  12);
}

/**
 * @brief Get field time_usec from group_start message
 *
 * @return [us] Timestamp (UNIX Epoch time or time since system boot).
        The receiving end can infer timestamp format (since 1.1.1970 or since system boot) by checking for the magnitude of the number.
 */
static inline uint64_t mavlink_msg_group_start_get_time_usec(const mavlink_message_t* msg)
{
    return _MAV_RETURN_uint64_t(msg,  0);
}

/**
 * @brief Decode a group_start message into a struct
 *
 * @param msg The message to decode
 * @param group_start C-struct to decode the message contents into
 */
static inline void mavlink_msg_group_start_decode(const mavlink_message_t* msg, mavlink_group_start_t* group_start)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
    group_start->time_usec = mavlink_msg_group_start_get_time_usec(msg);
    group_start->group_id = mavlink_msg_group_start_get_group_id(msg);
    group_start->mission_checksum = mavlink_msg_group_start_get_mission_checksum(msg);
#else
        uint8_t len = msg->len < MAVLINK_MSG_ID_GROUP_START_LEN? msg->len : MAVLINK_MSG_ID_GROUP_START_LEN;
        memset(group_start, 0, MAVLINK_MSG_ID_GROUP_START_LEN);
    memcpy(group_start, _MAV_PAYLOAD(msg), len);
#endif
}
