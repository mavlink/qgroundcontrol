// MESSAGE DEBUG_VECT PACKING

#define MAVLINK_MSG_ID_DEBUG_VECT 251
#define MAVLINK_MSG_ID_DEBUG_VECT_LEN 30
#define MAVLINK_MSG_251_LEN 30
#define MAVLINK_MSG_ID_DEBUG_VECT_KEY 0x2B
#define MAVLINK_MSG_251_KEY 0x2B

typedef struct __mavlink_debug_vect_t 
{
	uint64_t usec;	///< Timestamp
	float x;	///< x
	float y;	///< y
	float z;	///< z
	char name[10];	///< Name

} mavlink_debug_vect_t;
#define MAVLINK_MSG_DEBUG_VECT_FIELD_NAME_LEN 10

/**
 * @brief Pack a debug_vect message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param name Name
 * @param usec Timestamp
 * @param x x
 * @param y y
 * @param z z
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_debug_vect_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const char* name, uint64_t usec, float x, float y, float z)
{
	mavlink_debug_vect_t *p = (mavlink_debug_vect_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_DEBUG_VECT;

	memcpy(p->name, name, sizeof(p->name));	// char[10]:Name
	p->usec = usec;	// uint64_t:Timestamp
	p->x = x;	// float:x
	p->y = y;	// float:y
	p->z = z;	// float:z

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_DEBUG_VECT_LEN);
}

/**
 * @brief Pack a debug_vect message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param name Name
 * @param usec Timestamp
 * @param x x
 * @param y y
 * @param z z
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_debug_vect_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const char* name, uint64_t usec, float x, float y, float z)
{
	mavlink_debug_vect_t *p = (mavlink_debug_vect_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_DEBUG_VECT;

	memcpy(p->name, name, sizeof(p->name));	// char[10]:Name
	p->usec = usec;	// uint64_t:Timestamp
	p->x = x;	// float:x
	p->y = y;	// float:y
	p->z = z;	// float:z

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_DEBUG_VECT_LEN);
}

/**
 * @brief Encode a debug_vect struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param debug_vect C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_debug_vect_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_debug_vect_t* debug_vect)
{
	return mavlink_msg_debug_vect_pack(system_id, component_id, msg, debug_vect->name, debug_vect->usec, debug_vect->x, debug_vect->y, debug_vect->z);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a debug_vect message
 * @param chan MAVLink channel to send the message
 *
 * @param name Name
 * @param usec Timestamp
 * @param x x
 * @param y y
 * @param z z
 */
static inline void mavlink_msg_debug_vect_send(mavlink_channel_t chan, const char* name, uint64_t usec, float x, float y, float z)
{
	mavlink_header_t hdr;
	mavlink_debug_vect_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_DEBUG_VECT_LEN )
	memcpy(payload.name, name, sizeof(payload.name));	// char[10]:Name
	payload.usec = usec;	// uint64_t:Timestamp
	payload.x = x;	// float:x
	payload.y = y;	// float:y
	payload.z = z;	// float:z

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_DEBUG_VECT_LEN;
	hdr.msgid = MAVLINK_MSG_ID_DEBUG_VECT;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0x2B, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE DEBUG_VECT UNPACKING

/**
 * @brief Get field name from debug_vect message
 *
 * @return Name
 */
static inline uint16_t mavlink_msg_debug_vect_get_name(const mavlink_message_t* msg, char* name)
{
	mavlink_debug_vect_t *p = (mavlink_debug_vect_t *)&msg->payload[0];

	memcpy(name, p->name, sizeof(p->name));
	return sizeof(p->name);
}

/**
 * @brief Get field usec from debug_vect message
 *
 * @return Timestamp
 */
static inline uint64_t mavlink_msg_debug_vect_get_usec(const mavlink_message_t* msg)
{
	mavlink_debug_vect_t *p = (mavlink_debug_vect_t *)&msg->payload[0];
	return (uint64_t)(p->usec);
}

/**
 * @brief Get field x from debug_vect message
 *
 * @return x
 */
static inline float mavlink_msg_debug_vect_get_x(const mavlink_message_t* msg)
{
	mavlink_debug_vect_t *p = (mavlink_debug_vect_t *)&msg->payload[0];
	return (float)(p->x);
}

/**
 * @brief Get field y from debug_vect message
 *
 * @return y
 */
static inline float mavlink_msg_debug_vect_get_y(const mavlink_message_t* msg)
{
	mavlink_debug_vect_t *p = (mavlink_debug_vect_t *)&msg->payload[0];
	return (float)(p->y);
}

/**
 * @brief Get field z from debug_vect message
 *
 * @return z
 */
static inline float mavlink_msg_debug_vect_get_z(const mavlink_message_t* msg)
{
	mavlink_debug_vect_t *p = (mavlink_debug_vect_t *)&msg->payload[0];
	return (float)(p->z);
}

/**
 * @brief Decode a debug_vect message into a struct
 *
 * @param msg The message to decode
 * @param debug_vect C-struct to decode the message contents into
 */
static inline void mavlink_msg_debug_vect_decode(const mavlink_message_t* msg, mavlink_debug_vect_t* debug_vect)
{
	memcpy( debug_vect, msg->payload, sizeof(mavlink_debug_vect_t));
}
