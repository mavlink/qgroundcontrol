// MESSAGE SLUGS_ACTION PACKING

#define MAVLINK_MSG_ID_SLUGS_ACTION 183
#define MAVLINK_MSG_ID_SLUGS_ACTION_LEN 4
#define MAVLINK_MSG_183_LEN 4
#define MAVLINK_MSG_ID_SLUGS_ACTION_KEY 0xD4
#define MAVLINK_MSG_183_KEY 0xD4

typedef struct __mavlink_slugs_action_t 
{
	uint16_t actionVal;	///< Value associated with the action
	uint8_t target;	///< The system reporting the action
	uint8_t actionId;	///< Action ID. See apDefinitions.h in the SLUGS /clib directory for the ID names

} mavlink_slugs_action_t;

/**
 * @brief Pack a slugs_action message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target The system reporting the action
 * @param actionId Action ID. See apDefinitions.h in the SLUGS /clib directory for the ID names
 * @param actionVal Value associated with the action
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_slugs_action_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t target, uint8_t actionId, uint16_t actionVal)
{
	mavlink_slugs_action_t *p = (mavlink_slugs_action_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SLUGS_ACTION;

	p->target = target;	// uint8_t:The system reporting the action
	p->actionId = actionId;	// uint8_t:Action ID. See apDefinitions.h in the SLUGS /clib directory for the ID names
	p->actionVal = actionVal;	// uint16_t:Value associated with the action

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_SLUGS_ACTION_LEN);
}

/**
 * @brief Pack a slugs_action message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target The system reporting the action
 * @param actionId Action ID. See apDefinitions.h in the SLUGS /clib directory for the ID names
 * @param actionVal Value associated with the action
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_slugs_action_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t target, uint8_t actionId, uint16_t actionVal)
{
	mavlink_slugs_action_t *p = (mavlink_slugs_action_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SLUGS_ACTION;

	p->target = target;	// uint8_t:The system reporting the action
	p->actionId = actionId;	// uint8_t:Action ID. See apDefinitions.h in the SLUGS /clib directory for the ID names
	p->actionVal = actionVal;	// uint16_t:Value associated with the action

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_SLUGS_ACTION_LEN);
}

/**
 * @brief Encode a slugs_action struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param slugs_action C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_slugs_action_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_slugs_action_t* slugs_action)
{
	return mavlink_msg_slugs_action_pack(system_id, component_id, msg, slugs_action->target, slugs_action->actionId, slugs_action->actionVal);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a slugs_action message
 * @param chan MAVLink channel to send the message
 *
 * @param target The system reporting the action
 * @param actionId Action ID. See apDefinitions.h in the SLUGS /clib directory for the ID names
 * @param actionVal Value associated with the action
 */
static inline void mavlink_msg_slugs_action_send(mavlink_channel_t chan, uint8_t target, uint8_t actionId, uint16_t actionVal)
{
	mavlink_header_t hdr;
	mavlink_slugs_action_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_SLUGS_ACTION_LEN )
	payload.target = target;	// uint8_t:The system reporting the action
	payload.actionId = actionId;	// uint8_t:Action ID. See apDefinitions.h in the SLUGS /clib directory for the ID names
	payload.actionVal = actionVal;	// uint16_t:Value associated with the action

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_SLUGS_ACTION_LEN;
	hdr.msgid = MAVLINK_MSG_ID_SLUGS_ACTION;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0xD4, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE SLUGS_ACTION UNPACKING

/**
 * @brief Get field target from slugs_action message
 *
 * @return The system reporting the action
 */
static inline uint8_t mavlink_msg_slugs_action_get_target(const mavlink_message_t* msg)
{
	mavlink_slugs_action_t *p = (mavlink_slugs_action_t *)&msg->payload[0];
	return (uint8_t)(p->target);
}

/**
 * @brief Get field actionId from slugs_action message
 *
 * @return Action ID. See apDefinitions.h in the SLUGS /clib directory for the ID names
 */
static inline uint8_t mavlink_msg_slugs_action_get_actionId(const mavlink_message_t* msg)
{
	mavlink_slugs_action_t *p = (mavlink_slugs_action_t *)&msg->payload[0];
	return (uint8_t)(p->actionId);
}

/**
 * @brief Get field actionVal from slugs_action message
 *
 * @return Value associated with the action
 */
static inline uint16_t mavlink_msg_slugs_action_get_actionVal(const mavlink_message_t* msg)
{
	mavlink_slugs_action_t *p = (mavlink_slugs_action_t *)&msg->payload[0];
	return (uint16_t)(p->actionVal);
}

/**
 * @brief Decode a slugs_action message into a struct
 *
 * @param msg The message to decode
 * @param slugs_action C-struct to decode the message contents into
 */
static inline void mavlink_msg_slugs_action_decode(const mavlink_message_t* msg, mavlink_slugs_action_t* slugs_action)
{
	memcpy( slugs_action, msg->payload, sizeof(mavlink_slugs_action_t));
}
