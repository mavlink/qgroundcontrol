// MESSAGE ACTION_ACK PACKING

#define MAVLINK_MSG_ID_ACTION_ACK 9
#define MAVLINK_MSG_ID_ACTION_ACK_LEN 2
#define MAVLINK_MSG_9_LEN 2

typedef struct __mavlink_action_ack_t 
{
	uint8_t action; ///< The action id
	uint8_t result; ///< 0: Action DENIED, 1: Action executed

} mavlink_action_ack_t;

/**
 * @brief Pack a action_ack message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param action The action id
 * @param result 0: Action DENIED, 1: Action executed
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_action_ack_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t action, uint8_t result)
{
	mavlink_action_ack_t *p = (mavlink_action_ack_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_ACTION_ACK;

	p->action = action; // uint8_t:The action id
	p->result = result; // uint8_t:0: Action DENIED, 1: Action executed

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_ACTION_ACK_LEN);
}

/**
 * @brief Pack a action_ack message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param action The action id
 * @param result 0: Action DENIED, 1: Action executed
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_action_ack_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t action, uint8_t result)
{
	mavlink_action_ack_t *p = (mavlink_action_ack_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_ACTION_ACK;

	p->action = action; // uint8_t:The action id
	p->result = result; // uint8_t:0: Action DENIED, 1: Action executed

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_ACTION_ACK_LEN);
}

/**
 * @brief Encode a action_ack struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param action_ack C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_action_ack_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_action_ack_t* action_ack)
{
	return mavlink_msg_action_ack_pack(system_id, component_id, msg, action_ack->action, action_ack->result);
}

/**
 * @brief Send a action_ack message
 * @param chan MAVLink channel to send the message
 *
 * @param action The action id
 * @param result 0: Action DENIED, 1: Action executed
 */


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_action_ack_send(mavlink_channel_t chan, uint8_t action, uint8_t result)
{
	mavlink_header_t hdr;
	mavlink_action_ack_t payload;
	uint16_t checksum;
	mavlink_action_ack_t *p = &payload;

	p->action = action; // uint8_t:The action id
	p->result = result; // uint8_t:0: Action DENIED, 1: Action executed

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_ACTION_ACK_LEN;
	hdr.msgid = MAVLINK_MSG_ID_ACTION_ACK;
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
// MESSAGE ACTION_ACK UNPACKING

/**
 * @brief Get field action from action_ack message
 *
 * @return The action id
 */
static inline uint8_t mavlink_msg_action_ack_get_action(const mavlink_message_t* msg)
{
	mavlink_action_ack_t *p = (mavlink_action_ack_t *)&msg->payload[0];
	return (uint8_t)(p->action);
}

/**
 * @brief Get field result from action_ack message
 *
 * @return 0: Action DENIED, 1: Action executed
 */
static inline uint8_t mavlink_msg_action_ack_get_result(const mavlink_message_t* msg)
{
	mavlink_action_ack_t *p = (mavlink_action_ack_t *)&msg->payload[0];
	return (uint8_t)(p->result);
}

/**
 * @brief Decode a action_ack message into a struct
 *
 * @param msg The message to decode
 * @param action_ack C-struct to decode the message contents into
 */
static inline void mavlink_msg_action_ack_decode(const mavlink_message_t* msg, mavlink_action_ack_t* action_ack)
{
	memcpy( action_ack, msg->payload, sizeof(mavlink_action_ack_t));
}
