// MESSAGE GLOBAL_POSITION_INT PACKING

#define MAVLINK_MSG_ID_GLOBAL_POSITION_INT 73
#define MAVLINK_MSG_ID_GLOBAL_POSITION_INT_LEN 20
#define MAVLINK_MSG_73_LEN 20
#define MAVLINK_MSG_ID_GLOBAL_POSITION_INT_KEY 0xD4
#define MAVLINK_MSG_73_KEY 0xD4

typedef struct __mavlink_global_position_int_t 
{
	int32_t lat;	///< Latitude, expressed as * 1E7
	int32_t lon;	///< Longitude, expressed as * 1E7
	int32_t alt;	///< Altitude in meters, expressed as * 1000 (millimeters), above MSL
	int16_t vx;	///< Ground X Speed (Latitude), expressed as m/s * 100
	int16_t vy;	///< Ground Y Speed (Longitude), expressed as m/s * 100
	int16_t vz;	///< Ground Z Speed (Altitude), expressed as m/s * 100
	uint16_t hdg;	///< Compass heading in degrees * 100, 0.0..359.99 degrees. If unknown, set to: 65535

} mavlink_global_position_int_t;

/**
 * @brief Pack a global_position_int message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param lat Latitude, expressed as * 1E7
 * @param lon Longitude, expressed as * 1E7
 * @param alt Altitude in meters, expressed as * 1000 (millimeters), above MSL
 * @param vx Ground X Speed (Latitude), expressed as m/s * 100
 * @param vy Ground Y Speed (Longitude), expressed as m/s * 100
 * @param vz Ground Z Speed (Altitude), expressed as m/s * 100
 * @param hdg Compass heading in degrees * 100, 0.0..359.99 degrees. If unknown, set to: 65535
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_global_position_int_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, int32_t lat, int32_t lon, int32_t alt, int16_t vx, int16_t vy, int16_t vz, uint16_t hdg)
{
	mavlink_global_position_int_t *p = (mavlink_global_position_int_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_GLOBAL_POSITION_INT;

	p->lat = lat;	// int32_t:Latitude, expressed as * 1E7
	p->lon = lon;	// int32_t:Longitude, expressed as * 1E7
	p->alt = alt;	// int32_t:Altitude in meters, expressed as * 1000 (millimeters), above MSL
	p->vx = vx;	// int16_t:Ground X Speed (Latitude), expressed as m/s * 100
	p->vy = vy;	// int16_t:Ground Y Speed (Longitude), expressed as m/s * 100
	p->vz = vz;	// int16_t:Ground Z Speed (Altitude), expressed as m/s * 100
	p->hdg = hdg;	// uint16_t:Compass heading in degrees * 100, 0.0..359.99 degrees. If unknown, set to: 65535

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_GLOBAL_POSITION_INT_LEN);
}

/**
 * @brief Pack a global_position_int message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param lat Latitude, expressed as * 1E7
 * @param lon Longitude, expressed as * 1E7
 * @param alt Altitude in meters, expressed as * 1000 (millimeters), above MSL
 * @param vx Ground X Speed (Latitude), expressed as m/s * 100
 * @param vy Ground Y Speed (Longitude), expressed as m/s * 100
 * @param vz Ground Z Speed (Altitude), expressed as m/s * 100
 * @param hdg Compass heading in degrees * 100, 0.0..359.99 degrees. If unknown, set to: 65535
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_global_position_int_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, int32_t lat, int32_t lon, int32_t alt, int16_t vx, int16_t vy, int16_t vz, uint16_t hdg)
{
	mavlink_global_position_int_t *p = (mavlink_global_position_int_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_GLOBAL_POSITION_INT;

	p->lat = lat;	// int32_t:Latitude, expressed as * 1E7
	p->lon = lon;	// int32_t:Longitude, expressed as * 1E7
	p->alt = alt;	// int32_t:Altitude in meters, expressed as * 1000 (millimeters), above MSL
	p->vx = vx;	// int16_t:Ground X Speed (Latitude), expressed as m/s * 100
	p->vy = vy;	// int16_t:Ground Y Speed (Longitude), expressed as m/s * 100
	p->vz = vz;	// int16_t:Ground Z Speed (Altitude), expressed as m/s * 100
	p->hdg = hdg;	// uint16_t:Compass heading in degrees * 100, 0.0..359.99 degrees. If unknown, set to: 65535

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_GLOBAL_POSITION_INT_LEN);
}

/**
 * @brief Encode a global_position_int struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param global_position_int C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_global_position_int_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_global_position_int_t* global_position_int)
{
	return mavlink_msg_global_position_int_pack(system_id, component_id, msg, global_position_int->lat, global_position_int->lon, global_position_int->alt, global_position_int->vx, global_position_int->vy, global_position_int->vz, global_position_int->hdg);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a global_position_int message
 * @param chan MAVLink channel to send the message
 *
 * @param lat Latitude, expressed as * 1E7
 * @param lon Longitude, expressed as * 1E7
 * @param alt Altitude in meters, expressed as * 1000 (millimeters), above MSL
 * @param vx Ground X Speed (Latitude), expressed as m/s * 100
 * @param vy Ground Y Speed (Longitude), expressed as m/s * 100
 * @param vz Ground Z Speed (Altitude), expressed as m/s * 100
 * @param hdg Compass heading in degrees * 100, 0.0..359.99 degrees. If unknown, set to: 65535
 */
static inline void mavlink_msg_global_position_int_send(mavlink_channel_t chan, int32_t lat, int32_t lon, int32_t alt, int16_t vx, int16_t vy, int16_t vz, uint16_t hdg)
{
	mavlink_header_t hdr;
	mavlink_global_position_int_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_GLOBAL_POSITION_INT_LEN )
	payload.lat = lat;	// int32_t:Latitude, expressed as * 1E7
	payload.lon = lon;	// int32_t:Longitude, expressed as * 1E7
	payload.alt = alt;	// int32_t:Altitude in meters, expressed as * 1000 (millimeters), above MSL
	payload.vx = vx;	// int16_t:Ground X Speed (Latitude), expressed as m/s * 100
	payload.vy = vy;	// int16_t:Ground Y Speed (Longitude), expressed as m/s * 100
	payload.vz = vz;	// int16_t:Ground Z Speed (Altitude), expressed as m/s * 100
	payload.hdg = hdg;	// uint16_t:Compass heading in degrees * 100, 0.0..359.99 degrees. If unknown, set to: 65535

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_GLOBAL_POSITION_INT_LEN;
	hdr.msgid = MAVLINK_MSG_ID_GLOBAL_POSITION_INT;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0xD4, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE GLOBAL_POSITION_INT UNPACKING

/**
 * @brief Get field lat from global_position_int message
 *
 * @return Latitude, expressed as * 1E7
 */
static inline int32_t mavlink_msg_global_position_int_get_lat(const mavlink_message_t* msg)
{
	mavlink_global_position_int_t *p = (mavlink_global_position_int_t *)&msg->payload[0];
	return (int32_t)(p->lat);
}

/**
 * @brief Get field lon from global_position_int message
 *
 * @return Longitude, expressed as * 1E7
 */
static inline int32_t mavlink_msg_global_position_int_get_lon(const mavlink_message_t* msg)
{
	mavlink_global_position_int_t *p = (mavlink_global_position_int_t *)&msg->payload[0];
	return (int32_t)(p->lon);
}

/**
 * @brief Get field alt from global_position_int message
 *
 * @return Altitude in meters, expressed as * 1000 (millimeters), above MSL
 */
static inline int32_t mavlink_msg_global_position_int_get_alt(const mavlink_message_t* msg)
{
	mavlink_global_position_int_t *p = (mavlink_global_position_int_t *)&msg->payload[0];
	return (int32_t)(p->alt);
}

/**
 * @brief Get field vx from global_position_int message
 *
 * @return Ground X Speed (Latitude), expressed as m/s * 100
 */
static inline int16_t mavlink_msg_global_position_int_get_vx(const mavlink_message_t* msg)
{
	mavlink_global_position_int_t *p = (mavlink_global_position_int_t *)&msg->payload[0];
	return (int16_t)(p->vx);
}

/**
 * @brief Get field vy from global_position_int message
 *
 * @return Ground Y Speed (Longitude), expressed as m/s * 100
 */
static inline int16_t mavlink_msg_global_position_int_get_vy(const mavlink_message_t* msg)
{
	mavlink_global_position_int_t *p = (mavlink_global_position_int_t *)&msg->payload[0];
	return (int16_t)(p->vy);
}

/**
 * @brief Get field vz from global_position_int message
 *
 * @return Ground Z Speed (Altitude), expressed as m/s * 100
 */
static inline int16_t mavlink_msg_global_position_int_get_vz(const mavlink_message_t* msg)
{
	mavlink_global_position_int_t *p = (mavlink_global_position_int_t *)&msg->payload[0];
	return (int16_t)(p->vz);
}

/**
 * @brief Get field hdg from global_position_int message
 *
 * @return Compass heading in degrees * 100, 0.0..359.99 degrees. If unknown, set to: 65535
 */
static inline uint16_t mavlink_msg_global_position_int_get_hdg(const mavlink_message_t* msg)
{
	mavlink_global_position_int_t *p = (mavlink_global_position_int_t *)&msg->payload[0];
	return (uint16_t)(p->hdg);
}

/**
 * @brief Decode a global_position_int message into a struct
 *
 * @param msg The message to decode
 * @param global_position_int C-struct to decode the message contents into
 */
static inline void mavlink_msg_global_position_int_decode(const mavlink_message_t* msg, mavlink_global_position_int_t* global_position_int)
{
	memcpy( global_position_int, msg->payload, sizeof(mavlink_global_position_int_t));
}
