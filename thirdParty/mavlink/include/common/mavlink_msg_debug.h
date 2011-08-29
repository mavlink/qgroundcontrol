// MESSAGE DEBUG PACKING

#define MAVLINK_MSG_ID_DEBUG 254

typedef struct __mavlink_debug_t
{
 uint32_t time_boot_ms; ///< Timestamp (milliseconds since system boot)
 float value; ///< DEBUG value
 uint8_t ind; ///< index of debug variable
} mavlink_debug_t;

#define MAVLINK_MSG_ID_DEBUG_LEN 9
#define MAVLINK_MSG_ID_254_LEN 9



#define MAVLINK_MESSAGE_INFO_DEBUG { \
	"DEBUG", \
	3, \
	{  { "time_boot_ms", MAVLINK_TYPE_UINT32_T, 0, 0, offsetof(mavlink_debug_t, time_boot_ms) }, \
         { "value", MAVLINK_TYPE_FLOAT, 0, 4, offsetof(mavlink_debug_t, value) }, \
         { "ind", MAVLINK_TYPE_UINT8_T, 0, 8, offsetof(mavlink_debug_t, ind) }, \
         } \
}


/**
 * @brief Pack a debug message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_boot_ms Timestamp (milliseconds since system boot)
 * @param ind index of debug variable
 * @param value DEBUG value
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_debug_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint32_t time_boot_ms, uint8_t ind, float value)
{
	msg->msgid = MAVLINK_MSG_ID_DEBUG;

	put_uint32_t_by_index(msg, 0, time_boot_ms); // Timestamp (milliseconds since system boot)
	put_float_by_index(msg, 4, value); // DEBUG value
	put_uint8_t_by_index(msg, 8, ind); // index of debug variable

	return mavlink_finalize_message(msg, system_id, component_id, 9, 46);
}

/**
 * @brief Pack a debug message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param time_boot_ms Timestamp (milliseconds since system boot)
 * @param ind index of debug variable
 * @param value DEBUG value
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_debug_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint32_t time_boot_ms,uint8_t ind,float value)
{
	msg->msgid = MAVLINK_MSG_ID_DEBUG;

	put_uint32_t_by_index(msg, 0, time_boot_ms); // Timestamp (milliseconds since system boot)
	put_float_by_index(msg, 4, value); // DEBUG value
	put_uint8_t_by_index(msg, 8, ind); // index of debug variable

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, 9, 46);
}

/**
 * @brief Encode a debug struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param debug C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_debug_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_debug_t* debug)
{
	return mavlink_msg_debug_pack(system_id, component_id, msg, debug->time_boot_ms, debug->ind, debug->value);
}

/**
 * @brief Send a debug message
 * @param chan MAVLink channel to send the message
 *
 * @param time_boot_ms Timestamp (milliseconds since system boot)
 * @param ind index of debug variable
 * @param value DEBUG value
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_debug_send(mavlink_channel_t chan, uint32_t time_boot_ms, uint8_t ind, float value)
{
	MAVLINK_ALIGNED_MESSAGE(msg, 9);
	msg->msgid = MAVLINK_MSG_ID_DEBUG;

	put_uint32_t_by_index(msg, 0, time_boot_ms); // Timestamp (milliseconds since system boot)
	put_float_by_index(msg, 4, value); // DEBUG value
	put_uint8_t_by_index(msg, 8, ind); // index of debug variable

	mavlink_finalize_message_chan_send(msg, chan, 9, 46);
}

#endif

// MESSAGE DEBUG UNPACKING


/**
 * @brief Get field time_boot_ms from debug message
 *
 * @return Timestamp (milliseconds since system boot)
 */
static inline uint32_t mavlink_msg_debug_get_time_boot_ms(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint32_t(msg,  0);
}

/**
 * @brief Get field ind from debug message
 *
 * @return index of debug variable
 */
static inline uint8_t mavlink_msg_debug_get_ind(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  8);
}

/**
 * @brief Get field value from debug message
 *
 * @return DEBUG value
 */
static inline float mavlink_msg_debug_get_value(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_float(msg,  4);
}

/**
 * @brief Decode a debug message into a struct
 *
 * @param msg The message to decode
 * @param debug C-struct to decode the message contents into
 */
static inline void mavlink_msg_debug_decode(const mavlink_message_t* msg, mavlink_debug_t* debug)
{
#if MAVLINK_NEED_BYTE_SWAP
	debug->time_boot_ms = mavlink_msg_debug_get_time_boot_ms(msg);
	debug->value = mavlink_msg_debug_get_value(msg);
	debug->ind = mavlink_msg_debug_get_ind(msg);
#else
	memcpy(debug, MAVLINK_PAYLOAD(msg), 9);
#endif
}
