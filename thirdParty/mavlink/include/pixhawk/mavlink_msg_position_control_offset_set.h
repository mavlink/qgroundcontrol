// MESSAGE POSITION_CONTROL_OFFSET_SET PACKING

#define MAVLINK_MSG_ID_POSITION_CONTROL_OFFSET_SET 154
#define MAVLINK_MSG_ID_POSITION_CONTROL_OFFSET_SET_LEN 18
#define MAVLINK_MSG_154_LEN 18

typedef struct __mavlink_position_control_offset_set_t 
{
	uint8_t target_system; ///< System ID
	uint8_t target_component; ///< Component ID
	float x; ///< x position offset
	float y; ///< y position offset
	float z; ///< z position offset
	float yaw; ///< yaw orientation offset in radians, 0 = NORTH

} mavlink_position_control_offset_set_t;

/**
 * @brief Pack a position_control_offset_set message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param x x position offset
 * @param y y position offset
 * @param z z position offset
 * @param yaw yaw orientation offset in radians, 0 = NORTH
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_position_control_offset_set_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component, float x, float y, float z, float yaw)
{
	mavlink_position_control_offset_set_t *p = (mavlink_position_control_offset_set_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_POSITION_CONTROL_OFFSET_SET;

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	p->x = x; // float:x position offset
	p->y = y; // float:y position offset
	p->z = z; // float:z position offset
	p->yaw = yaw; // float:yaw orientation offset in radians, 0 = NORTH

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_POSITION_CONTROL_OFFSET_SET_LEN);
}

/**
 * @brief Pack a position_control_offset_set message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system System ID
 * @param target_component Component ID
 * @param x x position offset
 * @param y y position offset
 * @param z z position offset
 * @param yaw yaw orientation offset in radians, 0 = NORTH
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_position_control_offset_set_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component, float x, float y, float z, float yaw)
{
	mavlink_position_control_offset_set_t *p = (mavlink_position_control_offset_set_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_POSITION_CONTROL_OFFSET_SET;

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	p->x = x; // float:x position offset
	p->y = y; // float:y position offset
	p->z = z; // float:z position offset
	p->yaw = yaw; // float:yaw orientation offset in radians, 0 = NORTH

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_POSITION_CONTROL_OFFSET_SET_LEN);
}

/**
 * @brief Encode a position_control_offset_set struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param position_control_offset_set C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_position_control_offset_set_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_position_control_offset_set_t* position_control_offset_set)
{
	return mavlink_msg_position_control_offset_set_pack(system_id, component_id, msg, position_control_offset_set->target_system, position_control_offset_set->target_component, position_control_offset_set->x, position_control_offset_set->y, position_control_offset_set->z, position_control_offset_set->yaw);
}

/**
 * @brief Send a position_control_offset_set message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param x x position offset
 * @param y y position offset
 * @param z z position offset
 * @param yaw yaw orientation offset in radians, 0 = NORTH
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_position_control_offset_set_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, float x, float y, float z, float yaw)
{
	mavlink_message_t msg;
	uint16_t checksum;
	mavlink_position_control_offset_set_t *p = (mavlink_position_control_offset_set_t *)&msg.payload[0];

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	p->x = x; // float:x position offset
	p->y = y; // float:y position offset
	p->z = z; // float:z position offset
	p->yaw = yaw; // float:yaw orientation offset in radians, 0 = NORTH

	msg.STX = MAVLINK_STX;
	msg.len = MAVLINK_MSG_ID_POSITION_CONTROL_OFFSET_SET_LEN;
	msg.msgid = MAVLINK_MSG_ID_POSITION_CONTROL_OFFSET_SET;
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
static inline void mavlink_msg_position_control_offset_set_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, float x, float y, float z, float yaw)
{
	mavlink_header_t hdr;
	mavlink_position_control_offset_set_t payload;
	uint16_t checksum;
	mavlink_position_control_offset_set_t *p = &payload;

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	p->x = x; // float:x position offset
	p->y = y; // float:y position offset
	p->z = z; // float:z position offset
	p->yaw = yaw; // float:yaw orientation offset in radians, 0 = NORTH

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_POSITION_CONTROL_OFFSET_SET_LEN;
	hdr.msgid = MAVLINK_MSG_ID_POSITION_CONTROL_OFFSET_SET;
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
// MESSAGE POSITION_CONTROL_OFFSET_SET UNPACKING

/**
 * @brief Get field target_system from position_control_offset_set message
 *
 * @return System ID
 */
static inline uint8_t mavlink_msg_position_control_offset_set_get_target_system(const mavlink_message_t* msg)
{
	mavlink_position_control_offset_set_t *p = (mavlink_position_control_offset_set_t *)&msg->payload[0];
	return (uint8_t)(p->target_system);
}

/**
 * @brief Get field target_component from position_control_offset_set message
 *
 * @return Component ID
 */
static inline uint8_t mavlink_msg_position_control_offset_set_get_target_component(const mavlink_message_t* msg)
{
	mavlink_position_control_offset_set_t *p = (mavlink_position_control_offset_set_t *)&msg->payload[0];
	return (uint8_t)(p->target_component);
}

/**
 * @brief Get field x from position_control_offset_set message
 *
 * @return x position offset
 */
static inline float mavlink_msg_position_control_offset_set_get_x(const mavlink_message_t* msg)
{
	mavlink_position_control_offset_set_t *p = (mavlink_position_control_offset_set_t *)&msg->payload[0];
	return (float)(p->x);
}

/**
 * @brief Get field y from position_control_offset_set message
 *
 * @return y position offset
 */
static inline float mavlink_msg_position_control_offset_set_get_y(const mavlink_message_t* msg)
{
	mavlink_position_control_offset_set_t *p = (mavlink_position_control_offset_set_t *)&msg->payload[0];
	return (float)(p->y);
}

/**
 * @brief Get field z from position_control_offset_set message
 *
 * @return z position offset
 */
static inline float mavlink_msg_position_control_offset_set_get_z(const mavlink_message_t* msg)
{
	mavlink_position_control_offset_set_t *p = (mavlink_position_control_offset_set_t *)&msg->payload[0];
	return (float)(p->z);
}

/**
 * @brief Get field yaw from position_control_offset_set message
 *
 * @return yaw orientation offset in radians, 0 = NORTH
 */
static inline float mavlink_msg_position_control_offset_set_get_yaw(const mavlink_message_t* msg)
{
	mavlink_position_control_offset_set_t *p = (mavlink_position_control_offset_set_t *)&msg->payload[0];
	return (float)(p->yaw);
}

/**
 * @brief Decode a position_control_offset_set message into a struct
 *
 * @param msg The message to decode
 * @param position_control_offset_set C-struct to decode the message contents into
 */
static inline void mavlink_msg_position_control_offset_set_decode(const mavlink_message_t* msg, mavlink_position_control_offset_set_t* position_control_offset_set)
{
	memcpy( position_control_offset_set, msg->payload, sizeof(mavlink_position_control_offset_set_t));
}
