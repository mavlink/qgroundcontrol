// MESSAGE EXTENDED_MESSAGE PACKING

#define MAVLINK_MSG_ID_EXTENDED_MESSAGE 255

typedef struct __mavlink_extended_message_t
{
 uint8_t target_system; ///< System which should execute the command
 uint8_t target_component; ///< Component which should execute the command, 0 for all components
 uint8_t protocol_flags; ///< Retransmission / ACK flags
} mavlink_extended_message_t;

#define MAVLINK_MSG_ID_EXTENDED_MESSAGE_LEN 3
#define MAVLINK_MSG_ID_255_LEN 3



#define MAVLINK_MESSAGE_INFO_EXTENDED_MESSAGE { \
	"EXTENDED_MESSAGE", \
	3, \
	{  { "target_system", MAVLINK_TYPE_UINT8_T, 0, 0, offsetof(mavlink_extended_message_t, target_system) }, \
         { "target_component", MAVLINK_TYPE_UINT8_T, 0, 1, offsetof(mavlink_extended_message_t, target_component) }, \
         { "protocol_flags", MAVLINK_TYPE_UINT8_T, 0, 2, offsetof(mavlink_extended_message_t, protocol_flags) }, \
         } \
}


/**
 * @brief Pack a extended_message message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system System which should execute the command
 * @param target_component Component which should execute the command, 0 for all components
 * @param protocol_flags Retransmission / ACK flags
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_extended_message_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint8_t target_system, uint8_t target_component, uint8_t protocol_flags)
{
	msg->msgid = MAVLINK_MSG_ID_EXTENDED_MESSAGE;

	put_uint8_t_by_index(msg, 0, target_system); // System which should execute the command
	put_uint8_t_by_index(msg, 1, target_component); // Component which should execute the command, 0 for all components
	put_uint8_t_by_index(msg, 2, protocol_flags); // Retransmission / ACK flags

	return mavlink_finalize_message(msg, system_id, component_id, 3, 247);
}

/**
 * @brief Pack a extended_message message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system System which should execute the command
 * @param target_component Component which should execute the command, 0 for all components
 * @param protocol_flags Retransmission / ACK flags
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_extended_message_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint8_t target_system,uint8_t target_component,uint8_t protocol_flags)
{
	msg->msgid = MAVLINK_MSG_ID_EXTENDED_MESSAGE;

	put_uint8_t_by_index(msg, 0, target_system); // System which should execute the command
	put_uint8_t_by_index(msg, 1, target_component); // Component which should execute the command, 0 for all components
	put_uint8_t_by_index(msg, 2, protocol_flags); // Retransmission / ACK flags

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, 3, 247);
}

/**
 * @brief Encode a extended_message struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param extended_message C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_extended_message_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_extended_message_t* extended_message)
{
	return mavlink_msg_extended_message_pack(system_id, component_id, msg, extended_message->target_system, extended_message->target_component, extended_message->protocol_flags);
}

/**
 * @brief Send a extended_message message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system System which should execute the command
 * @param target_component Component which should execute the command, 0 for all components
 * @param protocol_flags Retransmission / ACK flags
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_extended_message_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, uint8_t protocol_flags)
{
	MAVLINK_ALIGNED_MESSAGE(msg, 3);
	msg->msgid = MAVLINK_MSG_ID_EXTENDED_MESSAGE;

	put_uint8_t_by_index(msg, 0, target_system); // System which should execute the command
	put_uint8_t_by_index(msg, 1, target_component); // Component which should execute the command, 0 for all components
	put_uint8_t_by_index(msg, 2, protocol_flags); // Retransmission / ACK flags

	mavlink_finalize_message_chan_send(msg, chan, 3, 247);
}

#endif

// MESSAGE EXTENDED_MESSAGE UNPACKING


/**
 * @brief Get field target_system from extended_message message
 *
 * @return System which should execute the command
 */
static inline uint8_t mavlink_msg_extended_message_get_target_system(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  0);
}

/**
 * @brief Get field target_component from extended_message message
 *
 * @return Component which should execute the command, 0 for all components
 */
static inline uint8_t mavlink_msg_extended_message_get_target_component(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  1);
}

/**
 * @brief Get field protocol_flags from extended_message message
 *
 * @return Retransmission / ACK flags
 */
static inline uint8_t mavlink_msg_extended_message_get_protocol_flags(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  2);
}

/**
 * @brief Decode a extended_message message into a struct
 *
 * @param msg The message to decode
 * @param extended_message C-struct to decode the message contents into
 */
static inline void mavlink_msg_extended_message_decode(const mavlink_message_t* msg, mavlink_extended_message_t* extended_message)
{
#if MAVLINK_NEED_BYTE_SWAP
	extended_message->target_system = mavlink_msg_extended_message_get_target_system(msg);
	extended_message->target_component = mavlink_msg_extended_message_get_target_component(msg);
	extended_message->protocol_flags = mavlink_msg_extended_message_get_protocol_flags(msg);
#else
	memcpy(extended_message, MAVLINK_PAYLOAD(msg), 3);
#endif
}
