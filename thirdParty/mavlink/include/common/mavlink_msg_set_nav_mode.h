// MESSAGE SET_NAV_MODE PACKING

#define MAVLINK_MSG_ID_SET_NAV_MODE 12

typedef struct __mavlink_set_nav_mode_t
{
 uint8_t target; ///< The system setting the mode
 uint8_t nav_mode; ///< The new navigation mode
} mavlink_set_nav_mode_t;

/**
 * @brief Pack a set_nav_mode message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target The system setting the mode
 * @param nav_mode The new navigation mode
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_set_nav_mode_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint8_t target, uint8_t nav_mode)
{
	msg->msgid = MAVLINK_MSG_ID_SET_NAV_MODE;

	put_uint8_t_by_index(target, 0,  msg->payload); // The system setting the mode
	put_uint8_t_by_index(nav_mode, 1,  msg->payload); // The new navigation mode

	return mavlink_finalize_message(msg, system_id, component_id, 2, 134);
}

/**
 * @brief Pack a set_nav_mode message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target The system setting the mode
 * @param nav_mode The new navigation mode
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_set_nav_mode_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint8_t target,uint8_t nav_mode)
{
	msg->msgid = MAVLINK_MSG_ID_SET_NAV_MODE;

	put_uint8_t_by_index(target, 0,  msg->payload); // The system setting the mode
	put_uint8_t_by_index(nav_mode, 1,  msg->payload); // The new navigation mode

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, 2, 134);
}

#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

/**
 * @brief Pack a set_nav_mode message on a channel and send
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target The system setting the mode
 * @param nav_mode The new navigation mode
 */
static inline void mavlink_msg_set_nav_mode_pack_chan_send(mavlink_channel_t chan,
							   mavlink_message_t* msg,
						           uint8_t target,uint8_t nav_mode)
{
	msg->msgid = MAVLINK_MSG_ID_SET_NAV_MODE;

	put_uint8_t_by_index(target, 0,  msg->payload); // The system setting the mode
	put_uint8_t_by_index(nav_mode, 1,  msg->payload); // The new navigation mode

	mavlink_finalize_message_chan_send(msg, chan, 2, 134);
}
#endif // MAVLINK_USE_CONVENIENCE_FUNCTIONS


/**
 * @brief Encode a set_nav_mode struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param set_nav_mode C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_set_nav_mode_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_set_nav_mode_t* set_nav_mode)
{
	return mavlink_msg_set_nav_mode_pack(system_id, component_id, msg, set_nav_mode->target, set_nav_mode->nav_mode);
}

/**
 * @brief Send a set_nav_mode message
 * @param chan MAVLink channel to send the message
 *
 * @param target The system setting the mode
 * @param nav_mode The new navigation mode
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_set_nav_mode_send(mavlink_channel_t chan, uint8_t target, uint8_t nav_mode)
{
	MAVLINK_ALIGNED_MESSAGE(msg, 2);
	mavlink_msg_set_nav_mode_pack_chan_send(chan, msg, target, nav_mode);
}

#endif

// MESSAGE SET_NAV_MODE UNPACKING


/**
 * @brief Get field target from set_nav_mode message
 *
 * @return The system setting the mode
 */
static inline uint8_t mavlink_msg_set_nav_mode_get_target(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  0);
}

/**
 * @brief Get field nav_mode from set_nav_mode message
 *
 * @return The new navigation mode
 */
static inline uint8_t mavlink_msg_set_nav_mode_get_nav_mode(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  1);
}

/**
 * @brief Decode a set_nav_mode message into a struct
 *
 * @param msg The message to decode
 * @param set_nav_mode C-struct to decode the message contents into
 */
static inline void mavlink_msg_set_nav_mode_decode(const mavlink_message_t* msg, mavlink_set_nav_mode_t* set_nav_mode)
{
#if MAVLINK_NEED_BYTE_SWAP
	set_nav_mode->target = mavlink_msg_set_nav_mode_get_target(msg);
	set_nav_mode->nav_mode = mavlink_msg_set_nav_mode_get_nav_mode(msg);
#else
	memcpy(set_nav_mode, msg->payload, 2);
#endif
}
