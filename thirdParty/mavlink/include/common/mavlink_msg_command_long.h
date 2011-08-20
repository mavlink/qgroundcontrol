// MESSAGE COMMAND_LONG PACKING

#define MAVLINK_MSG_ID_COMMAND_LONG 76
#define MAVLINK_MSG_ID_COMMAND_LONG_LEN 32
#define MAVLINK_MSG_76_LEN 32
#define MAVLINK_MSG_ID_COMMAND_LONG_KEY 0x3F
#define MAVLINK_MSG_76_KEY 0x3F

typedef struct __mavlink_command_long_t 
{
	float param1;	///< Parameter 1, as defined by MAV_CMD enum.
	float param2;	///< Parameter 2, as defined by MAV_CMD enum.
	float param3;	///< Parameter 3, as defined by MAV_CMD enum.
	float param4;	///< Parameter 4, as defined by MAV_CMD enum.
	float param5;	///< Parameter 5, as defined by MAV_CMD enum.
	float param6;	///< Parameter 6, as defined by MAV_CMD enum.
	float param7;	///< Parameter 7, as defined by MAV_CMD enum.
	uint8_t target_system;	///< System which should execute the command
	uint8_t target_component;	///< Component which should execute the command, 0 for all components
	uint8_t command;	///< Command ID, as defined by MAV_CMD enum.
	uint8_t confirmation;	///< 0: First transmission of this command. 1-255: Confirmation transmissions (e.g. for kill command)

} mavlink_command_long_t;

/**
 * @brief Pack a command_long message
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
 * @param param5 Parameter 5, as defined by MAV_CMD enum.
 * @param param6 Parameter 6, as defined by MAV_CMD enum.
 * @param param7 Parameter 7, as defined by MAV_CMD enum.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_command_long_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component, uint8_t command, uint8_t confirmation, float param1, float param2, float param3, float param4, float param5, float param6, float param7)
{
	mavlink_command_long_t *p = (mavlink_command_long_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_COMMAND_LONG;

	p->target_system = target_system;	// uint8_t:System which should execute the command
	p->target_component = target_component;	// uint8_t:Component which should execute the command, 0 for all components
	p->command = command;	// uint8_t:Command ID, as defined by MAV_CMD enum.
	p->confirmation = confirmation;	// uint8_t:0: First transmission of this command. 1-255: Confirmation transmissions (e.g. for kill command)
	p->param1 = param1;	// float:Parameter 1, as defined by MAV_CMD enum.
	p->param2 = param2;	// float:Parameter 2, as defined by MAV_CMD enum.
	p->param3 = param3;	// float:Parameter 3, as defined by MAV_CMD enum.
	p->param4 = param4;	// float:Parameter 4, as defined by MAV_CMD enum.
	p->param5 = param5;	// float:Parameter 5, as defined by MAV_CMD enum.
	p->param6 = param6;	// float:Parameter 6, as defined by MAV_CMD enum.
	p->param7 = param7;	// float:Parameter 7, as defined by MAV_CMD enum.

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_COMMAND_LONG_LEN);
}

/**
 * @brief Pack a command_long message
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
 * @param param5 Parameter 5, as defined by MAV_CMD enum.
 * @param param6 Parameter 6, as defined by MAV_CMD enum.
 * @param param7 Parameter 7, as defined by MAV_CMD enum.
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_command_long_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component, uint8_t command, uint8_t confirmation, float param1, float param2, float param3, float param4, float param5, float param6, float param7)
{
	mavlink_command_long_t *p = (mavlink_command_long_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_COMMAND_LONG;

	p->target_system = target_system;	// uint8_t:System which should execute the command
	p->target_component = target_component;	// uint8_t:Component which should execute the command, 0 for all components
	p->command = command;	// uint8_t:Command ID, as defined by MAV_CMD enum.
	p->confirmation = confirmation;	// uint8_t:0: First transmission of this command. 1-255: Confirmation transmissions (e.g. for kill command)
	p->param1 = param1;	// float:Parameter 1, as defined by MAV_CMD enum.
	p->param2 = param2;	// float:Parameter 2, as defined by MAV_CMD enum.
	p->param3 = param3;	// float:Parameter 3, as defined by MAV_CMD enum.
	p->param4 = param4;	// float:Parameter 4, as defined by MAV_CMD enum.
	p->param5 = param5;	// float:Parameter 5, as defined by MAV_CMD enum.
	p->param6 = param6;	// float:Parameter 6, as defined by MAV_CMD enum.
	p->param7 = param7;	// float:Parameter 7, as defined by MAV_CMD enum.

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_COMMAND_LONG_LEN);
}

/**
 * @brief Encode a command_long struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param command_long C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_command_long_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_command_long_t* command_long)
{
	return mavlink_msg_command_long_pack(system_id, component_id, msg, command_long->target_system, command_long->target_component, command_long->command, command_long->confirmation, command_long->param1, command_long->param2, command_long->param3, command_long->param4, command_long->param5, command_long->param6, command_long->param7);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a command_long message
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
 * @param param5 Parameter 5, as defined by MAV_CMD enum.
 * @param param6 Parameter 6, as defined by MAV_CMD enum.
 * @param param7 Parameter 7, as defined by MAV_CMD enum.
 */
static inline void mavlink_msg_command_long_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, uint8_t command, uint8_t confirmation, float param1, float param2, float param3, float param4, float param5, float param6, float param7)
{
	mavlink_header_t hdr;
	mavlink_command_long_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_COMMAND_LONG_LEN )
	payload.target_system = target_system;	// uint8_t:System which should execute the command
	payload.target_component = target_component;	// uint8_t:Component which should execute the command, 0 for all components
	payload.command = command;	// uint8_t:Command ID, as defined by MAV_CMD enum.
	payload.confirmation = confirmation;	// uint8_t:0: First transmission of this command. 1-255: Confirmation transmissions (e.g. for kill command)
	payload.param1 = param1;	// float:Parameter 1, as defined by MAV_CMD enum.
	payload.param2 = param2;	// float:Parameter 2, as defined by MAV_CMD enum.
	payload.param3 = param3;	// float:Parameter 3, as defined by MAV_CMD enum.
	payload.param4 = param4;	// float:Parameter 4, as defined by MAV_CMD enum.
	payload.param5 = param5;	// float:Parameter 5, as defined by MAV_CMD enum.
	payload.param6 = param6;	// float:Parameter 6, as defined by MAV_CMD enum.
	payload.param7 = param7;	// float:Parameter 7, as defined by MAV_CMD enum.

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_COMMAND_LONG_LEN;
	hdr.msgid = MAVLINK_MSG_ID_COMMAND_LONG;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0x3F, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE COMMAND_LONG UNPACKING

/**
 * @brief Get field target_system from command_long message
 *
 * @return System which should execute the command
 */
static inline uint8_t mavlink_msg_command_long_get_target_system(const mavlink_message_t* msg)
{
	mavlink_command_long_t *p = (mavlink_command_long_t *)&msg->payload[0];
	return (uint8_t)(p->target_system);
}

/**
 * @brief Get field target_component from command_long message
 *
 * @return Component which should execute the command, 0 for all components
 */
static inline uint8_t mavlink_msg_command_long_get_target_component(const mavlink_message_t* msg)
{
	mavlink_command_long_t *p = (mavlink_command_long_t *)&msg->payload[0];
	return (uint8_t)(p->target_component);
}

/**
 * @brief Get field command from command_long message
 *
 * @return Command ID, as defined by MAV_CMD enum.
 */
static inline uint8_t mavlink_msg_command_long_get_command(const mavlink_message_t* msg)
{
	mavlink_command_long_t *p = (mavlink_command_long_t *)&msg->payload[0];
	return (uint8_t)(p->command);
}

/**
 * @brief Get field confirmation from command_long message
 *
 * @return 0: First transmission of this command. 1-255: Confirmation transmissions (e.g. for kill command)
 */
static inline uint8_t mavlink_msg_command_long_get_confirmation(const mavlink_message_t* msg)
{
	mavlink_command_long_t *p = (mavlink_command_long_t *)&msg->payload[0];
	return (uint8_t)(p->confirmation);
}

/**
 * @brief Get field param1 from command_long message
 *
 * @return Parameter 1, as defined by MAV_CMD enum.
 */
static inline float mavlink_msg_command_long_get_param1(const mavlink_message_t* msg)
{
	mavlink_command_long_t *p = (mavlink_command_long_t *)&msg->payload[0];
	return (float)(p->param1);
}

/**
 * @brief Get field param2 from command_long message
 *
 * @return Parameter 2, as defined by MAV_CMD enum.
 */
static inline float mavlink_msg_command_long_get_param2(const mavlink_message_t* msg)
{
	mavlink_command_long_t *p = (mavlink_command_long_t *)&msg->payload[0];
	return (float)(p->param2);
}

/**
 * @brief Get field param3 from command_long message
 *
 * @return Parameter 3, as defined by MAV_CMD enum.
 */
static inline float mavlink_msg_command_long_get_param3(const mavlink_message_t* msg)
{
	mavlink_command_long_t *p = (mavlink_command_long_t *)&msg->payload[0];
	return (float)(p->param3);
}

/**
 * @brief Get field param4 from command_long message
 *
 * @return Parameter 4, as defined by MAV_CMD enum.
 */
static inline float mavlink_msg_command_long_get_param4(const mavlink_message_t* msg)
{
	mavlink_command_long_t *p = (mavlink_command_long_t *)&msg->payload[0];
	return (float)(p->param4);
}

/**
 * @brief Get field param5 from command_long message
 *
 * @return Parameter 5, as defined by MAV_CMD enum.
 */
static inline float mavlink_msg_command_long_get_param5(const mavlink_message_t* msg)
{
	mavlink_command_long_t *p = (mavlink_command_long_t *)&msg->payload[0];
	return (float)(p->param5);
}

/**
 * @brief Get field param6 from command_long message
 *
 * @return Parameter 6, as defined by MAV_CMD enum.
 */
static inline float mavlink_msg_command_long_get_param6(const mavlink_message_t* msg)
{
	mavlink_command_long_t *p = (mavlink_command_long_t *)&msg->payload[0];
	return (float)(p->param6);
}

/**
 * @brief Get field param7 from command_long message
 *
 * @return Parameter 7, as defined by MAV_CMD enum.
 */
static inline float mavlink_msg_command_long_get_param7(const mavlink_message_t* msg)
{
	mavlink_command_long_t *p = (mavlink_command_long_t *)&msg->payload[0];
	return (float)(p->param7);
}

/**
 * @brief Decode a command_long message into a struct
 *
 * @param msg The message to decode
 * @param command_long C-struct to decode the message contents into
 */
static inline void mavlink_msg_command_long_decode(const mavlink_message_t* msg, mavlink_command_long_t* command_long)
{
	memcpy( command_long, msg->payload, sizeof(mavlink_command_long_t));
}
