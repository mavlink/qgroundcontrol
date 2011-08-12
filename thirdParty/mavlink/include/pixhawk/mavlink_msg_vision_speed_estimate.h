// MESSAGE VISION_SPEED_ESTIMATE PACKING

#define MAVLINK_MSG_ID_VISION_SPEED_ESTIMATE 113
#define MAVLINK_MSG_ID_VISION_SPEED_ESTIMATE_LEN 20
#define MAVLINK_MSG_113_LEN 20

typedef struct __mavlink_vision_speed_estimate_t 
{
	uint64_t usec; ///< Timestamp (milliseconds)
	float x; ///< Global X speed
	float y; ///< Global Y speed
	float z; ///< Global Z speed

} mavlink_vision_speed_estimate_t;

/**
 * @brief Pack a vision_speed_estimate message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param usec Timestamp (milliseconds)
 * @param x Global X speed
 * @param y Global Y speed
 * @param z Global Z speed
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_vision_speed_estimate_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint64_t usec, float x, float y, float z)
{
	mavlink_vision_speed_estimate_t *p = (mavlink_vision_speed_estimate_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_VISION_SPEED_ESTIMATE;

	p->usec = usec; // uint64_t:Timestamp (milliseconds)
	p->x = x; // float:Global X speed
	p->y = y; // float:Global Y speed
	p->z = z; // float:Global Z speed

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_VISION_SPEED_ESTIMATE_LEN);
}

/**
 * @brief Pack a vision_speed_estimate message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param usec Timestamp (milliseconds)
 * @param x Global X speed
 * @param y Global Y speed
 * @param z Global Z speed
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_vision_speed_estimate_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint64_t usec, float x, float y, float z)
{
	mavlink_vision_speed_estimate_t *p = (mavlink_vision_speed_estimate_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_VISION_SPEED_ESTIMATE;

	p->usec = usec; // uint64_t:Timestamp (milliseconds)
	p->x = x; // float:Global X speed
	p->y = y; // float:Global Y speed
	p->z = z; // float:Global Z speed

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_VISION_SPEED_ESTIMATE_LEN);
}

/**
 * @brief Encode a vision_speed_estimate struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param vision_speed_estimate C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_vision_speed_estimate_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_vision_speed_estimate_t* vision_speed_estimate)
{
	return mavlink_msg_vision_speed_estimate_pack(system_id, component_id, msg, vision_speed_estimate->usec, vision_speed_estimate->x, vision_speed_estimate->y, vision_speed_estimate->z);
}

/**
 * @brief Send a vision_speed_estimate message
 * @param chan MAVLink channel to send the message
 *
 * @param usec Timestamp (milliseconds)
 * @param x Global X speed
 * @param y Global Y speed
 * @param z Global Z speed
 */


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_vision_speed_estimate_send(mavlink_channel_t chan, uint64_t usec, float x, float y, float z)
{
	mavlink_header_t hdr;
	mavlink_vision_speed_estimate_t payload;
	uint16_t checksum;
	mavlink_vision_speed_estimate_t *p = &payload;

	p->usec = usec; // uint64_t:Timestamp (milliseconds)
	p->x = x; // float:Global X speed
	p->y = y; // float:Global Y speed
	p->z = z; // float:Global Z speed

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_VISION_SPEED_ESTIMATE_LEN;
	hdr.msgid = MAVLINK_MSG_ID_VISION_SPEED_ESTIMATE;
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
// MESSAGE VISION_SPEED_ESTIMATE UNPACKING

/**
 * @brief Get field usec from vision_speed_estimate message
 *
 * @return Timestamp (milliseconds)
 */
static inline uint64_t mavlink_msg_vision_speed_estimate_get_usec(const mavlink_message_t* msg)
{
	mavlink_vision_speed_estimate_t *p = (mavlink_vision_speed_estimate_t *)&msg->payload[0];
	return (uint64_t)(p->usec);
}

/**
 * @brief Get field x from vision_speed_estimate message
 *
 * @return Global X speed
 */
static inline float mavlink_msg_vision_speed_estimate_get_x(const mavlink_message_t* msg)
{
	mavlink_vision_speed_estimate_t *p = (mavlink_vision_speed_estimate_t *)&msg->payload[0];
	return (float)(p->x);
}

/**
 * @brief Get field y from vision_speed_estimate message
 *
 * @return Global Y speed
 */
static inline float mavlink_msg_vision_speed_estimate_get_y(const mavlink_message_t* msg)
{
	mavlink_vision_speed_estimate_t *p = (mavlink_vision_speed_estimate_t *)&msg->payload[0];
	return (float)(p->y);
}

/**
 * @brief Get field z from vision_speed_estimate message
 *
 * @return Global Z speed
 */
static inline float mavlink_msg_vision_speed_estimate_get_z(const mavlink_message_t* msg)
{
	mavlink_vision_speed_estimate_t *p = (mavlink_vision_speed_estimate_t *)&msg->payload[0];
	return (float)(p->z);
}

/**
 * @brief Decode a vision_speed_estimate message into a struct
 *
 * @param msg The message to decode
 * @param vision_speed_estimate C-struct to decode the message contents into
 */
static inline void mavlink_msg_vision_speed_estimate_decode(const mavlink_message_t* msg, mavlink_vision_speed_estimate_t* vision_speed_estimate)
{
	memcpy( vision_speed_estimate, msg->payload, sizeof(mavlink_vision_speed_estimate_t));
}
