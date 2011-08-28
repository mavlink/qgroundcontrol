// MESSAGE SET_ALTITUDE PACKING

#define MAVLINK_MSG_ID_SET_ALTITUDE 65

typedef struct __mavlink_set_altitude_t
{
 uint32_t mode; ///< The new altitude in meters
 uint8_t target; ///< The system setting the altitude
} mavlink_set_altitude_t;

/**
 * @brief Pack a set_altitude message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target The system setting the altitude
 * @param mode The new altitude in meters
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_set_altitude_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint8_t target, uint32_t mode)
{
	msg->msgid = MAVLINK_MSG_ID_SET_ALTITUDE;

	put_uint32_t_by_index(mode, 0,  msg->payload); // The new altitude in meters
	put_uint8_t_by_index(target, 4,  msg->payload); // The system setting the altitude

	return mavlink_finalize_message(msg, system_id, component_id, 5, 56);
}

/**
 * @brief Pack a set_altitude message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target The system setting the altitude
 * @param mode The new altitude in meters
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_set_altitude_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint8_t target,uint32_t mode)
{
	msg->msgid = MAVLINK_MSG_ID_SET_ALTITUDE;

	put_uint32_t_by_index(mode, 0,  msg->payload); // The new altitude in meters
	put_uint8_t_by_index(target, 4,  msg->payload); // The system setting the altitude

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, 5, 56);
}

#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

/**
 * @brief Pack a set_altitude message on a channel and send
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target The system setting the altitude
 * @param mode The new altitude in meters
 */
static inline void mavlink_msg_set_altitude_pack_chan_send(mavlink_channel_t chan,
							   mavlink_message_t* msg,
						           uint8_t target,uint32_t mode)
{
	msg->msgid = MAVLINK_MSG_ID_SET_ALTITUDE;

	put_uint32_t_by_index(mode, 0,  msg->payload); // The new altitude in meters
	put_uint8_t_by_index(target, 4,  msg->payload); // The system setting the altitude

	mavlink_finalize_message_chan_send(msg, chan, 5, 56);
}
#endif // MAVLINK_USE_CONVENIENCE_FUNCTIONS


/**
 * @brief Encode a set_altitude struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param set_altitude C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_set_altitude_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_set_altitude_t* set_altitude)
{
	return mavlink_msg_set_altitude_pack(system_id, component_id, msg, set_altitude->target, set_altitude->mode);
}

/**
 * @brief Send a set_altitude message
 * @param chan MAVLink channel to send the message
 *
 * @param target The system setting the altitude
 * @param mode The new altitude in meters
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_set_altitude_send(mavlink_channel_t chan, uint8_t target, uint32_t mode)
{
	MAVLINK_ALIGNED_MESSAGE(msg, 5);
	mavlink_msg_set_altitude_pack_chan_send(chan, msg, target, mode);
}

#endif

// MESSAGE SET_ALTITUDE UNPACKING


/**
 * @brief Get field target from set_altitude message
 *
 * @return The system setting the altitude
 */
static inline uint8_t mavlink_msg_set_altitude_get_target(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  4);
}

/**
 * @brief Get field mode from set_altitude message
 *
 * @return The new altitude in meters
 */
static inline uint32_t mavlink_msg_set_altitude_get_mode(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint32_t(msg,  0);
}

/**
 * @brief Decode a set_altitude message into a struct
 *
 * @param msg The message to decode
 * @param set_altitude C-struct to decode the message contents into
 */
static inline void mavlink_msg_set_altitude_decode(const mavlink_message_t* msg, mavlink_set_altitude_t* set_altitude)
{
#if MAVLINK_NEED_BYTE_SWAP
	set_altitude->mode = mavlink_msg_set_altitude_get_mode(msg);
	set_altitude->target = mavlink_msg_set_altitude_get_target(msg);
#else
	memcpy(set_altitude, msg->payload, 5);
#endif
}
