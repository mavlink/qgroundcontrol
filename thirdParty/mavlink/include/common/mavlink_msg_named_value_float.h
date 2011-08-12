// MESSAGE NAMED_VALUE_FLOAT PACKING

#define MAVLINK_MSG_ID_NAMED_VALUE_FLOAT 252
#define MAVLINK_MSG_ID_NAMED_VALUE_FLOAT_LEN 14
#define MAVLINK_MSG_252_LEN 14

typedef struct __mavlink_named_value_float_t 
{
	float value; ///< Floating point value
	char name[10]; ///< Name of the debug variable

} mavlink_named_value_float_t;
#define MAVLINK_MSG_NAMED_VALUE_FLOAT_FIELD_NAME_LEN 10

/**
 * @brief Pack a named_value_float message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param name Name of the debug variable
 * @param value Floating point value
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_named_value_float_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const char* name, float value)
{
	mavlink_named_value_float_t *p = (mavlink_named_value_float_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_NAMED_VALUE_FLOAT;

	memcpy(p->name, name, sizeof(p->name)); // char[10]:Name of the debug variable
	p->value = value; // float:Floating point value

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_NAMED_VALUE_FLOAT_LEN);
}

/**
 * @brief Pack a named_value_float message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param name Name of the debug variable
 * @param value Floating point value
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_named_value_float_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const char* name, float value)
{
	mavlink_named_value_float_t *p = (mavlink_named_value_float_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_NAMED_VALUE_FLOAT;

	memcpy(p->name, name, sizeof(p->name)); // char[10]:Name of the debug variable
	p->value = value; // float:Floating point value

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_NAMED_VALUE_FLOAT_LEN);
}

/**
 * @brief Encode a named_value_float struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param named_value_float C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_named_value_float_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_named_value_float_t* named_value_float)
{
	return mavlink_msg_named_value_float_pack(system_id, component_id, msg, named_value_float->name, named_value_float->value);
}

/**
 * @brief Send a named_value_float message
 * @param chan MAVLink channel to send the message
 *
 * @param name Name of the debug variable
 * @param value Floating point value
 */


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_named_value_float_send(mavlink_channel_t chan, const char* name, float value)
{
	mavlink_header_t hdr;
	mavlink_named_value_float_t payload;
	uint16_t checksum;
	mavlink_named_value_float_t *p = &payload;

	memcpy(p->name, name, sizeof(p->name)); // char[10]:Name of the debug variable
	p->value = value; // float:Floating point value

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_NAMED_VALUE_FLOAT_LEN;
	hdr.msgid = MAVLINK_MSG_ID_NAMED_VALUE_FLOAT;
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
// MESSAGE NAMED_VALUE_FLOAT UNPACKING

/**
 * @brief Get field name from named_value_float message
 *
 * @return Name of the debug variable
 */
static inline uint16_t mavlink_msg_named_value_float_get_name(const mavlink_message_t* msg, char* name)
{
	mavlink_named_value_float_t *p = (mavlink_named_value_float_t *)&msg->payload[0];

	memcpy(name, p->name, sizeof(p->name));
	return sizeof(p->name);
}

/**
 * @brief Get field value from named_value_float message
 *
 * @return Floating point value
 */
static inline float mavlink_msg_named_value_float_get_value(const mavlink_message_t* msg)
{
	mavlink_named_value_float_t *p = (mavlink_named_value_float_t *)&msg->payload[0];
	return (float)(p->value);
}

/**
 * @brief Decode a named_value_float message into a struct
 *
 * @param msg The message to decode
 * @param named_value_float C-struct to decode the message contents into
 */
static inline void mavlink_msg_named_value_float_decode(const mavlink_message_t* msg, mavlink_named_value_float_t* named_value_float)
{
	memcpy( named_value_float, msg->payload, sizeof(mavlink_named_value_float_t));
}
