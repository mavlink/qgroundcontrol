// MESSAGE CHANGE_OPERATOR_CONTROL PACKING

#define MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL 5
#define MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL_LEN 28
#define MAVLINK_MSG_5_LEN 28
#define MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL_KEY 0x7E
#define MAVLINK_MSG_5_KEY 0x7E

typedef struct __mavlink_change_operator_control_t 
{
	uint8_t target_system;	///< System the GCS requests control for
	uint8_t control_request;	///< 0: request control of this MAV, 1: Release control of this MAV
	uint8_t version;	///< 0: key as plaintext, 1-255: future, different hashing/encryption variants. The GCS should in general use the safest mode possible initially and then gradually move down the encryption level if it gets a NACK message indicating an encryption mismatch.
	char passkey[25];	///< Password / Key, depending on version plaintext or encrypted. 25 or less characters, NULL terminated. The characters may involve A-Z, a-z, 0-9, and "!?,.-"

} mavlink_change_operator_control_t;
#define MAVLINK_MSG_CHANGE_OPERATOR_CONTROL_FIELD_PASSKEY_LEN 25

/**
 * @brief Pack a change_operator_control message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system System the GCS requests control for
 * @param control_request 0: request control of this MAV, 1: Release control of this MAV
 * @param version 0: key as plaintext, 1-255: future, different hashing/encryption variants. The GCS should in general use the safest mode possible initially and then gradually move down the encryption level if it gets a NACK message indicating an encryption mismatch.
 * @param passkey Password / Key, depending on version plaintext or encrypted. 25 or less characters, NULL terminated. The characters may involve A-Z, a-z, 0-9, and "!?,.-"
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_change_operator_control_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t target_system, uint8_t control_request, uint8_t version, const char* passkey)
{
	mavlink_change_operator_control_t *p = (mavlink_change_operator_control_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL;

	p->target_system = target_system;	// uint8_t:System the GCS requests control for
	p->control_request = control_request;	// uint8_t:0: request control of this MAV, 1: Release control of this MAV
	p->version = version;	// uint8_t:0: key as plaintext, 1-255: future, different hashing/encryption variants. The GCS should in general use the safest mode possible initially and then gradually move down the encryption level if it gets a NACK message indicating an encryption mismatch.
	memcpy(p->passkey, passkey, sizeof(p->passkey));	// char[25]:Password / Key, depending on version plaintext or encrypted. 25 or less characters, NULL terminated. The characters may involve A-Z, a-z, 0-9, and "!?,.-"

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL_LEN);
}

/**
 * @brief Pack a change_operator_control message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system System the GCS requests control for
 * @param control_request 0: request control of this MAV, 1: Release control of this MAV
 * @param version 0: key as plaintext, 1-255: future, different hashing/encryption variants. The GCS should in general use the safest mode possible initially and then gradually move down the encryption level if it gets a NACK message indicating an encryption mismatch.
 * @param passkey Password / Key, depending on version plaintext or encrypted. 25 or less characters, NULL terminated. The characters may involve A-Z, a-z, 0-9, and "!?,.-"
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_change_operator_control_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t target_system, uint8_t control_request, uint8_t version, const char* passkey)
{
	mavlink_change_operator_control_t *p = (mavlink_change_operator_control_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL;

	p->target_system = target_system;	// uint8_t:System the GCS requests control for
	p->control_request = control_request;	// uint8_t:0: request control of this MAV, 1: Release control of this MAV
	p->version = version;	// uint8_t:0: key as plaintext, 1-255: future, different hashing/encryption variants. The GCS should in general use the safest mode possible initially and then gradually move down the encryption level if it gets a NACK message indicating an encryption mismatch.
	memcpy(p->passkey, passkey, sizeof(p->passkey));	// char[25]:Password / Key, depending on version plaintext or encrypted. 25 or less characters, NULL terminated. The characters may involve A-Z, a-z, 0-9, and "!?,.-"

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL_LEN);
}

/**
 * @brief Encode a change_operator_control struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param change_operator_control C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_change_operator_control_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_change_operator_control_t* change_operator_control)
{
	return mavlink_msg_change_operator_control_pack(system_id, component_id, msg, change_operator_control->target_system, change_operator_control->control_request, change_operator_control->version, change_operator_control->passkey);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a change_operator_control message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system System the GCS requests control for
 * @param control_request 0: request control of this MAV, 1: Release control of this MAV
 * @param version 0: key as plaintext, 1-255: future, different hashing/encryption variants. The GCS should in general use the safest mode possible initially and then gradually move down the encryption level if it gets a NACK message indicating an encryption mismatch.
 * @param passkey Password / Key, depending on version plaintext or encrypted. 25 or less characters, NULL terminated. The characters may involve A-Z, a-z, 0-9, and "!?,.-"
 */
static inline void mavlink_msg_change_operator_control_send(mavlink_channel_t chan, uint8_t target_system, uint8_t control_request, uint8_t version, const char* passkey)
{
	mavlink_header_t hdr;
	mavlink_change_operator_control_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL_LEN )
	payload.target_system = target_system;	// uint8_t:System the GCS requests control for
	payload.control_request = control_request;	// uint8_t:0: request control of this MAV, 1: Release control of this MAV
	payload.version = version;	// uint8_t:0: key as plaintext, 1-255: future, different hashing/encryption variants. The GCS should in general use the safest mode possible initially and then gradually move down the encryption level if it gets a NACK message indicating an encryption mismatch.
	memcpy(payload.passkey, passkey, sizeof(payload.passkey));	// char[25]:Password / Key, depending on version plaintext or encrypted. 25 or less characters, NULL terminated. The characters may involve A-Z, a-z, 0-9, and "!?,.-"

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL_LEN;
	hdr.msgid = MAVLINK_MSG_ID_CHANGE_OPERATOR_CONTROL;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0x7E, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE CHANGE_OPERATOR_CONTROL UNPACKING

/**
 * @brief Get field target_system from change_operator_control message
 *
 * @return System the GCS requests control for
 */
static inline uint8_t mavlink_msg_change_operator_control_get_target_system(const mavlink_message_t* msg)
{
	mavlink_change_operator_control_t *p = (mavlink_change_operator_control_t *)&msg->payload[0];
	return (uint8_t)(p->target_system);
}

/**
 * @brief Get field control_request from change_operator_control message
 *
 * @return 0: request control of this MAV, 1: Release control of this MAV
 */
static inline uint8_t mavlink_msg_change_operator_control_get_control_request(const mavlink_message_t* msg)
{
	mavlink_change_operator_control_t *p = (mavlink_change_operator_control_t *)&msg->payload[0];
	return (uint8_t)(p->control_request);
}

/**
 * @brief Get field version from change_operator_control message
 *
 * @return 0: key as plaintext, 1-255: future, different hashing/encryption variants. The GCS should in general use the safest mode possible initially and then gradually move down the encryption level if it gets a NACK message indicating an encryption mismatch.
 */
static inline uint8_t mavlink_msg_change_operator_control_get_version(const mavlink_message_t* msg)
{
	mavlink_change_operator_control_t *p = (mavlink_change_operator_control_t *)&msg->payload[0];
	return (uint8_t)(p->version);
}

/**
 * @brief Get field passkey from change_operator_control message
 *
 * @return Password / Key, depending on version plaintext or encrypted. 25 or less characters, NULL terminated. The characters may involve A-Z, a-z, 0-9, and "!?,.-"
 */
static inline uint16_t mavlink_msg_change_operator_control_get_passkey(const mavlink_message_t* msg, char* passkey)
{
	mavlink_change_operator_control_t *p = (mavlink_change_operator_control_t *)&msg->payload[0];

	memcpy(passkey, p->passkey, sizeof(p->passkey));
	return sizeof(p->passkey);
}

/**
 * @brief Decode a change_operator_control message into a struct
 *
 * @param msg The message to decode
 * @param change_operator_control C-struct to decode the message contents into
 */
static inline void mavlink_msg_change_operator_control_decode(const mavlink_message_t* msg, mavlink_change_operator_control_t* change_operator_control)
{
	memcpy( change_operator_control, msg->payload, sizeof(mavlink_change_operator_control_t));
}
