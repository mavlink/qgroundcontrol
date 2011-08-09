// MESSAGE LOCAL_POSITION_SETPOINT_SET PACKING

#define MAVLINK_MSG_ID_LOCAL_POSITION_SETPOINT_SET 50
#define MAVLINK_MSG_ID_LOCAL_POSITION_SETPOINT_SET_LEN 18
#define MAVLINK_MSG_50_LEN 18

typedef struct __mavlink_local_position_setpoint_set_t 
{
	float x; ///< x position
	float y; ///< y position
	float z; ///< z position
	float yaw; ///< Desired yaw angle
	uint8_t target_system; ///< System ID
	uint8_t target_component; ///< Component ID

} mavlink_local_position_setpoint_set_t;

/**
 * @brief Pack a local_position_setpoint_set message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param x x position
 * @param y y position
 * @param z z position
 * @param yaw Desired yaw angle
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_local_position_setpoint_set_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component, float x, float y, float z, float yaw)
{
	mavlink_local_position_setpoint_set_t *p = (mavlink_local_position_setpoint_set_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_LOCAL_POSITION_SETPOINT_SET;

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	p->x = x; // float:x position
	p->y = y; // float:y position
	p->z = z; // float:z position
	p->yaw = yaw; // float:Desired yaw angle

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_LOCAL_POSITION_SETPOINT_SET_LEN);
}

/**
 * @brief Pack a local_position_setpoint_set message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system System ID
 * @param target_component Component ID
 * @param x x position
 * @param y y position
 * @param z z position
 * @param yaw Desired yaw angle
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_local_position_setpoint_set_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component, float x, float y, float z, float yaw)
{
	mavlink_local_position_setpoint_set_t *p = (mavlink_local_position_setpoint_set_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_LOCAL_POSITION_SETPOINT_SET;

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	p->x = x; // float:x position
	p->y = y; // float:y position
	p->z = z; // float:z position
	p->yaw = yaw; // float:Desired yaw angle

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_LOCAL_POSITION_SETPOINT_SET_LEN);
}

/**
 * @brief Encode a local_position_setpoint_set struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param local_position_setpoint_set C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_local_position_setpoint_set_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_local_position_setpoint_set_t* local_position_setpoint_set)
{
	return mavlink_msg_local_position_setpoint_set_pack(system_id, component_id, msg, local_position_setpoint_set->target_system, local_position_setpoint_set->target_component, local_position_setpoint_set->x, local_position_setpoint_set->y, local_position_setpoint_set->z, local_position_setpoint_set->yaw);
}

/**
 * @brief Send a local_position_setpoint_set message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param x x position
 * @param y y position
 * @param z z position
 * @param yaw Desired yaw angle
 */


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_local_position_setpoint_set_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, float x, float y, float z, float yaw)
{
	mavlink_header_t hdr;
	mavlink_local_position_setpoint_set_t payload;
	uint16_t checksum;
	mavlink_local_position_setpoint_set_t *p = &payload;

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	p->x = x; // float:x position
	p->y = y; // float:y position
	p->z = z; // float:z position
	p->yaw = yaw; // float:Desired yaw angle

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_LOCAL_POSITION_SETPOINT_SET_LEN;
	hdr.msgid = MAVLINK_MSG_ID_LOCAL_POSITION_SETPOINT_SET;
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
// MESSAGE LOCAL_POSITION_SETPOINT_SET UNPACKING

/**
 * @brief Get field target_system from local_position_setpoint_set message
 *
 * @return System ID
 */
static inline uint8_t mavlink_msg_local_position_setpoint_set_get_target_system(const mavlink_message_t* msg)
{
	mavlink_local_position_setpoint_set_t *p = (mavlink_local_position_setpoint_set_t *)&msg->payload[0];
	return (uint8_t)(p->target_system);
}

/**
 * @brief Get field target_component from local_position_setpoint_set message
 *
 * @return Component ID
 */
static inline uint8_t mavlink_msg_local_position_setpoint_set_get_target_component(const mavlink_message_t* msg)
{
	mavlink_local_position_setpoint_set_t *p = (mavlink_local_position_setpoint_set_t *)&msg->payload[0];
	return (uint8_t)(p->target_component);
}

/**
 * @brief Get field x from local_position_setpoint_set message
 *
 * @return x position
 */
static inline float mavlink_msg_local_position_setpoint_set_get_x(const mavlink_message_t* msg)
{
	mavlink_local_position_setpoint_set_t *p = (mavlink_local_position_setpoint_set_t *)&msg->payload[0];
	return (float)(p->x);
}

/**
 * @brief Get field y from local_position_setpoint_set message
 *
 * @return y position
 */
static inline float mavlink_msg_local_position_setpoint_set_get_y(const mavlink_message_t* msg)
{
	mavlink_local_position_setpoint_set_t *p = (mavlink_local_position_setpoint_set_t *)&msg->payload[0];
	return (float)(p->y);
}

/**
 * @brief Get field z from local_position_setpoint_set message
 *
 * @return z position
 */
static inline float mavlink_msg_local_position_setpoint_set_get_z(const mavlink_message_t* msg)
{
	mavlink_local_position_setpoint_set_t *p = (mavlink_local_position_setpoint_set_t *)&msg->payload[0];
	return (float)(p->z);
}

/**
 * @brief Get field yaw from local_position_setpoint_set message
 *
 * @return Desired yaw angle
 */
static inline float mavlink_msg_local_position_setpoint_set_get_yaw(const mavlink_message_t* msg)
{
	mavlink_local_position_setpoint_set_t *p = (mavlink_local_position_setpoint_set_t *)&msg->payload[0];
	return (float)(p->yaw);
}

/**
 * @brief Decode a local_position_setpoint_set message into a struct
 *
 * @param msg The message to decode
 * @param local_position_setpoint_set C-struct to decode the message contents into
 */
static inline void mavlink_msg_local_position_setpoint_set_decode(const mavlink_message_t* msg, mavlink_local_position_setpoint_set_t* local_position_setpoint_set)
{
	memcpy( local_position_setpoint_set, msg->payload, sizeof(mavlink_local_position_setpoint_set_t));
}
