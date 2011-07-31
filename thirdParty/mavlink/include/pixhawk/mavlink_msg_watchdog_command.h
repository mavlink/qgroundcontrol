// MESSAGE WATCHDOG_COMMAND PACKING

#define MAVLINK_MSG_ID_WATCHDOG_COMMAND 153
#define MAVLINK_MSG_ID_WATCHDOG_COMMAND_LEN 6
#define MAVLINK_MSG_153_LEN 6

typedef struct __mavlink_watchdog_command_t 
{
	uint8_t target_system_id; ///< Target system ID
	uint16_t watchdog_id; ///< Watchdog ID
	uint16_t process_id; ///< Process ID
	uint8_t command_id; ///< Command ID

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

	p->target_system_id = target_system_id; // uint8_t:Target system ID
	p->watchdog_id = watchdog_id; // uint16_t:Watchdog ID
	p->process_id = process_id; // uint16_t:Process ID
	p->command_id = command_id; // uint8_t:Command ID

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

	p->target_system_id = target_system_id; // uint8_t:Target system ID
	p->watchdog_id = watchdog_id; // uint16_t:Watchdog ID
	p->process_id = process_id; // uint16_t:Process ID
	p->command_id = command_id; // uint8_t:Command ID

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
	mavlink_message_t msg;
	uint16_t checksum;
	mavlink_watchdog_command_t *p = (mavlink_watchdog_command_t *)&msg.payload[0];

	p->target_system_id = target_system_id; // uint8_t:Target system ID
	p->watchdog_id = watchdog_id; // uint16_t:Watchdog ID
	p->process_id = process_id; // uint16_t:Process ID
	p->command_id = command_id; // uint8_t:Command ID

	msg.STX = MAVLINK_STX;
	msg.len = MAVLINK_MSG_ID_WATCHDOG_COMMAND_LEN;
	msg.msgid = MAVLINK_MSG_ID_WATCHDOG_COMMAND;
	msg.sysid = mavlink_system.sysid;
	msg.compid = mavlink_system.compid;
	msg.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = msg.seq + 1;
	checksum = crc_calculate_msg(&msg, msg.len + MAVLINK_CORE_HEADER_LEN);
	msg.ck_a = (uint8_t)(checksum & 0xFF); ///< Low byte
	msg.ck_b = (uint8_t)(checksum >> 8); ///< High byte

	mavlink_send_msg(chan, &msg);
}

#endif

#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS_SMALL
static inline void mavlink_msg_watchdog_command_send(mavlink_channel_t chan, uint8_t target_system_id, uint16_t watchdog_id, uint16_t process_id, uint8_t command_id)
{
	mavlink_header_t hdr;
	mavlink_watchdog_command_t payload;
	uint16_t checksum;
	mavlink_watchdog_command_t *p = &payload;

	p->target_system_id = target_system_id; // uint8_t:Target system ID
	p->watchdog_id = watchdog_id; // uint16_t:Watchdog ID
	p->process_id = process_id; // uint16_t:Process ID
	p->command_id = command_id; // uint8_t:Command ID

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_WATCHDOG_COMMAND_LEN;
	hdr.msgid = MAVLINK_MSG_ID_WATCHDOG_COMMAND;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );

	crc_init(&checksum);
	checksum = crc_calculate_mem((uint8_t *)&hdr.len, &checksum, MAVLINK_CORE_HEADER_LEN);
	checksum = crc_calculate_mem((uint8_t *)&payload, &checksum, hdr.len );
	hdr.ck_a = (uint8_t)(checksum & 0xFF); ///< Low byte
	hdr.ck_b = (uint8_t)(checksum >> 8); ///< High byte

	mavlink_send_mem(chan, (uint8_t *)&payload, hdr.len);
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck_a, MAVLINK_NUM_CHECKSUM_BYTES);
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
