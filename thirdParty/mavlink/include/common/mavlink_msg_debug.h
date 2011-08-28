// MESSAGE DEBUG PACKING

#define MAVLINK_MSG_ID_DEBUG 255

typedef struct __mavlink_debug_t
{
 float value; ///< DEBUG value
 uint8_t ind; ///< index of debug variable
} mavlink_debug_t;

#define MAVLINK_MSG_ID_DEBUG_LEN 5
#define MAVLINK_MSG_ID_255_LEN 5



#define MAVLINK_MESSAGE_INFO_DEBUG { \
	"DEBUG", \
	2, \
	{  { "value", MAVLINK_TYPE_FLOAT, 0, 0, offsetof(mavlink_debug_t, value) }, \
         { "ind", MAVLINK_TYPE_UINT8_T, 0, 4, offsetof(mavlink_debug_t, ind) }, \
         } \
}


/**
 * @brief Pack a debug message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param ind index of debug variable
 * @param value DEBUG value
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_debug_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint8_t ind, float value)
{
	msg->msgid = MAVLINK_MSG_ID_DEBUG;

	put_float_by_index(msg, 0, value); // DEBUG value
	put_uint8_t_by_index(msg, 4, ind); // index of debug variable

	return mavlink_finalize_message(msg, system_id, component_id, 5, 127);
}

/**
 * @brief Pack a debug message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param ind index of debug variable
 * @param value DEBUG value
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_debug_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint8_t ind,float value)
{
	msg->msgid = MAVLINK_MSG_ID_DEBUG;

	put_float_by_index(msg, 0, value); // DEBUG value
	put_uint8_t_by_index(msg, 4, ind); // index of debug variable

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, 5, 127);
}

#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

/**
 * @brief Pack a debug message on a channel and send
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param ind index of debug variable
 * @param value DEBUG value
 */
static inline void mavlink_msg_debug_pack_chan_send(mavlink_channel_t chan,
							   mavlink_message_t* msg,
						           uint8_t ind,float value)
{
	msg->msgid = MAVLINK_MSG_ID_DEBUG;

	put_float_by_index(msg, 0, value); // DEBUG value
	put_uint8_t_by_index(msg, 4, ind); // index of debug variable

	mavlink_finalize_message_chan_send(msg, chan, 5, 127);
}
#endif // MAVLINK_USE_CONVENIENCE_FUNCTIONS


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
	return mavlink_msg_debug_pack(system_id, component_id, msg, debug->ind, debug->value);
}

/**
 * @brief Send a debug message
 * @param chan MAVLink channel to send the message
 *
 * @param ind index of debug variable
 * @param value DEBUG value
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_debug_send(mavlink_channel_t chan, uint8_t ind, float value)
{
	MAVLINK_ALIGNED_MESSAGE(msg, 5);
	mavlink_msg_debug_pack_chan_send(chan, msg, ind, value);
}

#endif

// MESSAGE DEBUG UNPACKING


/**
 * @brief Get field ind from debug message
 *
 * @return index of debug variable
 */
static inline uint8_t mavlink_msg_debug_get_ind(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  4);
}

/**
 * @brief Get field value from debug message
 *
 * @return DEBUG value
 */
static inline float mavlink_msg_debug_get_value(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_float(msg,  0);
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
	debug->value = mavlink_msg_debug_get_value(msg);
	debug->ind = mavlink_msg_debug_get_ind(msg);
#else
	memcpy(debug, MAVLINK_PAYLOAD(msg), 5);
#endif
}
