// MESSAGE MARKER PACKING

#define MAVLINK_MSG_ID_MARKER 130
#define MAVLINK_MSG_ID_MARKER_LEN 26
#define MAVLINK_MSG_130_LEN 26

typedef struct __mavlink_marker_t 
{
	float x; ///< x position
	float y; ///< y position
	float z; ///< z position
	float roll; ///< roll orientation
	float pitch; ///< pitch orientation
	float yaw; ///< yaw orientation
	uint16_t id; ///< ID

} mavlink_marker_t;

/**
 * @brief Pack a marker message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param id ID
 * @param x x position
 * @param y y position
 * @param z z position
 * @param roll roll orientation
 * @param pitch pitch orientation
 * @param yaw yaw orientation
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_marker_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint16_t id, float x, float y, float z, float roll, float pitch, float yaw)
{
	mavlink_marker_t *p = (mavlink_marker_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_MARKER;

	p->id = id; // uint16_t:ID
	p->x = x; // float:x position
	p->y = y; // float:y position
	p->z = z; // float:z position
	p->roll = roll; // float:roll orientation
	p->pitch = pitch; // float:pitch orientation
	p->yaw = yaw; // float:yaw orientation

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_MARKER_LEN);
}

/**
 * @brief Pack a marker message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param id ID
 * @param x x position
 * @param y y position
 * @param z z position
 * @param roll roll orientation
 * @param pitch pitch orientation
 * @param yaw yaw orientation
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_marker_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint16_t id, float x, float y, float z, float roll, float pitch, float yaw)
{
	mavlink_marker_t *p = (mavlink_marker_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_MARKER;

	p->id = id; // uint16_t:ID
	p->x = x; // float:x position
	p->y = y; // float:y position
	p->z = z; // float:z position
	p->roll = roll; // float:roll orientation
	p->pitch = pitch; // float:pitch orientation
	p->yaw = yaw; // float:yaw orientation

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_MARKER_LEN);
}

/**
 * @brief Encode a marker struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param marker C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_marker_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_marker_t* marker)
{
	return mavlink_msg_marker_pack(system_id, component_id, msg, marker->id, marker->x, marker->y, marker->z, marker->roll, marker->pitch, marker->yaw);
}

/**
 * @brief Send a marker message
 * @param chan MAVLink channel to send the message
 *
 * @param id ID
 * @param x x position
 * @param y y position
 * @param z z position
 * @param roll roll orientation
 * @param pitch pitch orientation
 * @param yaw yaw orientation
 */


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_marker_send(mavlink_channel_t chan, uint16_t id, float x, float y, float z, float roll, float pitch, float yaw)
{
	mavlink_header_t hdr;
	mavlink_marker_t payload;
	uint16_t checksum;
	mavlink_marker_t *p = &payload;

	p->id = id; // uint16_t:ID
	p->x = x; // float:x position
	p->y = y; // float:y position
	p->z = z; // float:z position
	p->roll = roll; // float:roll orientation
	p->pitch = pitch; // float:pitch orientation
	p->yaw = yaw; // float:yaw orientation

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_MARKER_LEN;
	hdr.msgid = MAVLINK_MSG_ID_MARKER;
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
// MESSAGE MARKER UNPACKING

/**
 * @brief Get field id from marker message
 *
 * @return ID
 */
static inline uint16_t mavlink_msg_marker_get_id(const mavlink_message_t* msg)
{
	mavlink_marker_t *p = (mavlink_marker_t *)&msg->payload[0];
	return (uint16_t)(p->id);
}

/**
 * @brief Get field x from marker message
 *
 * @return x position
 */
static inline float mavlink_msg_marker_get_x(const mavlink_message_t* msg)
{
	mavlink_marker_t *p = (mavlink_marker_t *)&msg->payload[0];
	return (float)(p->x);
}

/**
 * @brief Get field y from marker message
 *
 * @return y position
 */
static inline float mavlink_msg_marker_get_y(const mavlink_message_t* msg)
{
	mavlink_marker_t *p = (mavlink_marker_t *)&msg->payload[0];
	return (float)(p->y);
}

/**
 * @brief Get field z from marker message
 *
 * @return z position
 */
static inline float mavlink_msg_marker_get_z(const mavlink_message_t* msg)
{
	mavlink_marker_t *p = (mavlink_marker_t *)&msg->payload[0];
	return (float)(p->z);
}

/**
 * @brief Get field roll from marker message
 *
 * @return roll orientation
 */
static inline float mavlink_msg_marker_get_roll(const mavlink_message_t* msg)
{
	mavlink_marker_t *p = (mavlink_marker_t *)&msg->payload[0];
	return (float)(p->roll);
}

/**
 * @brief Get field pitch from marker message
 *
 * @return pitch orientation
 */
static inline float mavlink_msg_marker_get_pitch(const mavlink_message_t* msg)
{
	mavlink_marker_t *p = (mavlink_marker_t *)&msg->payload[0];
	return (float)(p->pitch);
}

/**
 * @brief Get field yaw from marker message
 *
 * @return yaw orientation
 */
static inline float mavlink_msg_marker_get_yaw(const mavlink_message_t* msg)
{
	mavlink_marker_t *p = (mavlink_marker_t *)&msg->payload[0];
	return (float)(p->yaw);
}

/**
 * @brief Decode a marker message into a struct
 *
 * @param msg The message to decode
 * @param marker C-struct to decode the message contents into
 */
static inline void mavlink_msg_marker_decode(const mavlink_message_t* msg, mavlink_marker_t* marker)
{
	memcpy( marker, msg->payload, sizeof(mavlink_marker_t));
}
