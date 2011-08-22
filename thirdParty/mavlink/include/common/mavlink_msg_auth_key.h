// MESSAGE AUTH_KEY PACKING

#define MAVLINK_MSG_ID_AUTH_KEY 7
#define MAVLINK_MSG_ID_AUTH_KEY_LEN 32
#define MAVLINK_MSG_7_LEN 32
#define MAVLINK_MSG_ID_AUTH_KEY_KEY 0xBA
#define MAVLINK_MSG_7_KEY 0xBA

typedef struct __mavlink_auth_key_t 
{
	char key[32];	///< key

} mavlink_auth_key_t;
#define MAVLINK_MSG_AUTH_KEY_FIELD_KEY_LEN 32

/**
 * @brief Pack a auth_key message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param key key
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_auth_key_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const char* key)
{
	mavlink_auth_key_t *p = (mavlink_auth_key_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_AUTH_KEY;

	memcpy(p->key, key, sizeof(p->key));	// char[32]:key

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_AUTH_KEY_LEN);
}

/**
 * @brief Pack a auth_key message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param key key
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_auth_key_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const char* key)
{
	mavlink_auth_key_t *p = (mavlink_auth_key_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_AUTH_KEY;

	memcpy(p->key, key, sizeof(p->key));	// char[32]:key

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_AUTH_KEY_LEN);
}

/**
 * @brief Encode a auth_key struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param auth_key C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_auth_key_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_auth_key_t* auth_key)
{
	return mavlink_msg_auth_key_pack(system_id, component_id, msg, auth_key->key);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a auth_key message
 * @param chan MAVLink channel to send the message
 *
 * @param key key
 */
static inline void mavlink_msg_auth_key_send(mavlink_channel_t chan, const char* key)
{
	mavlink_header_t hdr;
	mavlink_auth_key_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_AUTH_KEY_LEN )
	memcpy(payload.key, key, sizeof(payload.key));	// char[32]:key

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_AUTH_KEY_LEN;
	hdr.msgid = MAVLINK_MSG_ID_AUTH_KEY;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0xBA, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE AUTH_KEY UNPACKING

/**
 * @brief Get field key from auth_key message
 *
 * @return key
 */
static inline uint16_t mavlink_msg_auth_key_get_key(const mavlink_message_t* msg, char* key)
{
	mavlink_auth_key_t *p = (mavlink_auth_key_t *)&msg->payload[0];

	memcpy(key, p->key, sizeof(p->key));
	return sizeof(p->key);
}

/**
 * @brief Decode a auth_key message into a struct
 *
 * @param msg The message to decode
 * @param auth_key C-struct to decode the message contents into
 */
static inline void mavlink_msg_auth_key_decode(const mavlink_message_t* msg, mavlink_auth_key_t* auth_key)
{
	memcpy( auth_key, msg->payload, sizeof(mavlink_auth_key_t));
}
