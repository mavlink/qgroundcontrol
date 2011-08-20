// MESSAGE DEBUG PACKING

#define MAVLINK_MSG_ID_DEBUG 255
#define MAVLINK_MSG_ID_DEBUG_LEN 5
#define MAVLINK_MSG_255_LEN 5
#define MAVLINK_MSG_ID_DEBUG_KEY 0x54
#define MAVLINK_MSG_255_KEY 0x54

typedef struct __mavlink_debug_t 
{
	float value;	///< DEBUG value
	uint8_t ind;	///< index of debug variable

} mavlink_debug_t;

/**
 * @brief Pack a debug message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param ind index of debug variable
 * @param value DEBUG value
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_debug_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t ind, float value)
{
	mavlink_debug_t *p = (mavlink_debug_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_DEBUG;

	p->ind = ind;	// uint8_t:index of debug variable
	p->value = value;	// float:DEBUG value

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_DEBUG_LEN);
}

/**
 * @brief Pack a debug message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param ind index of debug variable
 * @param value DEBUG value
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_debug_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t ind, float value)
{
	mavlink_debug_t *p = (mavlink_debug_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_DEBUG;

	p->ind = ind;	// uint8_t:index of debug variable
	p->value = value;	// float:DEBUG value

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_DEBUG_LEN);
}

/**
 * @brief Encode a debug struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param debug C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_debug_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_debug_t* debug)
{
	return mavlink_msg_debug_pack(system_id, component_id, msg, debug->ind, debug->value);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a debug message
 * @param chan MAVLink channel to send the message
 *
 * @param ind index of debug variable
 * @param value DEBUG value
 */
static inline void mavlink_msg_debug_send(mavlink_channel_t chan, uint8_t ind, float value)
{
	mavlink_header_t hdr;
	mavlink_debug_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_DEBUG_LEN )
	payload.ind = ind;	// uint8_t:index of debug variable
	payload.value = value;	// float:DEBUG value

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_DEBUG_LEN;
	hdr.msgid = MAVLINK_MSG_ID_DEBUG;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0x54, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE DEBUG UNPACKING

/**
 * @brief Get field ind from debug message
 *
 * @return index of debug variable
 */
static inline uint8_t mavlink_msg_debug_get_ind(const mavlink_message_t* msg)
{
	mavlink_debug_t *p = (mavlink_debug_t *)&msg->payload[0];
	return (uint8_t)(p->ind);
}

/**
 * @brief Get field value from debug message
 *
 * @return DEBUG value
 */
static inline float mavlink_msg_debug_get_value(const mavlink_message_t* msg)
{
	mavlink_debug_t *p = (mavlink_debug_t *)&msg->payload[0];
	return (float)(p->value);
}

/**
 * @brief Decode a debug message into a struct
 *
 * @param msg The message to decode
 * @param debug C-struct to decode the message contents into
 */
static inline void mavlink_msg_debug_decode(const mavlink_message_t* msg, mavlink_debug_t* debug)
{
	memcpy( debug, msg->payload, sizeof(mavlink_debug_t));
}
