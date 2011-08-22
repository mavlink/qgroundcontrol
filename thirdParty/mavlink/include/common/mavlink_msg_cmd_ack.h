// MESSAGE CMD_ACK PACKING

#define MAVLINK_MSG_ID_CMD_ACK 9
#define MAVLINK_MSG_ID_CMD_ACK_LEN 2
#define MAVLINK_MSG_9_LEN 2
#define MAVLINK_MSG_ID_CMD_ACK_KEY 0x90
#define MAVLINK_MSG_9_KEY 0x90

typedef struct __mavlink_cmd_ack_t 
{
	uint8_t cmd;	///< The MAV_CMD ID
	uint8_t result;	///< 0: Action DENIED, 1: Action executed

} mavlink_cmd_ack_t;

/**
 * @brief Pack a cmd_ack message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param cmd The MAV_CMD ID
 * @param result 0: Action DENIED, 1: Action executed
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_cmd_ack_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t cmd, uint8_t result)
{
	mavlink_cmd_ack_t *p = (mavlink_cmd_ack_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_CMD_ACK;

	p->cmd = cmd;	// uint8_t:The MAV_CMD ID
	p->result = result;	// uint8_t:0: Action DENIED, 1: Action executed

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_CMD_ACK_LEN);
}

/**
 * @brief Pack a cmd_ack message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param cmd The MAV_CMD ID
 * @param result 0: Action DENIED, 1: Action executed
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_cmd_ack_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t cmd, uint8_t result)
{
	mavlink_cmd_ack_t *p = (mavlink_cmd_ack_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_CMD_ACK;

	p->cmd = cmd;	// uint8_t:The MAV_CMD ID
	p->result = result;	// uint8_t:0: Action DENIED, 1: Action executed

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_CMD_ACK_LEN);
}

/**
 * @brief Encode a cmd_ack struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param cmd_ack C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_cmd_ack_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_cmd_ack_t* cmd_ack)
{
	return mavlink_msg_cmd_ack_pack(system_id, component_id, msg, cmd_ack->cmd, cmd_ack->result);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a cmd_ack message
 * @param chan MAVLink channel to send the message
 *
 * @param cmd The MAV_CMD ID
 * @param result 0: Action DENIED, 1: Action executed
 */
static inline void mavlink_msg_cmd_ack_send(mavlink_channel_t chan, uint8_t cmd, uint8_t result)
{
	mavlink_header_t hdr;
	mavlink_cmd_ack_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_CMD_ACK_LEN )
	payload.cmd = cmd;	// uint8_t:The MAV_CMD ID
	payload.result = result;	// uint8_t:0: Action DENIED, 1: Action executed

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_CMD_ACK_LEN;
	hdr.msgid = MAVLINK_MSG_ID_CMD_ACK;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0x90, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE CMD_ACK UNPACKING

/**
 * @brief Get field cmd from cmd_ack message
 *
 * @return The MAV_CMD ID
 */
static inline uint8_t mavlink_msg_cmd_ack_get_cmd(const mavlink_message_t* msg)
{
	mavlink_cmd_ack_t *p = (mavlink_cmd_ack_t *)&msg->payload[0];
	return (uint8_t)(p->cmd);
}

/**
 * @brief Get field result from cmd_ack message
 *
 * @return 0: Action DENIED, 1: Action executed
 */
static inline uint8_t mavlink_msg_cmd_ack_get_result(const mavlink_message_t* msg)
{
	mavlink_cmd_ack_t *p = (mavlink_cmd_ack_t *)&msg->payload[0];
	return (uint8_t)(p->result);
}

/**
 * @brief Decode a cmd_ack message into a struct
 *
 * @param msg The message to decode
 * @param cmd_ack C-struct to decode the message contents into
 */
static inline void mavlink_msg_cmd_ack_decode(const mavlink_message_t* msg, mavlink_cmd_ack_t* cmd_ack)
{
	memcpy( cmd_ack, msg->payload, sizeof(mavlink_cmd_ack_t));
}
