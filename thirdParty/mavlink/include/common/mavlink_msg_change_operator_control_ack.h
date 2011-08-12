// MESSAGE CHANGE_OPERATOR_CONTROL_ACK PACKING

#define MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL_ACK 6
#define MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL_ACK_LEN 3
#define MAVLINK_MSG_6_LEN 3
#define MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL_ACK_KEY 0x75
#define MAVLINK_MSG_6_KEY 0x75

typedef struct __mavlink_change_operator_control_ack_t 
{
	uint8_t gcs_system_id;	///< ID of the GCS this message 
	uint8_t control_request;	///< 0: request control of this MAV, 1: Release control of this MAV
	uint8_t ack;	///< 0: ACK, 1: NACK: Wrong passkey, 2: NACK: Unsupported passkey encryption method, 3: NACK: Already under control

} mavlink_change_operator_control_ack_t;

/**
 * @brief Pack a change_operator_control_ack message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param gcs_system_id ID of the GCS this message 
 * @param control_request 0: request control of this MAV, 1: Release control of this MAV
 * @param ack 0: ACK, 1: NACK: Wrong passkey, 2: NACK: Unsupported passkey encryption method, 3: NACK: Already under control
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_change_operator_control_ack_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t gcs_system_id, uint8_t control_request, uint8_t ack)
{
	mavlink_change_operator_control_ack_t *p = (mavlink_change_operator_control_ack_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL_ACK;

	p->gcs_system_id = gcs_system_id;	// uint8_t:ID of the GCS this message 
	p->control_request = control_request;	// uint8_t:0: request control of this MAV, 1: Release control of this MAV
	p->ack = ack;	// uint8_t:0: ACK, 1: NACK: Wrong passkey, 2: NACK: Unsupported passkey encryption method, 3: NACK: Already under control

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL_ACK_LEN);
}

/**
 * @brief Pack a change_operator_control_ack message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param gcs_system_id ID of the GCS this message 
 * @param control_request 0: request control of this MAV, 1: Release control of this MAV
 * @param ack 0: ACK, 1: NACK: Wrong passkey, 2: NACK: Unsupported passkey encryption method, 3: NACK: Already under control
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_change_operator_control_ack_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t gcs_system_id, uint8_t control_request, uint8_t ack)
{
	mavlink_change_operator_control_ack_t *p = (mavlink_change_operator_control_ack_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL_ACK;

	p->gcs_system_id = gcs_system_id;	// uint8_t:ID of the GCS this message 
	p->control_request = control_request;	// uint8_t:0: request control of this MAV, 1: Release control of this MAV
	p->ack = ack;	// uint8_t:0: ACK, 1: NACK: Wrong passkey, 2: NACK: Unsupported passkey encryption method, 3: NACK: Already under control

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL_ACK_LEN);
}

/**
 * @brief Encode a change_operator_control_ack struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param change_operator_control_ack C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_change_operator_control_ack_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_change_operator_control_ack_t* change_operator_control_ack)
{
	return mavlink_msg_change_operator_control_ack_pack(system_id, component_id, msg, change_operator_control_ack->gcs_system_id, change_operator_control_ack->control_request, change_operator_control_ack->ack);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a change_operator_control_ack message
 * @param chan MAVLink channel to send the message
 *
 * @param gcs_system_id ID of the GCS this message 
 * @param control_request 0: request control of this MAV, 1: Release control of this MAV
 * @param ack 0: ACK, 1: NACK: Wrong passkey, 2: NACK: Unsupported passkey encryption method, 3: NACK: Already under control
 */
static inline void mavlink_msg_change_operator_control_ack_send(mavlink_channel_t chan, uint8_t gcs_system_id, uint8_t control_request, uint8_t ack)
{
	mavlink_header_t hdr;
	mavlink_change_operator_control_ack_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL_ACK_LEN )
	payload.gcs_system_id = gcs_system_id;	// uint8_t:ID of the GCS this message 
	payload.control_request = control_request;	// uint8_t:0: request control of this MAV, 1: Release control of this MAV
	payload.ack = ack;	// uint8_t:0: ACK, 1: NACK: Wrong passkey, 2: NACK: Unsupported passkey encryption method, 3: NACK: Already under control

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL_ACK_LEN;
	hdr.msgid = MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL_ACK;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0x75, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE CHANGE_OPERATOR_CONTROL_ACK UNPACKING

/**
 * @brief Get field gcs_system_id from change_operator_control_ack message
 *
 * @return ID of the GCS this message 
 */
static inline uint8_t mavlink_msg_change_operator_control_ack_get_gcs_system_id(const mavlink_message_t* msg)
{
	mavlink_change_operator_control_ack_t *p = (mavlink_change_operator_control_ack_t *)&msg->payload[0];
	return (uint8_t)(p->gcs_system_id);
}

/**
 * @brief Get field control_request from change_operator_control_ack message
 *
 * @return 0: request control of this MAV, 1: Release control of this MAV
 */
static inline uint8_t mavlink_msg_change_operator_control_ack_get_control_request(const mavlink_message_t* msg)
{
	mavlink_change_operator_control_ack_t *p = (mavlink_change_operator_control_ack_t *)&msg->payload[0];
	return (uint8_t)(p->control_request);
}

/**
 * @brief Get field ack from change_operator_control_ack message
 *
 * @return 0: ACK, 1: NACK: Wrong passkey, 2: NACK: Unsupported passkey encryption method, 3: NACK: Already under control
 */
static inline uint8_t mavlink_msg_change_operator_control_ack_get_ack(const mavlink_message_t* msg)
{
	mavlink_change_operator_control_ack_t *p = (mavlink_change_operator_control_ack_t *)&msg->payload[0];
	return (uint8_t)(p->ack);
}

/**
 * @brief Decode a change_operator_control_ack message into a struct
 *
 * @param msg The message to decode
 * @param change_operator_control_ack C-struct to decode the message contents into
 */
static inline void mavlink_msg_change_operator_control_ack_decode(const mavlink_message_t* msg, mavlink_change_operator_control_ack_t* change_operator_control_ack)
{
	memcpy( change_operator_control_ack, msg->payload, sizeof(mavlink_change_operator_control_ack_t));
}
