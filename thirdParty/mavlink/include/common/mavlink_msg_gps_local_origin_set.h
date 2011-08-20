// MESSAGE GPS_LOCAL_ORIGIN_SET PACKING

#define MAVLINK_MSG_ID_GPS_LOCAL_ORIGIN_SET 49
#define MAVLINK_MSG_ID_GPS_LOCAL_ORIGIN_SET_LEN 12
#define MAVLINK_MSG_49_LEN 12
#define MAVLINK_MSG_ID_GPS_LOCAL_ORIGIN_SET_KEY 0x3C
#define MAVLINK_MSG_49_KEY 0x3C

typedef struct __mavlink_gps_local_origin_set_t 
{
	int32_t latitude;	///< Latitude (WGS84), expressed as * 1E7
	int32_t longitude;	///< Longitude (WGS84), expressed as * 1E7
	int32_t altitude;	///< Altitude(WGS84), expressed as * 1000

} mavlink_gps_local_origin_set_t;

/**
 * @brief Pack a gps_local_origin_set message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param latitude Latitude (WGS84), expressed as * 1E7
 * @param longitude Longitude (WGS84), expressed as * 1E7
 * @param altitude Altitude(WGS84), expressed as * 1000
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_gps_local_origin_set_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, int32_t latitude, int32_t longitude, int32_t altitude)
{
	mavlink_gps_local_origin_set_t *p = (mavlink_gps_local_origin_set_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_GPS_LOCAL_ORIGIN_SET;

	p->latitude = latitude;	// int32_t:Latitude (WGS84), expressed as * 1E7
	p->longitude = longitude;	// int32_t:Longitude (WGS84), expressed as * 1E7
	p->altitude = altitude;	// int32_t:Altitude(WGS84), expressed as * 1000

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_GPS_LOCAL_ORIGIN_SET_LEN);
}

/**
 * @brief Pack a gps_local_origin_set message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param latitude Latitude (WGS84), expressed as * 1E7
 * @param longitude Longitude (WGS84), expressed as * 1E7
 * @param altitude Altitude(WGS84), expressed as * 1000
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_gps_local_origin_set_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, int32_t latitude, int32_t longitude, int32_t altitude)
{
	mavlink_gps_local_origin_set_t *p = (mavlink_gps_local_origin_set_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_GPS_LOCAL_ORIGIN_SET;

	p->latitude = latitude;	// int32_t:Latitude (WGS84), expressed as * 1E7
	p->longitude = longitude;	// int32_t:Longitude (WGS84), expressed as * 1E7
	p->altitude = altitude;	// int32_t:Altitude(WGS84), expressed as * 1000

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_GPS_LOCAL_ORIGIN_SET_LEN);
}

/**
 * @brief Encode a gps_local_origin_set struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param gps_local_origin_set C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_gps_local_origin_set_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_gps_local_origin_set_t* gps_local_origin_set)
{
	return mavlink_msg_gps_local_origin_set_pack(system_id, component_id, msg, gps_local_origin_set->latitude, gps_local_origin_set->longitude, gps_local_origin_set->altitude);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a gps_local_origin_set message
 * @param chan MAVLink channel to send the message
 *
 * @param latitude Latitude (WGS84), expressed as * 1E7
 * @param longitude Longitude (WGS84), expressed as * 1E7
 * @param altitude Altitude(WGS84), expressed as * 1000
 */
static inline void mavlink_msg_gps_local_origin_set_send(mavlink_channel_t chan, int32_t latitude, int32_t longitude, int32_t altitude)
{
	mavlink_header_t hdr;
	mavlink_gps_local_origin_set_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_GPS_LOCAL_ORIGIN_SET_LEN )
	payload.latitude = latitude;	// int32_t:Latitude (WGS84), expressed as * 1E7
	payload.longitude = longitude;	// int32_t:Longitude (WGS84), expressed as * 1E7
	payload.altitude = altitude;	// int32_t:Altitude(WGS84), expressed as * 1000

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_GPS_LOCAL_ORIGIN_SET_LEN;
	hdr.msgid = MAVLINK_MSG_ID_GPS_LOCAL_ORIGIN_SET;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0x3C, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE GPS_LOCAL_ORIGIN_SET UNPACKING

/**
 * @brief Get field latitude from gps_local_origin_set message
 *
 * @return Latitude (WGS84), expressed as * 1E7
 */
static inline int32_t mavlink_msg_gps_local_origin_set_get_latitude(const mavlink_message_t* msg)
{
	mavlink_gps_local_origin_set_t *p = (mavlink_gps_local_origin_set_t *)&msg->payload[0];
	return (int32_t)(p->latitude);
}

/**
 * @brief Get field longitude from gps_local_origin_set message
 *
 * @return Longitude (WGS84), expressed as * 1E7
 */
static inline int32_t mavlink_msg_gps_local_origin_set_get_longitude(const mavlink_message_t* msg)
{
	mavlink_gps_local_origin_set_t *p = (mavlink_gps_local_origin_set_t *)&msg->payload[0];
	return (int32_t)(p->longitude);
}

/**
 * @brief Get field altitude from gps_local_origin_set message
 *
 * @return Altitude(WGS84), expressed as * 1000
 */
static inline int32_t mavlink_msg_gps_local_origin_set_get_altitude(const mavlink_message_t* msg)
{
	mavlink_gps_local_origin_set_t *p = (mavlink_gps_local_origin_set_t *)&msg->payload[0];
	return (int32_t)(p->altitude);
}

/**
 * @brief Decode a gps_local_origin_set message into a struct
 *
 * @param msg The message to decode
 * @param gps_local_origin_set C-struct to decode the message contents into
 */
static inline void mavlink_msg_gps_local_origin_set_decode(const mavlink_message_t* msg, mavlink_gps_local_origin_set_t* gps_local_origin_set)
{
	memcpy( gps_local_origin_set, msg->payload, sizeof(mavlink_gps_local_origin_set_t));
}
