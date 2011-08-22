// MESSAGE GPS_SET_GLOBAL_ORIGIN PACKING

#define MAVLINK_MSG_ID_GPS_SET_GLOBAL_ORIGIN 48
#define MAVLINK_MSG_ID_GPS_SET_GLOBAL_ORIGIN_LEN 14
#define MAVLINK_MSG_48_LEN 14
#define MAVLINK_MSG_ID_GPS_SET_GLOBAL_ORIGIN_KEY 0x8E
#define MAVLINK_MSG_48_KEY 0x8E

typedef struct __mavlink_gps_set_global_origin_t 
{
	int32_t latitude;	///< global position * 1E7
	int32_t longitude;	///< global position * 1E7
	int32_t altitude;	///< global position * 1000
	uint8_t target_system;	///< System ID
	uint8_t target_component;	///< Component ID

} mavlink_gps_set_global_origin_t;

/**
 * @brief Pack a gps_set_global_origin message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param latitude global position * 1E7
 * @param longitude global position * 1E7
 * @param altitude global position * 1000
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_gps_set_global_origin_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component, int32_t latitude, int32_t longitude, int32_t altitude)
{
	mavlink_gps_set_global_origin_t *p = (mavlink_gps_set_global_origin_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_GPS_SET_GLOBAL_ORIGIN;

	p->target_system = target_system;	// uint8_t:System ID
	p->target_component = target_component;	// uint8_t:Component ID
	p->latitude = latitude;	// int32_t:global position * 1E7
	p->longitude = longitude;	// int32_t:global position * 1E7
	p->altitude = altitude;	// int32_t:global position * 1000

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_GPS_SET_GLOBAL_ORIGIN_LEN);
}

/**
 * @brief Pack a gps_set_global_origin message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system System ID
 * @param target_component Component ID
 * @param latitude global position * 1E7
 * @param longitude global position * 1E7
 * @param altitude global position * 1000
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_gps_set_global_origin_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component, int32_t latitude, int32_t longitude, int32_t altitude)
{
	mavlink_gps_set_global_origin_t *p = (mavlink_gps_set_global_origin_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_GPS_SET_GLOBAL_ORIGIN;

	p->target_system = target_system;	// uint8_t:System ID
	p->target_component = target_component;	// uint8_t:Component ID
	p->latitude = latitude;	// int32_t:global position * 1E7
	p->longitude = longitude;	// int32_t:global position * 1E7
	p->altitude = altitude;	// int32_t:global position * 1000

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_GPS_SET_GLOBAL_ORIGIN_LEN);
}

/**
 * @brief Encode a gps_set_global_origin struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param gps_set_global_origin C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_gps_set_global_origin_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_gps_set_global_origin_t* gps_set_global_origin)
{
	return mavlink_msg_gps_set_global_origin_pack(system_id, component_id, msg, gps_set_global_origin->target_system, gps_set_global_origin->target_component, gps_set_global_origin->latitude, gps_set_global_origin->longitude, gps_set_global_origin->altitude);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a gps_set_global_origin message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param latitude global position * 1E7
 * @param longitude global position * 1E7
 * @param altitude global position * 1000
 */
static inline void mavlink_msg_gps_set_global_origin_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, int32_t latitude, int32_t longitude, int32_t altitude)
{
	mavlink_header_t hdr;
	mavlink_gps_set_global_origin_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_GPS_SET_GLOBAL_ORIGIN_LEN )
	payload.target_system = target_system;	// uint8_t:System ID
	payload.target_component = target_component;	// uint8_t:Component ID
	payload.latitude = latitude;	// int32_t:global position * 1E7
	payload.longitude = longitude;	// int32_t:global position * 1E7
	payload.altitude = altitude;	// int32_t:global position * 1000

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_GPS_SET_GLOBAL_ORIGIN_LEN;
	hdr.msgid = MAVLINK_MSG_ID_GPS_SET_GLOBAL_ORIGIN;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0x8E, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE GPS_SET_GLOBAL_ORIGIN UNPACKING

/**
 * @brief Get field target_system from gps_set_global_origin message
 *
 * @return System ID
 */
static inline uint8_t mavlink_msg_gps_set_global_origin_get_target_system(const mavlink_message_t* msg)
{
	mavlink_gps_set_global_origin_t *p = (mavlink_gps_set_global_origin_t *)&msg->payload[0];
	return (uint8_t)(p->target_system);
}

/**
 * @brief Get field target_component from gps_set_global_origin message
 *
 * @return Component ID
 */
static inline uint8_t mavlink_msg_gps_set_global_origin_get_target_component(const mavlink_message_t* msg)
{
	mavlink_gps_set_global_origin_t *p = (mavlink_gps_set_global_origin_t *)&msg->payload[0];
	return (uint8_t)(p->target_component);
}

/**
 * @brief Get field latitude from gps_set_global_origin message
 *
 * @return global position * 1E7
 */
static inline int32_t mavlink_msg_gps_set_global_origin_get_latitude(const mavlink_message_t* msg)
{
	mavlink_gps_set_global_origin_t *p = (mavlink_gps_set_global_origin_t *)&msg->payload[0];
	return (int32_t)(p->latitude);
}

/**
 * @brief Get field longitude from gps_set_global_origin message
 *
 * @return global position * 1E7
 */
static inline int32_t mavlink_msg_gps_set_global_origin_get_longitude(const mavlink_message_t* msg)
{
	mavlink_gps_set_global_origin_t *p = (mavlink_gps_set_global_origin_t *)&msg->payload[0];
	return (int32_t)(p->longitude);
}

/**
 * @brief Get field altitude from gps_set_global_origin message
 *
 * @return global position * 1000
 */
static inline int32_t mavlink_msg_gps_set_global_origin_get_altitude(const mavlink_message_t* msg)
{
	mavlink_gps_set_global_origin_t *p = (mavlink_gps_set_global_origin_t *)&msg->payload[0];
	return (int32_t)(p->altitude);
}

/**
 * @brief Decode a gps_set_global_origin message into a struct
 *
 * @param msg The message to decode
 * @param gps_set_global_origin C-struct to decode the message contents into
 */
static inline void mavlink_msg_gps_set_global_origin_decode(const mavlink_message_t* msg, mavlink_gps_set_global_origin_t* gps_set_global_origin)
{
	memcpy( gps_set_global_origin, msg->payload, sizeof(mavlink_gps_set_global_origin_t));
}
