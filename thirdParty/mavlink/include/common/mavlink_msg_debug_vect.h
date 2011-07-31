// MESSAGE DEBUG_VECT PACKING

#define MAVLINK_MSG_ID_DEBUG_VECT 251
#define MAVLINK_MSG_ID_DEBUG_VECT_LEN 30
#define MAVLINK_MSG_251_LEN 30

typedef struct __mavlink_debug_vect_t 
{
	char name[10]; ///< Name
	uint64_t usec; ///< Timestamp
	float x; ///< x
	float y; ///< y
	float z; ///< z

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

	memcpy(p->name, name, sizeof(p->name)); // char[10]:Name
	p->usec = usec; // uint64_t:Timestamp
	p->x = x; // float:x
	p->y = y; // float:y
	p->z = z; // float:z

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

	memcpy(p->name, name, sizeof(p->name)); // char[10]:Name
	p->usec = usec; // uint64_t:Timestamp
	p->x = x; // float:x
	p->y = y; // float:y
	p->z = z; // float:z

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
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_debug_vect_send(mavlink_channel_t chan, const char* name, uint64_t usec, float x, float y, float z)
{
	mavlink_message_t msg;
	uint16_t checksum;
	mavlink_debug_vect_t *p = (mavlink_debug_vect_t *)&msg.payload[0];

	memcpy(p->name, name, sizeof(p->name)); // char[10]:Name
	p->usec = usec; // uint64_t:Timestamp
	p->x = x; // float:x
	p->y = y; // float:y
	p->z = z; // float:z

	msg.STX = MAVLINK_STX;
	msg.len = MAVLINK_MSG_ID_DEBUG_VECT_LEN;
	msg.msgid = MAVLINK_MSG_ID_DEBUG_VECT;
	msg.sysid = mavlink_system.sysid;
	msg.compid = mavlink_system.compid;
	msg.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = msg.seq + 1;
	checksum = crc_calculate_msg(&msg, msg.len + MAVLINK_CORE_HEADER_LEN);
	msg.ck_a = (uint8_t)(checksum & 0xFF); ///< Low byte
	msg.ck_b = (uint8_t)(checksum >> 8); ///< High byte

	mavlink_send_msg(chan, &msg);
}

#endif

#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS_SMALL
static inline void mavlink_msg_debug_vect_send(mavlink_channel_t chan, const char* name, uint64_t usec, float x, float y, float z)
{
	mavlink_header_t hdr;
	mavlink_debug_vect_t payload;
	uint16_t checksum;
	mavlink_debug_vect_t *p = &payload;

	memcpy(p->name, name, sizeof(p->name)); // char[10]:Name
	p->usec = usec; // uint64_t:Timestamp
	p->x = x; // float:x
	p->y = y; // float:y
	p->z = z; // float:z

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_DEBUG_VECT_LEN;
	hdr.msgid = MAVLINK_MSG_ID_DEBUG_VECT;
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
