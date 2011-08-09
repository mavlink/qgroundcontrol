// MESSAGE COMMAND_ACK PACKING

#define MAVLINK_MSG_ID_COMMAND_ACK 76
#define MAVLINK_MSG_ID_COMMAND_ACK_LEN 8
#define MAVLINK_MSG_76_LEN 8

typedef struct __mavlink_command_ack_t 
{
	float command; ///< Current airspeed in m/s
	float result; ///< 1: Action ACCEPTED and EXECUTED, 1: Action TEMPORARY REJECTED/DENIED, 2: Action PERMANENTLY DENIED, 3: Action UNKNOWN/UNSUPPORTED, 4: Requesting CONFIRMATION

} mavlink_command_ack_t;

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
static inline uint16_t mavlink_msg_command_ack_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, float command, float result)
{
	mavlink_command_ack_t *p = (mavlink_command_ack_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_COMMAND_ACK;

	p->command = command; // float:Current airspeed in m/s
	p->result = result; // float:1: Action ACCEPTED and EXECUTED, 1: Action TEMPORARY REJECTED/DENIED, 2: Action PERMANENTLY DENIED, 3: Action UNKNOWN/UNSUPPORTED, 4: Requesting CONFIRMATION

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_COMMAND_ACK_LEN);
}

/**
 * @brief Pack a command_ack message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param command Current airspeed in m/s
 * @param result 1: Action ACCEPTED and EXECUTED, 1: Action TEMPORARY REJECTED/DENIED, 2: Action PERMANENTLY DENIED, 3: Action UNKNOWN/UNSUPPORTED, 4: Requesting CONFIRMATION
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_command_ack_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, float command, float result)
{
	mavlink_command_ack_t *p = (mavlink_command_ack_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_COMMAND_ACK;

	p->command = command; // float:Current airspeed in m/s
	p->result = result; // float:1: Action ACCEPTED and EXECUTED, 1: Action TEMPORARY REJECTED/DENIED, 2: Action PERMANENTLY DENIED, 3: Action UNKNOWN/UNSUPPORTED, 4: Requesting CONFIRMATION

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_COMMAND_ACK_LEN);
}

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
	mavlink_header_t hdr;
	mavlink_command_ack_t payload;
	uint16_t checksum;
	mavlink_command_ack_t *p = &payload;

	p->command = command; // float:Current airspeed in m/s
	p->result = result; // float:1: Action ACCEPTED and EXECUTED, 1: Action TEMPORARY REJECTED/DENIED, 2: Action PERMANENTLY DENIED, 3: Action UNKNOWN/UNSUPPORTED, 4: Requesting CONFIRMATION

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_COMMAND_ACK_LEN;
	hdr.msgid = MAVLINK_MSG_ID_COMMAND_ACK;
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
// MESSAGE COMMAND_ACK UNPACKING

/**
 * @brief Get field command from command_ack message
 *
 * @return Current airspeed in m/s
 */
static inline float mavlink_msg_command_ack_get_command(const mavlink_message_t* msg)
{
	mavlink_command_ack_t *p = (mavlink_command_ack_t *)&msg->payload[0];
	return (float)(p->command);
}

/**
 * @brief Get field result from command_ack message
 *
 * @return 1: Action ACCEPTED and EXECUTED, 1: Action TEMPORARY REJECTED/DENIED, 2: Action PERMANENTLY DENIED, 3: Action UNKNOWN/UNSUPPORTED, 4: Requesting CONFIRMATION
 */
static inline float mavlink_msg_command_ack_get_result(const mavlink_message_t* msg)
{
	mavlink_command_ack_t *p = (mavlink_command_ack_t *)&msg->payload[0];
	return (float)(p->result);
}

/**
 * @brief Decode a command_ack message into a struct
 *
 * @param msg The message to decode
 * @param command_ack C-struct to decode the message contents into
 */
static inline void mavlink_msg_command_ack_decode(const mavlink_message_t* msg, mavlink_command_ack_t* command_ack)
{
	memcpy( command_ack, msg->payload, sizeof(mavlink_command_ack_t));
}
