// MESSAGE LOCAL_POSITION_SETPOINT PACKING

#define MAVLINK_MSG_ID_LOCAL_POSITION_SETPOINT 51
#define MAVLINK_MSG_ID_LOCAL_POSITION_SETPOINT_LEN 16
#define MAVLINK_MSG_51_LEN 16
#define MAVLINK_MSG_ID_LOCAL_POSITION_SETPOINT_KEY 0x4B
#define MAVLINK_MSG_51_KEY 0x4B

typedef struct __mavlink_local_position_setpoint_t 
{
	float x;	///< x position
	float y;	///< y position
	float z;	///< z position
	float yaw;	///< Desired yaw angle

} mavlink_local_position_setpoint_t;

/**
 * @brief Pack a local_position_setpoint message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param x x position
 * @param y y position
 * @param z z position
 * @param yaw Desired yaw angle
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_local_position_setpoint_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, float x, float y, float z, float yaw)
{
	mavlink_local_position_setpoint_t *p = (mavlink_local_position_setpoint_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_LOCAL_POSITION_SETPOINT;

	p->x = x;	// float:x position
	p->y = y;	// float:y position
	p->z = z;	// float:z position
	p->yaw = yaw;	// float:Desired yaw angle

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_LOCAL_POSITION_SETPOINT_LEN);
}

/**
 * @brief Pack a local_position_setpoint message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param x x position
 * @param y y position
 * @param z z position
 * @param yaw Desired yaw angle
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_local_position_setpoint_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, float x, float y, float z, float yaw)
{
	mavlink_local_position_setpoint_t *p = (mavlink_local_position_setpoint_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_LOCAL_POSITION_SETPOINT;

	p->x = x;	// float:x position
	p->y = y;	// float:y position
	p->z = z;	// float:z position
	p->yaw = yaw;	// float:Desired yaw angle

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_LOCAL_POSITION_SETPOINT_LEN);
}

/**
 * @brief Encode a local_position_setpoint struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param local_position_setpoint C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_local_position_setpoint_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_local_position_setpoint_t* local_position_setpoint)
{
	return mavlink_msg_local_position_setpoint_pack(system_id, component_id, msg, local_position_setpoint->x, local_position_setpoint->y, local_position_setpoint->z, local_position_setpoint->yaw);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a local_position_setpoint message
 * @param chan MAVLink channel to send the message
 *
 * @param x x position
 * @param y y position
 * @param z z position
 * @param yaw Desired yaw angle
 */
static inline void mavlink_msg_local_position_setpoint_send(mavlink_channel_t chan, float x, float y, float z, float yaw)
{
	mavlink_header_t hdr;
	mavlink_local_position_setpoint_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_LOCAL_POSITION_SETPOINT_LEN )
	payload.x = x;	// float:x position
	payload.y = y;	// float:y position
	payload.z = z;	// float:z position
	payload.yaw = yaw;	// float:Desired yaw angle

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_LOCAL_POSITION_SETPOINT_LEN;
	hdr.msgid = MAVLINK_MSG_ID_LOCAL_POSITION_SETPOINT;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0x4B, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE LOCAL_POSITION_SETPOINT UNPACKING

/**
 * @brief Get field x from local_position_setpoint message
 *
 * @return x position
 */
static inline float mavlink_msg_local_position_setpoint_get_x(const mavlink_message_t* msg)
{
	mavlink_local_position_setpoint_t *p = (mavlink_local_position_setpoint_t *)&msg->payload[0];
	return (float)(p->x);
}

/**
 * @brief Get field y from local_position_setpoint message
 *
 * @return y position
 */
static inline float mavlink_msg_local_position_setpoint_get_y(const mavlink_message_t* msg)
{
	mavlink_local_position_setpoint_t *p = (mavlink_local_position_setpoint_t *)&msg->payload[0];
	return (float)(p->y);
}

/**
 * @brief Get field z from local_position_setpoint message
 *
 * @return z position
 */
static inline float mavlink_msg_local_position_setpoint_get_z(const mavlink_message_t* msg)
{
	mavlink_local_position_setpoint_t *p = (mavlink_local_position_setpoint_t *)&msg->payload[0];
	return (float)(p->z);
}

/**
 * @brief Get field yaw from local_position_setpoint message
 *
 * @return Desired yaw angle
 */
static inline float mavlink_msg_local_position_setpoint_get_yaw(const mavlink_message_t* msg)
{
	mavlink_local_position_setpoint_t *p = (mavlink_local_position_setpoint_t *)&msg->payload[0];
	return (float)(p->yaw);
}

/**
 * @brief Decode a local_position_setpoint message into a struct
 *
 * @param msg The message to decode
 * @param local_position_setpoint C-struct to decode the message contents into
 */
static inline void mavlink_msg_local_position_setpoint_decode(const mavlink_message_t* msg, mavlink_local_position_setpoint_t* local_position_setpoint)
{
	memcpy( local_position_setpoint, msg->payload, sizeof(mavlink_local_position_setpoint_t));
}
