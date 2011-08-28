// MESSAGE SET_SAFETY_MODE PACKING

#define MAVLINK_MSG_ID_SET_SAFETY_MODE 13

typedef struct __mavlink_set_safety_mode_t
{
 uint8_t target; ///< The system setting the mode
 uint8_t safety_mode; ///< The new safety mode. The MAV will reject some mode changes during flight.
} mavlink_set_safety_mode_t;

#define MAVLINK_MSG_ID_SET_SAFETY_MODE_LEN 2
#define MAVLINK_MSG_ID_13_LEN 2



#define MAVLINK_MESSAGE_INFO_SET_SAFETY_MODE { \
	"SET_SAFETY_MODE", \
	2, \
	{  { "target", MAVLINK_TYPE_UINT8_T, 0, 0, offsetof(mavlink_set_safety_mode_t, target) }, \
         { "safety_mode", MAVLINK_TYPE_UINT8_T, 0, 1, offsetof(mavlink_set_safety_mode_t, safety_mode) }, \
         } \
}


/**
 * @brief Pack a set_safety_mode message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target The system setting the mode
 * @param safety_mode The new safety mode. The MAV will reject some mode changes during flight.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_set_safety_mode_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint8_t target, uint8_t safety_mode)
{
	msg->msgid = MAVLINK_MSG_ID_SET_SAFETY_MODE;

	put_uint8_t_by_index(msg, 0, target); // The system setting the mode
	put_uint8_t_by_index(msg, 1, safety_mode); // The new safety mode. The MAV will reject some mode changes during flight.

	return mavlink_finalize_message(msg, system_id, component_id, 2, 8);
}

/**
 * @brief Pack a set_safety_mode message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target The system setting the mode
 * @param safety_mode The new safety mode. The MAV will reject some mode changes during flight.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_set_safety_mode_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint8_t target,uint8_t safety_mode)
{
	msg->msgid = MAVLINK_MSG_ID_SET_SAFETY_MODE;

	put_uint8_t_by_index(msg, 0, target); // The system setting the mode
	put_uint8_t_by_index(msg, 1, safety_mode); // The new safety mode. The MAV will reject some mode changes during flight.

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, 2, 8);
}

#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

/**
 * @brief Pack a set_safety_mode message on a channel and send
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target The system setting the mode
 * @param safety_mode The new safety mode. The MAV will reject some mode changes during flight.
 */
static inline void mavlink_msg_set_safety_mode_pack_chan_send(mavlink_channel_t chan,
							   mavlink_message_t* msg,
						           uint8_t target,uint8_t safety_mode)
{
	msg->msgid = MAVLINK_MSG_ID_SET_SAFETY_MODE;

	put_uint8_t_by_index(msg, 0, target); // The system setting the mode
	put_uint8_t_by_index(msg, 1, safety_mode); // The new safety mode. The MAV will reject some mode changes during flight.

	mavlink_finalize_message_chan_send(msg, chan, 2, 8);
}
#endif // MAVLINK_USE_CONVENIENCE_FUNCTIONS


/**
 * @brief Encode a set_safety_mode struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param set_safety_mode C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_set_safety_mode_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_set_safety_mode_t* set_safety_mode)
{
	return mavlink_msg_set_safety_mode_pack(system_id, component_id, msg, set_safety_mode->target, set_safety_mode->safety_mode);
}

/**
 * @brief Send a set_safety_mode message
 * @param chan MAVLink channel to send the message
 *
 * @param target The system setting the mode
 * @param safety_mode The new safety mode. The MAV will reject some mode changes during flight.
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_set_safety_mode_send(mavlink_channel_t chan, uint8_t target, uint8_t safety_mode)
{
	MAVLINK_ALIGNED_MESSAGE(msg, 2);
	mavlink_msg_set_safety_mode_pack_chan_send(chan, msg, target, safety_mode);
}

#endif

// MESSAGE SET_SAFETY_MODE UNPACKING


/**
 * @brief Get field target from set_safety_mode message
 *
 * @return The system setting the mode
 */
static inline uint8_t mavlink_msg_set_safety_mode_get_target(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  0);
}

/**
 * @brief Get field safety_mode from set_safety_mode message
 *
 * @return The new safety mode. The MAV will reject some mode changes during flight.
 */
static inline uint8_t mavlink_msg_set_safety_mode_get_safety_mode(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  1);
}

/**
 * @brief Decode a set_safety_mode message into a struct
 *
 * @param msg The message to decode
 * @param set_safety_mode C-struct to decode the message contents into
 */
static inline void mavlink_msg_set_safety_mode_decode(const mavlink_message_t* msg, mavlink_set_safety_mode_t* set_safety_mode)
{
#if MAVLINK_NEED_BYTE_SWAP
	set_safety_mode->target = mavlink_msg_set_safety_mode_get_target(msg);
	set_safety_mode->safety_mode = mavlink_msg_set_safety_mode_get_safety_mode(msg);
#else
	memcpy(set_safety_mode, MAVLINK_PAYLOAD(msg), 2);
#endif
}
