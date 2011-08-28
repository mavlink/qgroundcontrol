// MESSAGE COMMAND_ACK PACKING

#define MAVLINK_MSG_ID_COMMAND_ACK 77

typedef struct __mavlink_command_ack_t
{
 float command; ///< Current airspeed in m/s
 float result; ///< 1: Action ACCEPTED and EXECUTED, 1: Action TEMPORARY REJECTED/DENIED, 2: Action PERMANENTLY DENIED, 3: Action UNKNOWN/UNSUPPORTED, 4: Requesting CONFIRMATION
} mavlink_command_ack_t;

#define MAVLINK_MSG_ID_COMMAND_ACK_LEN 8
#define MAVLINK_MSG_ID_77_LEN 8



#define MAVLINK_MESSAGE_INFO_COMMAND_ACK { \
	"COMMAND_ACK", \
	2, \
	{  { "command", MAVLINK_TYPE_FLOAT, 0, 0, offsetof(mavlink_command_ack_t, command) }, \
         { "result", MAVLINK_TYPE_FLOAT, 0, 4, offsetof(mavlink_command_ack_t, result) }, \
         } \
}


/**
 * @brief Pack a command_ack message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param command Current airspeed in m/s
 * @param result 1: Action ACCEPTED and EXECUTED, 1: Action TEMPORARY REJECTED/DENIED, 2: Action PERMANENTLY DENIED, 3: Action UNKNOWN/UNSUPPORTED, 4: Requesting CONFIRMATION
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_command_ack_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       float command, float result)
{
	msg->msgid = MAVLINK_MSG_ID_COMMAND_ACK;

	put_float_by_index(msg, 0, command); // Current airspeed in m/s
	put_float_by_index(msg, 4, result); // 1: Action ACCEPTED and EXECUTED, 1: Action TEMPORARY REJECTED/DENIED, 2: Action PERMANENTLY DENIED, 3: Action UNKNOWN/UNSUPPORTED, 4: Requesting CONFIRMATION

	return mavlink_finalize_message(msg, system_id, component_id, 8, 8);
}

/**
 * @brief Pack a command_ack message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param command Current airspeed in m/s
 * @param result 1: Action ACCEPTED and EXECUTED, 1: Action TEMPORARY REJECTED/DENIED, 2: Action PERMANENTLY DENIED, 3: Action UNKNOWN/UNSUPPORTED, 4: Requesting CONFIRMATION
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_command_ack_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           float command,float result)
{
	msg->msgid = MAVLINK_MSG_ID_COMMAND_ACK;

	put_float_by_index(msg, 0, command); // Current airspeed in m/s
	put_float_by_index(msg, 4, result); // 1: Action ACCEPTED and EXECUTED, 1: Action TEMPORARY REJECTED/DENIED, 2: Action PERMANENTLY DENIED, 3: Action UNKNOWN/UNSUPPORTED, 4: Requesting CONFIRMATION

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, 8, 8);
}

#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

/**
 * @brief Pack a command_ack message on a channel and send
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param command Current airspeed in m/s
 * @param result 1: Action ACCEPTED and EXECUTED, 1: Action TEMPORARY REJECTED/DENIED, 2: Action PERMANENTLY DENIED, 3: Action UNKNOWN/UNSUPPORTED, 4: Requesting CONFIRMATION
 */
static inline void mavlink_msg_command_ack_pack_chan_send(mavlink_channel_t chan,
							   mavlink_message_t* msg,
						           float command,float result)
{
	msg->msgid = MAVLINK_MSG_ID_COMMAND_ACK;

	put_float_by_index(msg, 0, command); // Current airspeed in m/s
	put_float_by_index(msg, 4, result); // 1: Action ACCEPTED and EXECUTED, 1: Action TEMPORARY REJECTED/DENIED, 2: Action PERMANENTLY DENIED, 3: Action UNKNOWN/UNSUPPORTED, 4: Requesting CONFIRMATION

	mavlink_finalize_message_chan_send(msg, chan, 8, 8);
}
#endif // MAVLINK_USE_CONVENIENCE_FUNCTIONS


/**
 * @brief Encode a command_ack struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param command_ack C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_command_ack_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_command_ack_t* command_ack)
{
	return mavlink_msg_command_ack_pack(system_id, component_id, msg, command_ack->command, command_ack->result);
}

/**
 * @brief Send a command_ack message
 * @param chan MAVLink channel to send the message
 *
 * @param command Current airspeed in m/s
 * @param result 1: Action ACCEPTED and EXECUTED, 1: Action TEMPORARY REJECTED/DENIED, 2: Action PERMANENTLY DENIED, 3: Action UNKNOWN/UNSUPPORTED, 4: Requesting CONFIRMATION
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_command_ack_send(mavlink_channel_t chan, float command, float result)
{
	MAVLINK_ALIGNED_MESSAGE(msg, 8);
	mavlink_msg_command_ack_pack_chan_send(chan, msg, command, result);
}

#endif

// MESSAGE COMMAND_ACK UNPACKING


/**
 * @brief Get field command from command_ack message
 *
 * @return Current airspeed in m/s
 */
static inline float mavlink_msg_command_ack_get_command(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_float(msg,  0);
}

/**
 * @brief Get field result from command_ack message
 *
 * @return 1: Action ACCEPTED and EXECUTED, 1: Action TEMPORARY REJECTED/DENIED, 2: Action PERMANENTLY DENIED, 3: Action UNKNOWN/UNSUPPORTED, 4: Requesting CONFIRMATION
 */
static inline float mavlink_msg_command_ack_get_result(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_float(msg,  4);
}

/**
 * @brief Decode a command_ack message into a struct
 *
 * @param msg The message to decode
 * @param command_ack C-struct to decode the message contents into
 */
static inline void mavlink_msg_command_ack_decode(const mavlink_message_t* msg, mavlink_command_ack_t* command_ack)
{
#if MAVLINK_NEED_BYTE_SWAP
	command_ack->command = mavlink_msg_command_ack_get_command(msg);
	command_ack->result = mavlink_msg_command_ack_get_result(msg);
#else
	memcpy(command_ack, MAVLINK_PAYLOAD(msg), 8);
#endif
}
