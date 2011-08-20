// MESSAGE WATCHDOG_COMMAND PACKING

#define MAVLINK_MSG_ID_WATCHDOG_COMMAND 153
#define MAVLINK_MSG_ID_WATCHDOG_COMMAND_LEN 6
#define MAVLINK_MSG_153_LEN 6
#define MAVLINK_MSG_ID_WATCHDOG_COMMAND_KEY 0xA9
#define MAVLINK_MSG_153_KEY 0xA9

typedef struct __mavlink_watchdog_command_t 
{
	uint16_t watchdog_id;	///< Watchdog ID
	uint16_t process_id;	///< Process ID
	uint8_t target_system_id;	///< Target system ID
	uint8_t command_id;	///< Command ID

} mavlink_watchdog_command_t;

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
static inline uint16_t mavlink_msg_watchdog_command_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t target_system_id, uint16_t watchdog_id, uint16_t process_id, uint8_t command_id)
{
	mavlink_watchdog_command_t *p = (mavlink_watchdog_command_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_WATCHDOG_COMMAND;

	p->target_system_id = target_system_id;	// uint8_t:Target system ID
	p->watchdog_id = watchdog_id;	// uint16_t:Watchdog ID
	p->process_id = process_id;	// uint16_t:Process ID
	p->command_id = command_id;	// uint8_t:Command ID

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_WATCHDOG_COMMAND_LEN);
}

/**
 * @brief Pack a watchdog_command message
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
static inline uint16_t mavlink_msg_watchdog_command_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t target_system_id, uint16_t watchdog_id, uint16_t process_id, uint8_t command_id)
{
	mavlink_watchdog_command_t *p = (mavlink_watchdog_command_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_WATCHDOG_COMMAND;

	p->target_system_id = target_system_id;	// uint8_t:Target system ID
	p->watchdog_id = watchdog_id;	// uint16_t:Watchdog ID
	p->process_id = process_id;	// uint16_t:Process ID
	p->command_id = command_id;	// uint8_t:Command ID

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_WATCHDOG_COMMAND_LEN);
}

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


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a watchdog_command message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system_id Target system ID
 * @param watchdog_id Watchdog ID
 * @param process_id Process ID
 * @param command_id Command ID
 */
static inline void mavlink_msg_watchdog_command_send(mavlink_channel_t chan, uint8_t target_system_id, uint16_t watchdog_id, uint16_t process_id, uint8_t command_id)
{
	mavlink_header_t hdr;
	mavlink_watchdog_command_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_WATCHDOG_COMMAND_LEN )
	payload.target_system_id = target_system_id;	// uint8_t:Target system ID
	payload.watchdog_id = watchdog_id;	// uint16_t:Watchdog ID
	payload.process_id = process_id;	// uint16_t:Process ID
	payload.command_id = command_id;	// uint8_t:Command ID

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_WATCHDOG_COMMAND_LEN;
	hdr.msgid = MAVLINK_MSG_ID_WATCHDOG_COMMAND;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0xA9, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
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
	mavlink_watchdog_command_t *p = (mavlink_watchdog_command_t *)&msg->payload[0];
	return (uint8_t)(p->target_system_id);
}

/**
 * @brief Get field watchdog_id from watchdog_command message
 *
 * @return Watchdog ID
 */
static inline uint16_t mavlink_msg_watchdog_command_get_watchdog_id(const mavlink_message_t* msg)
{
	mavlink_watchdog_command_t *p = (mavlink_watchdog_command_t *)&msg->payload[0];
	return (uint16_t)(p->watchdog_id);
}

/**
 * @brief Get field process_id from watchdog_command message
 *
 * @return Process ID
 */
static inline uint16_t mavlink_msg_watchdog_command_get_process_id(const mavlink_message_t* msg)
{
	mavlink_watchdog_command_t *p = (mavlink_watchdog_command_t *)&msg->payload[0];
	return (uint16_t)(p->process_id);
}

/**
 * @brief Get field command_id from watchdog_command message
 *
 * @return Command ID
 */
static inline uint8_t mavlink_msg_watchdog_command_get_command_id(const mavlink_message_t* msg)
{
	mavlink_watchdog_command_t *p = (mavlink_watchdog_command_t *)&msg->payload[0];
	return (uint8_t)(p->command_id);
}

/**
 * @brief Decode a watchdog_command message into a struct
 *
 * @param msg The message to decode
 * @param watchdog_command C-struct to decode the message contents into
 */
static inline void mavlink_msg_watchdog_command_decode(const mavlink_message_t* msg, mavlink_watchdog_command_t* watchdog_command)
{
	memcpy( watchdog_command, msg->payload, sizeof(mavlink_watchdog_command_t));
}
