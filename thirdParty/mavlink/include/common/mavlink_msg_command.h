// MESSAGE COMMAND PACKING

#define MAVLINK_MSG_ID_COMMAND 75
#define MAVLINK_MSG_ID_COMMAND_LEN 20
#define MAVLINK_MSG_75_LEN 20
#define MAVLINK_MSG_ID_COMMAND_KEY 0x14
#define MAVLINK_MSG_75_KEY 0x14

typedef struct __mavlink_command_t 
{
	float param1;	///< Parameter 1, as defined by MAV_CMD enum.
	float param2;	///< Parameter 2, as defined by MAV_CMD enum.
	float param3;	///< Parameter 3, as defined by MAV_CMD enum.
	float param4;	///< Parameter 4, as defined by MAV_CMD enum.
	uint8_t target_system;	///< System which should execute the command
	uint8_t target_component;	///< Component which should execute the command, 0 for all components
	uint8_t command;	///< Command ID, as defined by MAV_CMD enum.
	uint8_t confirmation;	///< 0: First transmission of this command. 1-255: Confirmation transmissions (e.g. for kill command)

} mavlink_command_t;

/**
 * @brief Pack a command message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system System which should execute the command
 * @param target_component Component which should execute the command, 0 for all components
 * @param command Command ID, as defined by MAV_CMD enum.
 * @param confirmation 0: First transmission of this command. 1-255: Confirmation transmissions (e.g. for kill command)
 * @param param1 Parameter 1, as defined by MAV_CMD enum.
 * @param param2 Parameter 2, as defined by MAV_CMD enum.
 * @param param3 Parameter 3, as defined by MAV_CMD enum.
 * @param param4 Parameter 4, as defined by MAV_CMD enum.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_command_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component, uint8_t command, uint8_t confirmation, float param1, float param2, float param3, float param4)
{
	mavlink_command_t *p = (mavlink_command_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_COMMAND;

	p->target_system = target_system;	// uint8_t:System which should execute the command
	p->target_component = target_component;	// uint8_t:Component which should execute the command, 0 for all components
	p->command = command;	// uint8_t:Command ID, as defined by MAV_CMD enum.
	p->confirmation = confirmation;	// uint8_t:0: First transmission of this command. 1-255: Confirmation transmissions (e.g. for kill command)
	p->param1 = param1;	// float:Parameter 1, as defined by MAV_CMD enum.
	p->param2 = param2;	// float:Parameter 2, as defined by MAV_CMD enum.
	p->param3 = param3;	// float:Parameter 3, as defined by MAV_CMD enum.
	p->param4 = param4;	// float:Parameter 4, as defined by MAV_CMD enum.

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_COMMAND_LEN);
}

/**
 * @brief Pack a command message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system System which should execute the command
 * @param target_component Component which should execute the command, 0 for all components
 * @param command Command ID, as defined by MAV_CMD enum.
 * @param confirmation 0: First transmission of this command. 1-255: Confirmation transmissions (e.g. for kill command)
 * @param param1 Parameter 1, as defined by MAV_CMD enum.
 * @param param2 Parameter 2, as defined by MAV_CMD enum.
 * @param param3 Parameter 3, as defined by MAV_CMD enum.
 * @param param4 Parameter 4, as defined by MAV_CMD enum.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_command_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component, uint8_t command, uint8_t confirmation, float param1, float param2, float param3, float param4)
{
	mavlink_command_t *p = (mavlink_command_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_COMMAND;

	p->target_system = target_system;	// uint8_t:System which should execute the command
	p->target_component = target_component;	// uint8_t:Component which should execute the command, 0 for all components
	p->command = command;	// uint8_t:Command ID, as defined by MAV_CMD enum.
	p->confirmation = confirmation;	// uint8_t:0: First transmission of this command. 1-255: Confirmation transmissions (e.g. for kill command)
	p->param1 = param1;	// float:Parameter 1, as defined by MAV_CMD enum.
	p->param2 = param2;	// float:Parameter 2, as defined by MAV_CMD enum.
	p->param3 = param3;	// float:Parameter 3, as defined by MAV_CMD enum.
	p->param4 = param4;	// float:Parameter 4, as defined by MAV_CMD enum.

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_COMMAND_LEN);
}

/**
 * @brief Encode a command struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param command C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_command_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_command_t* command)
{
	return mavlink_msg_command_pack(system_id, component_id, msg, command->target_system, command->target_component, command->command, command->confirmation, command->param1, command->param2, command->param3, command->param4);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a command message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system System which should execute the command
 * @param target_component Component which should execute the command, 0 for all components
 * @param command Command ID, as defined by MAV_CMD enum.
 * @param confirmation 0: First transmission of this command. 1-255: Confirmation transmissions (e.g. for kill command)
 * @param param1 Parameter 1, as defined by MAV_CMD enum.
 * @param param2 Parameter 2, as defined by MAV_CMD enum.
 * @param param3 Parameter 3, as defined by MAV_CMD enum.
 * @param param4 Parameter 4, as defined by MAV_CMD enum.
 */
static inline void mavlink_msg_command_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, uint8_t command, uint8_t confirmation, float param1, float param2, float param3, float param4)
{
	mavlink_header_t hdr;
	mavlink_command_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_COMMAND_LEN )
	payload.target_system = target_system;	// uint8_t:System which should execute the command
	payload.target_component = target_component;	// uint8_t:Component which should execute the command, 0 for all components
	payload.command = command;	// uint8_t:Command ID, as defined by MAV_CMD enum.
	payload.confirmation = confirmation;	// uint8_t:0: First transmission of this command. 1-255: Confirmation transmissions (e.g. for kill command)
	payload.param1 = param1;	// float:Parameter 1, as defined by MAV_CMD enum.
	payload.param2 = param2;	// float:Parameter 2, as defined by MAV_CMD enum.
	payload.param3 = param3;	// float:Parameter 3, as defined by MAV_CMD enum.
	payload.param4 = param4;	// float:Parameter 4, as defined by MAV_CMD enum.

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_COMMAND_LEN;
	hdr.msgid = MAVLINK_MSG_ID_COMMAND;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0x14, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE COMMAND UNPACKING

/**
 * @brief Get field target_system from command message
 *
 * @return System which should execute the command
 */
static inline uint8_t mavlink_msg_command_get_target_system(const mavlink_message_t* msg)
{
	mavlink_command_t *p = (mavlink_command_t *)&msg->payload[0];
	return (uint8_t)(p->target_system);
}

/**
 * @brief Get field target_component from command message
 *
 * @return Component which should execute the command, 0 for all components
 */
static inline uint8_t mavlink_msg_command_get_target_component(const mavlink_message_t* msg)
{
	mavlink_command_t *p = (mavlink_command_t *)&msg->payload[0];
	return (uint8_t)(p->target_component);
}

/**
 * @brief Get field command from command message
 *
 * @return Command ID, as defined by MAV_CMD enum.
 */
static inline uint8_t mavlink_msg_command_get_command(const mavlink_message_t* msg)
{
	mavlink_command_t *p = (mavlink_command_t *)&msg->payload[0];
	return (uint8_t)(p->command);
}

/**
 * @brief Get field confirmation from command message
 *
 * @return 0: First transmission of this command. 1-255: Confirmation transmissions (e.g. for kill command)
 */
static inline uint8_t mavlink_msg_command_get_confirmation(const mavlink_message_t* msg)
{
	mavlink_command_t *p = (mavlink_command_t *)&msg->payload[0];
	return (uint8_t)(p->confirmation);
}

/**
 * @brief Get field param1 from command message
 *
 * @return Parameter 1, as defined by MAV_CMD enum.
 */
static inline float mavlink_msg_command_get_param1(const mavlink_message_t* msg)
{
	mavlink_command_t *p = (mavlink_command_t *)&msg->payload[0];
	return (float)(p->param1);
}

/**
 * @brief Get field param2 from command message
 *
 * @return Parameter 2, as defined by MAV_CMD enum.
 */
static inline float mavlink_msg_command_get_param2(const mavlink_message_t* msg)
{
	mavlink_command_t *p = (mavlink_command_t *)&msg->payload[0];
	return (float)(p->param2);
}

/**
 * @brief Get field param3 from command message
 *
 * @return Parameter 3, as defined by MAV_CMD enum.
 */
static inline float mavlink_msg_command_get_param3(const mavlink_message_t* msg)
{
	mavlink_command_t *p = (mavlink_command_t *)&msg->payload[0];
	return (float)(p->param3);
}

/**
 * @brief Get field param4 from command message
 *
 * @return Parameter 4, as defined by MAV_CMD enum.
 */
static inline float mavlink_msg_command_get_param4(const mavlink_message_t* msg)
{
	mavlink_command_t *p = (mavlink_command_t *)&msg->payload[0];
	return (float)(p->param4);
}

/**
 * @brief Decode a command message into a struct
 *
 * @param msg The message to decode
 * @param command C-struct to decode the message contents into
 */
static inline void mavlink_msg_command_decode(const mavlink_message_t* msg, mavlink_command_t* command)
{
	memcpy( command, msg->payload, sizeof(mavlink_command_t));
}
