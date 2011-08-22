// MESSAGE NAMED_VALUE_INT PACKING

#define MAVLINK_MSG_ID_NAMED_VALUE_INT 253
#define MAVLINK_MSG_ID_NAMED_VALUE_INT_LEN 14
#define MAVLINK_MSG_253_LEN 14
#define MAVLINK_MSG_ID_NAMED_VALUE_INT_KEY 0xD3
#define MAVLINK_MSG_253_KEY 0xD3

typedef struct __mavlink_named_value_int_t 
{
	int32_t value;	///< Signed integer value
	char name[10];	///< Name of the debug variable

} mavlink_named_value_int_t;
#define MAVLINK_MSG_NAMED_VALUE_INT_FIELD_NAME_LEN 10

/**
 * @brief Pack a named_value_int message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param name Name of the debug variable
 * @param value Signed integer value
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_named_value_int_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const char* name, int32_t value)
{
	mavlink_named_value_int_t *p = (mavlink_named_value_int_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_NAMED_VALUE_INT;

	memcpy(p->name, name, sizeof(p->name));	// char[10]:Name of the debug variable
	p->value = value;	// int32_t:Signed integer value

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_NAMED_VALUE_INT_LEN);
}

/**
 * @brief Pack a named_value_int message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param name Name of the debug variable
 * @param value Signed integer value
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_named_value_int_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const char* name, int32_t value)
{
	mavlink_named_value_int_t *p = (mavlink_named_value_int_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_NAMED_VALUE_INT;

	memcpy(p->name, name, sizeof(p->name));	// char[10]:Name of the debug variable
	p->value = value;	// int32_t:Signed integer value

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_NAMED_VALUE_INT_LEN);
}

/**
 * @brief Encode a named_value_int struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param named_value_int C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_named_value_int_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_named_value_int_t* named_value_int)
{
	return mavlink_msg_named_value_int_pack(system_id, component_id, msg, named_value_int->name, named_value_int->value);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a named_value_int message
 * @param chan MAVLink channel to send the message
 *
 * @param name Name of the debug variable
 * @param value Signed integer value
 */
static inline void mavlink_msg_named_value_int_send(mavlink_channel_t chan, const char* name, int32_t value)
{
	mavlink_header_t hdr;
	mavlink_named_value_int_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_NAMED_VALUE_INT_LEN )
	memcpy(payload.name, name, sizeof(payload.name));	// char[10]:Name of the debug variable
	payload.value = value;	// int32_t:Signed integer value

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_NAMED_VALUE_INT_LEN;
	hdr.msgid = MAVLINK_MSG_ID_NAMED_VALUE_INT;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0xD3, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE NAMED_VALUE_INT UNPACKING

/**
 * @brief Get field name from named_value_int message
 *
 * @return Name of the debug variable
 */
static inline uint16_t mavlink_msg_named_value_int_get_name(const mavlink_message_t* msg, char* name)
{
	mavlink_named_value_int_t *p = (mavlink_named_value_int_t *)&msg->payload[0];

	memcpy(name, p->name, sizeof(p->name));
	return sizeof(p->name);
}

/**
 * @brief Get field value from named_value_int message
 *
 * @return Signed integer value
 */
static inline int32_t mavlink_msg_named_value_int_get_value(const mavlink_message_t* msg)
{
	mavlink_named_value_int_t *p = (mavlink_named_value_int_t *)&msg->payload[0];
	return (int32_t)(p->value);
}

/**
 * @brief Decode a named_value_int message into a struct
 *
 * @param msg The message to decode
 * @param named_value_int C-struct to decode the message contents into
 */
static inline void mavlink_msg_named_value_int_decode(const mavlink_message_t* msg, mavlink_named_value_int_t* named_value_int)
{
	memcpy( named_value_int, msg->payload, sizeof(mavlink_named_value_int_t));
}
