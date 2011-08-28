// MESSAGE WATCHDOG_COMMAND PACKING

#define MAVLINK_MSG_ID_WATCHDOG_COMMAND 183

typedef struct __mavlink_watchdog_command_t
{
 uint16_t watchdog_id; ///< Watchdog ID
 uint16_t process_id; ///< Process ID
 uint8_t target_system_id; ///< Target system ID
 uint8_t command_id; ///< Command ID
} mavlink_watchdog_command_t;

#define MAVLINK_MSG_ID_WATCHDOG_COMMAND_LEN 6
#define MAVLINK_MSG_ID_183_LEN 6



#define MAVLINK_MESSAGE_INFO_WATCHDOG_COMMAND { \
	"WATCHDOG_COMMAND", \
	4, \
	{  { "watchdog_id", MAVLINK_TYPE_UINT16_T, 0, 0, offsetof(mavlink_watchdog_command_t, watchdog_id) }, \
         { "process_id", MAVLINK_TYPE_UINT16_T, 0, 2, offsetof(mavlink_watchdog_command_t, process_id) }, \
         { "target_system_id", MAVLINK_TYPE_UINT8_T, 0, 4, offsetof(mavlink_watchdog_command_t, target_system_id) }, \
         { "command_id", MAVLINK_TYPE_UINT8_T, 0, 5, offsetof(mavlink_watchdog_command_t, command_id) }, \
         } \
}


/**
 * @brief Pack a watchdog_command message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system_id Target system ID
 * @param watchdog_id Watchdog ID
 * @param process_id Process ID
 * @param command_id Command ID
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_watchdog_command_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint8_t target_system_id, uint16_t watchdog_id, uint16_t process_id, uint8_t command_id)
{
	msg->msgid = MAVLINK_MSG_ID_WATCHDOG_COMMAND;

	put_uint16_t_by_index(msg, 0, watchdog_id); // Watchdog ID
	put_uint16_t_by_index(msg, 2, process_id); // Process ID
	put_uint8_t_by_index(msg, 4, target_system_id); // Target system ID
	put_uint8_t_by_index(msg, 5, command_id); // Command ID

	return mavlink_finalize_message(msg, system_id, component_id, 6, 162);
}

/**
 * @brief Pack a watchdog_command message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system_id Target system ID
 * @param watchdog_id Watchdog ID
 * @param process_id Process ID
 * @param command_id Command ID
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_watchdog_command_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint8_t target_system_id,uint16_t watchdog_id,uint16_t process_id,uint8_t command_id)
{
	msg->msgid = MAVLINK_MSG_ID_WATCHDOG_COMMAND;

	put_uint16_t_by_index(msg, 0, watchdog_id); // Watchdog ID
	put_uint16_t_by_index(msg, 2, process_id); // Process ID
	put_uint8_t_by_index(msg, 4, target_system_id); // Target system ID
	put_uint8_t_by_index(msg, 5, command_id); // Command ID

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, 6, 162);
}

#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

/**
 * @brief Pack a watchdog_command message on a channel and send
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system_id Target system ID
 * @param watchdog_id Watchdog ID
 * @param process_id Process ID
 * @param command_id Command ID
 */
static inline void mavlink_msg_watchdog_command_pack_chan_send(mavlink_channel_t chan,
							   mavlink_message_t* msg,
						           uint8_t target_system_id,uint16_t watchdog_id,uint16_t process_id,uint8_t command_id)
{
	msg->msgid = MAVLINK_MSG_ID_WATCHDOG_COMMAND;

	put_uint16_t_by_index(msg, 0, watchdog_id); // Watchdog ID
	put_uint16_t_by_index(msg, 2, process_id); // Process ID
	put_uint8_t_by_index(msg, 4, target_system_id); // Target system ID
	put_uint8_t_by_index(msg, 5, command_id); // Command ID

	mavlink_finalize_message_chan_send(msg, chan, 6, 162);
}
#endif // MAVLINK_USE_CONVENIENCE_FUNCTIONS


/**
 * @brief Encode a watchdog_command struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param watchdog_command C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_watchdog_command_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_watchdog_command_t* watchdog_command)
{
	return mavlink_msg_watchdog_command_pack(system_id, component_id, msg, watchdog_command->target_system_id, watchdog_command->watchdog_id, watchdog_command->process_id, watchdog_command->command_id);
}

/**
 * @brief Send a watchdog_command message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system_id Target system ID
 * @param watchdog_id Watchdog ID
 * @param process_id Process ID
 * @param command_id Command ID
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_watchdog_command_send(mavlink_channel_t chan, uint8_t target_system_id, uint16_t watchdog_id, uint16_t process_id, uint8_t command_id)
{
	MAVLINK_ALIGNED_MESSAGE(msg, 6);
	mavlink_msg_watchdog_command_pack_chan_send(chan, msg, target_system_id, watchdog_id, process_id, command_id);
}

#endif

// MESSAGE WATCHDOG_COMMAND UNPACKING


/**
 * @brief Get field target_system_id from watchdog_command message
 *
 * @return Target system ID
 */
static inline uint8_t mavlink_msg_watchdog_command_get_target_system_id(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  4);
}

/**
 * @brief Get field watchdog_id from watchdog_command message
 *
 * @return Watchdog ID
 */
static inline uint16_t mavlink_msg_watchdog_command_get_watchdog_id(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint16_t(msg,  0);
}

/**
 * @brief Get field process_id from watchdog_command message
 *
 * @return Process ID
 */
static inline uint16_t mavlink_msg_watchdog_command_get_process_id(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint16_t(msg,  2);
}

/**
 * @brief Get field command_id from watchdog_command message
 *
 * @return Command ID
 */
static inline uint8_t mavlink_msg_watchdog_command_get_command_id(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  5);
}

/**
 * @brief Decode a watchdog_command message into a struct
 *
 * @param msg The message to decode
 * @param watchdog_command C-struct to decode the message contents into
 */
static inline void mavlink_msg_watchdog_command_decode(const mavlink_message_t* msg, mavlink_watchdog_command_t* watchdog_command)
{
#if MAVLINK_NEED_BYTE_SWAP
	watchdog_command->watchdog_id = mavlink_msg_watchdog_command_get_watchdog_id(msg);
	watchdog_command->process_id = mavlink_msg_watchdog_command_get_process_id(msg);
	watchdog_command->target_system_id = mavlink_msg_watchdog_command_get_target_system_id(msg);
	watchdog_command->command_id = mavlink_msg_watchdog_command_get_command_id(msg);
#else
	memcpy(watchdog_command, MAVLINK_PAYLOAD(msg), 6);
#endif
}
