// MESSAGE GLOBAL_POSITION_SETPOINT_INT PACKING

#define MAVLINK_MSG_ID_GLOBAL_POSITION_SETPOINT_INT 52
#define MAVLINK_MSG_ID_GLOBAL_POSITION_SETPOINT_INT_LEN 14
#define MAVLINK_MSG_52_LEN 14
#define MAVLINK_MSG_ID_GLOBAL_POSITION_SETPOINT_INT_KEY 0x14
#define MAVLINK_MSG_52_KEY 0x14

typedef struct __mavlink_global_position_setpoint_int_t 
{
	int32_t latitude;	///< WGS84 Latitude position in degrees * 1E7
	int32_t longitude;	///< WGS84 Longitude position in degrees * 1E7
	int32_t altitude;	///< WGS84 Altitude in meters * 1000 (positive for up)
	int16_t yaw;	///< Desired yaw angle in degrees * 100

} mavlink_global_position_setpoint_int_t;

/**
 * @brief Pack a global_position_setpoint_int message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param latitude WGS84 Latitude position in degrees * 1E7
 * @param longitude WGS84 Longitude position in degrees * 1E7
 * @param altitude WGS84 Altitude in meters * 1000 (positive for up)
 * @param yaw Desired yaw angle in degrees * 100
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_global_position_setpoint_int_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, int32_t latitude, int32_t longitude, int32_t altitude, int16_t yaw)
{
	mavlink_global_position_setpoint_int_t *p = (mavlink_global_position_setpoint_int_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_GLOBAL_POSITION_SETPOINT_INT;

	p->latitude = latitude;	// int32_t:WGS84 Latitude position in degrees * 1E7
	p->longitude = longitude;	// int32_t:WGS84 Longitude position in degrees * 1E7
	p->altitude = altitude;	// int32_t:WGS84 Altitude in meters * 1000 (positive for up)
	p->yaw = yaw;	// int16_t:Desired yaw angle in degrees * 100

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_GLOBAL_POSITION_SETPOINT_INT_LEN);
}

/**
 * @brief Pack a global_position_setpoint_int message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param latitude WGS84 Latitude position in degrees * 1E7
 * @param longitude WGS84 Longitude position in degrees * 1E7
 * @param altitude WGS84 Altitude in meters * 1000 (positive for up)
 * @param yaw Desired yaw angle in degrees * 100
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_global_position_setpoint_int_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, int32_t latitude, int32_t longitude, int32_t altitude, int16_t yaw)
{
	mavlink_global_position_setpoint_int_t *p = (mavlink_global_position_setpoint_int_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_GLOBAL_POSITION_SETPOINT_INT;

	p->latitude = latitude;	// int32_t:WGS84 Latitude position in degrees * 1E7
	p->longitude = longitude;	// int32_t:WGS84 Longitude position in degrees * 1E7
	p->altitude = altitude;	// int32_t:WGS84 Altitude in meters * 1000 (positive for up)
	p->yaw = yaw;	// int16_t:Desired yaw angle in degrees * 100

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_GLOBAL_POSITION_SETPOINT_INT_LEN);
}

/**
 * @brief Encode a global_position_setpoint_int struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param global_position_setpoint_int C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_global_position_setpoint_int_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_global_position_setpoint_int_t* global_position_setpoint_int)
{
	return mavlink_msg_global_position_setpoint_int_pack(system_id, component_id, msg, global_position_setpoint_int->latitude, global_position_setpoint_int->longitude, global_position_setpoint_int->altitude, global_position_setpoint_int->yaw);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a global_position_setpoint_int message
 * @param chan MAVLink channel to send the message
 *
 * @param latitude WGS84 Latitude position in degrees * 1E7
 * @param longitude WGS84 Longitude position in degrees * 1E7
 * @param altitude WGS84 Altitude in meters * 1000 (positive for up)
 * @param yaw Desired yaw angle in degrees * 100
 */
static inline void mavlink_msg_global_position_setpoint_int_send(mavlink_channel_t chan, int32_t latitude, int32_t longitude, int32_t altitude, int16_t yaw)
{
	mavlink_header_t hdr;
	mavlink_global_position_setpoint_int_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_GLOBAL_POSITION_SETPOINT_INT_LEN )
	payload.latitude = latitude;	// int32_t:WGS84 Latitude position in degrees * 1E7
	payload.longitude = longitude;	// int32_t:WGS84 Longitude position in degrees * 1E7
	payload.altitude = altitude;	// int32_t:WGS84 Altitude in meters * 1000 (positive for up)
	payload.yaw = yaw;	// int16_t:Desired yaw angle in degrees * 100

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_GLOBAL_POSITION_SETPOINT_INT_LEN;
	hdr.msgid = MAVLINK_MSG_ID_GLOBAL_POSITION_SETPOINT_INT;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0x14, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE GLOBAL_POSITION_SETPOINT_INT UNPACKING

/**
 * @brief Get field latitude from global_position_setpoint_int message
 *
 * @return WGS84 Latitude position in degrees * 1E7
 */
static inline int32_t mavlink_msg_global_position_setpoint_int_get_latitude(const mavlink_message_t* msg)
{
	mavlink_global_position_setpoint_int_t *p = (mavlink_global_position_setpoint_int_t *)&msg->payload[0];
	return (int32_t)(p->latitude);
}

/**
 * @brief Get field longitude from global_position_setpoint_int message
 *
 * @return WGS84 Longitude position in degrees * 1E7
 */
static inline int32_t mavlink_msg_global_position_setpoint_int_get_longitude(const mavlink_message_t* msg)
{
	mavlink_global_position_setpoint_int_t *p = (mavlink_global_position_setpoint_int_t *)&msg->payload[0];
	return (int32_t)(p->longitude);
}

/**
 * @brief Get field altitude from global_position_setpoint_int message
 *
 * @return WGS84 Altitude in meters * 1000 (positive for up)
 */
static inline int32_t mavlink_msg_global_position_setpoint_int_get_altitude(const mavlink_message_t* msg)
{
	mavlink_global_position_setpoint_int_t *p = (mavlink_global_position_setpoint_int_t *)&msg->payload[0];
	return (int32_t)(p->altitude);
}

/**
 * @brief Get field yaw from global_position_setpoint_int message
 *
 * @return Desired yaw angle in degrees * 100
 */
static inline int16_t mavlink_msg_global_position_setpoint_int_get_yaw(const mavlink_message_t* msg)
{
	mavlink_global_position_setpoint_int_t *p = (mavlink_global_position_setpoint_int_t *)&msg->payload[0];
	return (int16_t)(p->yaw);
}

/**
 * @brief Decode a global_position_setpoint_int message into a struct
 *
 * @param msg The message to decode
 * @param global_position_setpoint_int C-struct to decode the message contents into
 */
static inline void mavlink_msg_global_position_setpoint_int_decode(const mavlink_message_t* msg, mavlink_global_position_setpoint_int_t* global_position_setpoint_int)
{
	memcpy( global_position_setpoint_int, msg->payload, sizeof(mavlink_global_position_setpoint_int_t));
}
