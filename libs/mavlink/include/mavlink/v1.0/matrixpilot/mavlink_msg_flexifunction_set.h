// MESSAGE FLEXIFUNCTION_SET PACKING

#define MAVLINK_MSG_ID_FLEXIFUNCTION_SET 150

typedef struct __mavlink_flexifunction_set_t
{
 uint8_t target_system; ///< System ID
 uint8_t target_component; ///< Component ID
} mavlink_flexifunction_set_t;

#define MAVLINK_MSG_ID_FLEXIFUNCTION_SET_LEN 2
#define MAVLINK_MSG_ID_150_LEN 2

#define MAVLINK_MSG_ID_FLEXIFUNCTION_SET_CRC 181
#define MAVLINK_MSG_ID_150_CRC 181



#define MAVLINK_MESSAGE_INFO_FLEXIFUNCTION_SET { \
	"FLEXIFUNCTION_SET", \
	2, \
	{  { "target_system", NULL, MAVLINK_TYPE_UINT8_T, 0, 0, offsetof(mavlink_flexifunction_set_t, target_system) }, \
         { "target_component", NULL, MAVLINK_TYPE_UINT8_T, 0, 1, offsetof(mavlink_flexifunction_set_t, target_component) }, \
         } \
}


/**
 * @brief Pack a flexifunction_set message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_flexifunction_set_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint8_t target_system, uint8_t target_component)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_FLEXIFUNCTION_SET_LEN];
	_mav_put_uint8_t(buf, 0, target_system);
	_mav_put_uint8_t(buf, 1, target_component);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_FLEXIFUNCTION_SET_LEN);
#else
	mavlink_flexifunction_set_t packet;
	packet.target_system = target_system;
	packet.target_component = target_component;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_FLEXIFUNCTION_SET_LEN);
#endif

	msg->msgid = MAVLINK_MSG_ID_FLEXIFUNCTION_SET;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_FLEXIFUNCTION_SET_LEN, MAVLINK_MSG_ID_FLEXIFUNCTION_SET_CRC);
#else
    return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_FLEXIFUNCTION_SET_LEN);
#endif
}

/**
 * @brief Pack a flexifunction_set message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system System ID
 * @param target_component Component ID
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_flexifunction_set_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint8_t target_system,uint8_t target_component)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_FLEXIFUNCTION_SET_LEN];
	_mav_put_uint8_t(buf, 0, target_system);
	_mav_put_uint8_t(buf, 1, target_component);

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_FLEXIFUNCTION_SET_LEN);
#else
	mavlink_flexifunction_set_t packet;
	packet.target_system = target_system;
	packet.target_component = target_component;

        memcpy(_MAV_PAYLOAD_NON_CONST(msg), &packet, MAVLINK_MSG_ID_FLEXIFUNCTION_SET_LEN);
#endif

	msg->msgid = MAVLINK_MSG_ID_FLEXIFUNCTION_SET;
#if MAVLINK_CRC_EXTRA
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_FLEXIFUNCTION_SET_LEN, MAVLINK_MSG_ID_FLEXIFUNCTION_SET_CRC);
#else
    return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_FLEXIFUNCTION_SET_LEN);
#endif
}

/**
 * @brief Encode a flexifunction_set struct
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param flexifunction_set C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_flexifunction_set_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_flexifunction_set_t* flexifunction_set)
{
	return mavlink_msg_flexifunction_set_pack(system_id, component_id, msg, flexifunction_set->target_system, flexifunction_set->target_component);
}

/**
 * @brief Encode a flexifunction_set struct on a channel
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message will be sent over
 * @param msg The MAVLink message to compress the data into
 * @param flexifunction_set C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_flexifunction_set_encode_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const mavlink_flexifunction_set_t* flexifunction_set)
{
	return mavlink_msg_flexifunction_set_pack_chan(system_id, component_id, chan, msg, flexifunction_set->target_system, flexifunction_set->target_component);
}

/**
 * @brief Send a flexifunction_set message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system System ID
 * @param target_component Component ID
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_flexifunction_set_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[MAVLINK_MSG_ID_FLEXIFUNCTION_SET_LEN];
	_mav_put_uint8_t(buf, 0, target_system);
	_mav_put_uint8_t(buf, 1, target_component);

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_FLEXIFUNCTION_SET, buf, MAVLINK_MSG_ID_FLEXIFUNCTION_SET_LEN, MAVLINK_MSG_ID_FLEXIFUNCTION_SET_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_FLEXIFUNCTION_SET, buf, MAVLINK_MSG_ID_FLEXIFUNCTION_SET_LEN);
#endif
#else
	mavlink_flexifunction_set_t packet;
	packet.target_system = target_system;
	packet.target_component = target_component;

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_FLEXIFUNCTION_SET, (const char *)&packet, MAVLINK_MSG_ID_FLEXIFUNCTION_SET_LEN, MAVLINK_MSG_ID_FLEXIFUNCTION_SET_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_FLEXIFUNCTION_SET, (const char *)&packet, MAVLINK_MSG_ID_FLEXIFUNCTION_SET_LEN);
#endif
#endif
}

#if MAVLINK_MSG_ID_FLEXIFUNCTION_SET_LEN <= MAVLINK_MAX_PAYLOAD_LEN
/*
  This varient of _send() can be used to save stack space by re-using
  memory from the receive buffer.  The caller provides a
  mavlink_message_t which is the size of a full mavlink message. This
  is usually the receive buffer for the channel, and allows a reply to an
  incoming message with minimum stack space usage.
 */
static inline void mavlink_msg_flexifunction_set_send_buf(mavlink_message_t *msgbuf, mavlink_channel_t chan,  uint8_t target_system, uint8_t target_component)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char *buf = (char *)msgbuf;
	_mav_put_uint8_t(buf, 0, target_system);
	_mav_put_uint8_t(buf, 1, target_component);

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_FLEXIFUNCTION_SET, buf, MAVLINK_MSG_ID_FLEXIFUNCTION_SET_LEN, MAVLINK_MSG_ID_FLEXIFUNCTION_SET_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_FLEXIFUNCTION_SET, buf, MAVLINK_MSG_ID_FLEXIFUNCTION_SET_LEN);
#endif
#else
	mavlink_flexifunction_set_t *packet = (mavlink_flexifunction_set_t *)msgbuf;
	packet->target_system = target_system;
	packet->target_component = target_component;

#if MAVLINK_CRC_EXTRA
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_FLEXIFUNCTION_SET, (const char *)packet, MAVLINK_MSG_ID_FLEXIFUNCTION_SET_LEN, MAVLINK_MSG_ID_FLEXIFUNCTION_SET_CRC);
#else
    _mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_FLEXIFUNCTION_SET, (const char *)packet, MAVLINK_MSG_ID_FLEXIFUNCTION_SET_LEN);
#endif
#endif
}
#endif

#endif

// MESSAGE FLEXIFUNCTION_SET UNPACKING


/**
 * @brief Get field target_system from flexifunction_set message
 *
 * @return System ID
 */
static inline uint8_t mavlink_msg_flexifunction_set_get_target_system(const mavlink_message_t* msg)
{
	return _MAV_RETURN_uint8_t(msg,  0);
}

/**
 * @brief Get field target_component from flexifunction_set message
 *
 * @return Component ID
 */
static inline uint8_t mavlink_msg_flexifunction_set_get_target_component(const mavlink_message_t* msg)
{
	return _MAV_RETURN_uint8_t(msg,  1);
}

/**
 * @brief Decode a flexifunction_set message into a struct
 *
 * @param msg The message to decode
 * @param flexifunction_set C-struct to decode the message contents into
 */
static inline void mavlink_msg_flexifunction_set_decode(const mavlink_message_t* msg, mavlink_flexifunction_set_t* flexifunction_set)
{
#if MAVLINK_NEED_BYTE_SWAP
	flexifunction_set->target_system = mavlink_msg_flexifunction_set_get_target_system(msg);
	flexifunction_set->target_component = mavlink_msg_flexifunction_set_get_target_component(msg);
#else
	memcpy(flexifunction_set, _MAV_PAYLOAD(msg), MAVLINK_MSG_ID_FLEXIFUNCTION_SET_LEN);
#endif
}
