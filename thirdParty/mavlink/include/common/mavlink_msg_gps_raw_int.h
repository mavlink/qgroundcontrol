// MESSAGE GPS_RAW_INT PACKING

#define MAVLINK_MSG_ID_GPS_RAW_INT 25
#define MAVLINK_MSG_ID_GPS_RAW_INT_LEN 30
#define MAVLINK_MSG_25_LEN 30
#define MAVLINK_MSG_ID_GPS_RAW_INT_KEY 0xA6
#define MAVLINK_MSG_25_KEY 0xA6

typedef struct __mavlink_gps_raw_int_t 
{
	uint64_t usec;	///< Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	int32_t lat;	///< Latitude in 1E7 degrees
	int32_t lon;	///< Longitude in 1E7 degrees
	int32_t alt;	///< Altitude in 1E3 meters (millimeters) above MSL
	uint16_t eph;	///< GPS HDOP horizontal dilution of position in cm (m*100). If unknown, set to: 65535
	uint16_t epv;	///< GPS VDOP horizontal dilution of position in cm (m*100). If unknown, set to: 65535
	uint16_t vel;	///< GPS ground speed (m/s * 100). If unknown, set to: 65535
	uint16_t hdg;	///< Compass heading in degrees * 100, 0.0..359.99 degrees. If unknown, set to: 65535
	uint8_t fix_type;	///< 0-1: no fix, 2: 2D fix, 3: 3D fix. Some applications will not use the value of this field unless it is at least two, so always correctly fill in the fix.
	uint8_t satellites_visible;	///< Number of satellites visible. If unknown, set to 255

} mavlink_gps_raw_int_t;

/**
 * @brief Pack a gps_raw_int message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param usec Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 * @param fix_type 0-1: no fix, 2: 2D fix, 3: 3D fix. Some applications will not use the value of this field unless it is at least two, so always correctly fill in the fix.
 * @param lat Latitude in 1E7 degrees
 * @param lon Longitude in 1E7 degrees
 * @param alt Altitude in 1E3 meters (millimeters) above MSL
 * @param eph GPS HDOP horizontal dilution of position in cm (m*100). If unknown, set to: 65535
 * @param epv GPS VDOP horizontal dilution of position in cm (m*100). If unknown, set to: 65535
 * @param vel GPS ground speed (m/s * 100). If unknown, set to: 65535
 * @param hdg Compass heading in degrees * 100, 0.0..359.99 degrees. If unknown, set to: 65535
 * @param satellites_visible Number of satellites visible. If unknown, set to 255
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_gps_raw_int_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint64_t usec, uint8_t fix_type, int32_t lat, int32_t lon, int32_t alt, uint16_t eph, uint16_t epv, uint16_t vel, uint16_t hdg, uint8_t satellites_visible)
{
	mavlink_gps_raw_int_t *p = (mavlink_gps_raw_int_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_GPS_RAW_INT;

	p->usec = usec;	// uint64_t:Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	p->fix_type = fix_type;	// uint8_t:0-1: no fix, 2: 2D fix, 3: 3D fix. Some applications will not use the value of this field unless it is at least two, so always correctly fill in the fix.
	p->lat = lat;	// int32_t:Latitude in 1E7 degrees
	p->lon = lon;	// int32_t:Longitude in 1E7 degrees
	p->alt = alt;	// int32_t:Altitude in 1E3 meters (millimeters) above MSL
	p->eph = eph;	// uint16_t:GPS HDOP horizontal dilution of position in cm (m*100). If unknown, set to: 65535
	p->epv = epv;	// uint16_t:GPS VDOP horizontal dilution of position in cm (m*100). If unknown, set to: 65535
	p->vel = vel;	// uint16_t:GPS ground speed (m/s * 100). If unknown, set to: 65535
	p->hdg = hdg;	// uint16_t:Compass heading in degrees * 100, 0.0..359.99 degrees. If unknown, set to: 65535
	p->satellites_visible = satellites_visible;	// uint8_t:Number of satellites visible. If unknown, set to 255

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_GPS_RAW_INT_LEN);
}

/**
 * @brief Pack a gps_raw_int message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param usec Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 * @param fix_type 0-1: no fix, 2: 2D fix, 3: 3D fix. Some applications will not use the value of this field unless it is at least two, so always correctly fill in the fix.
 * @param lat Latitude in 1E7 degrees
 * @param lon Longitude in 1E7 degrees
 * @param alt Altitude in 1E3 meters (millimeters) above MSL
 * @param eph GPS HDOP horizontal dilution of position in cm (m*100). If unknown, set to: 65535
 * @param epv GPS VDOP horizontal dilution of position in cm (m*100). If unknown, set to: 65535
 * @param vel GPS ground speed (m/s * 100). If unknown, set to: 65535
 * @param hdg Compass heading in degrees * 100, 0.0..359.99 degrees. If unknown, set to: 65535
 * @param satellites_visible Number of satellites visible. If unknown, set to 255
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_gps_raw_int_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint64_t usec, uint8_t fix_type, int32_t lat, int32_t lon, int32_t alt, uint16_t eph, uint16_t epv, uint16_t vel, uint16_t hdg, uint8_t satellites_visible)
{
	mavlink_gps_raw_int_t *p = (mavlink_gps_raw_int_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_GPS_RAW_INT;

	p->usec = usec;	// uint64_t:Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	p->fix_type = fix_type;	// uint8_t:0-1: no fix, 2: 2D fix, 3: 3D fix. Some applications will not use the value of this field unless it is at least two, so always correctly fill in the fix.
	p->lat = lat;	// int32_t:Latitude in 1E7 degrees
	p->lon = lon;	// int32_t:Longitude in 1E7 degrees
	p->alt = alt;	// int32_t:Altitude in 1E3 meters (millimeters) above MSL
	p->eph = eph;	// uint16_t:GPS HDOP horizontal dilution of position in cm (m*100). If unknown, set to: 65535
	p->epv = epv;	// uint16_t:GPS VDOP horizontal dilution of position in cm (m*100). If unknown, set to: 65535
	p->vel = vel;	// uint16_t:GPS ground speed (m/s * 100). If unknown, set to: 65535
	p->hdg = hdg;	// uint16_t:Compass heading in degrees * 100, 0.0..359.99 degrees. If unknown, set to: 65535
	p->satellites_visible = satellites_visible;	// uint8_t:Number of satellites visible. If unknown, set to 255

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_GPS_RAW_INT_LEN);
}

/**
 * @brief Encode a gps_raw_int struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param gps_raw_int C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_gps_raw_int_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_gps_raw_int_t* gps_raw_int)
{
	return mavlink_msg_gps_raw_int_pack(system_id, component_id, msg, gps_raw_int->usec, gps_raw_int->fix_type, gps_raw_int->lat, gps_raw_int->lon, gps_raw_int->alt, gps_raw_int->eph, gps_raw_int->epv, gps_raw_int->vel, gps_raw_int->hdg, gps_raw_int->satellites_visible);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a gps_raw_int message
 * @param chan MAVLink channel to send the message
 *
 * @param usec Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 * @param fix_type 0-1: no fix, 2: 2D fix, 3: 3D fix. Some applications will not use the value of this field unless it is at least two, so always correctly fill in the fix.
 * @param lat Latitude in 1E7 degrees
 * @param lon Longitude in 1E7 degrees
 * @param alt Altitude in 1E3 meters (millimeters) above MSL
 * @param eph GPS HDOP horizontal dilution of position in cm (m*100). If unknown, set to: 65535
 * @param epv GPS VDOP horizontal dilution of position in cm (m*100). If unknown, set to: 65535
 * @param vel GPS ground speed (m/s * 100). If unknown, set to: 65535
 * @param hdg Compass heading in degrees * 100, 0.0..359.99 degrees. If unknown, set to: 65535
 * @param satellites_visible Number of satellites visible. If unknown, set to 255
 */
static inline void mavlink_msg_gps_raw_int_send(mavlink_channel_t chan, uint64_t usec, uint8_t fix_type, int32_t lat, int32_t lon, int32_t alt, uint16_t eph, uint16_t epv, uint16_t vel, uint16_t hdg, uint8_t satellites_visible)
{
	mavlink_header_t hdr;
	mavlink_gps_raw_int_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_GPS_RAW_INT_LEN )
	payload.usec = usec;	// uint64_t:Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	payload.fix_type = fix_type;	// uint8_t:0-1: no fix, 2: 2D fix, 3: 3D fix. Some applications will not use the value of this field unless it is at least two, so always correctly fill in the fix.
	payload.lat = lat;	// int32_t:Latitude in 1E7 degrees
	payload.lon = lon;	// int32_t:Longitude in 1E7 degrees
	payload.alt = alt;	// int32_t:Altitude in 1E3 meters (millimeters) above MSL
	payload.eph = eph;	// uint16_t:GPS HDOP horizontal dilution of position in cm (m*100). If unknown, set to: 65535
	payload.epv = epv;	// uint16_t:GPS VDOP horizontal dilution of position in cm (m*100). If unknown, set to: 65535
	payload.vel = vel;	// uint16_t:GPS ground speed (m/s * 100). If unknown, set to: 65535
	payload.hdg = hdg;	// uint16_t:Compass heading in degrees * 100, 0.0..359.99 degrees. If unknown, set to: 65535
	payload.satellites_visible = satellites_visible;	// uint8_t:Number of satellites visible. If unknown, set to 255

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_GPS_RAW_INT_LEN;
	hdr.msgid = MAVLINK_MSG_ID_GPS_RAW_INT;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0xA6, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE GPS_RAW_INT UNPACKING

/**
 * @brief Get field usec from gps_raw_int message
 *
 * @return Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 */
static inline uint64_t mavlink_msg_gps_raw_int_get_usec(const mavlink_message_t* msg)
{
	mavlink_gps_raw_int_t *p = (mavlink_gps_raw_int_t *)&msg->payload[0];
	return (uint64_t)(p->usec);
}

/**
 * @brief Get field fix_type from gps_raw_int message
 *
 * @return 0-1: no fix, 2: 2D fix, 3: 3D fix. Some applications will not use the value of this field unless it is at least two, so always correctly fill in the fix.
 */
static inline uint8_t mavlink_msg_gps_raw_int_get_fix_type(const mavlink_message_t* msg)
{
	mavlink_gps_raw_int_t *p = (mavlink_gps_raw_int_t *)&msg->payload[0];
	return (uint8_t)(p->fix_type);
}

/**
 * @brief Get field lat from gps_raw_int message
 *
 * @return Latitude in 1E7 degrees
 */
static inline int32_t mavlink_msg_gps_raw_int_get_lat(const mavlink_message_t* msg)
{
	mavlink_gps_raw_int_t *p = (mavlink_gps_raw_int_t *)&msg->payload[0];
	return (int32_t)(p->lat);
}

/**
 * @brief Get field lon from gps_raw_int message
 *
 * @return Longitude in 1E7 degrees
 */
static inline int32_t mavlink_msg_gps_raw_int_get_lon(const mavlink_message_t* msg)
{
	mavlink_gps_raw_int_t *p = (mavlink_gps_raw_int_t *)&msg->payload[0];
	return (int32_t)(p->lon);
}

/**
 * @brief Get field alt from gps_raw_int message
 *
 * @return Altitude in 1E3 meters (millimeters) above MSL
 */
static inline int32_t mavlink_msg_gps_raw_int_get_alt(const mavlink_message_t* msg)
{
	mavlink_gps_raw_int_t *p = (mavlink_gps_raw_int_t *)&msg->payload[0];
	return (int32_t)(p->alt);
}

/**
 * @brief Get field eph from gps_raw_int message
 *
 * @return GPS HDOP horizontal dilution of position in cm (m*100). If unknown, set to: 65535
 */
static inline uint16_t mavlink_msg_gps_raw_int_get_eph(const mavlink_message_t* msg)
{
	mavlink_gps_raw_int_t *p = (mavlink_gps_raw_int_t *)&msg->payload[0];
	return (uint16_t)(p->eph);
}

/**
 * @brief Get field epv from gps_raw_int message
 *
 * @return GPS VDOP horizontal dilution of position in cm (m*100). If unknown, set to: 65535
 */
static inline uint16_t mavlink_msg_gps_raw_int_get_epv(const mavlink_message_t* msg)
{
	mavlink_gps_raw_int_t *p = (mavlink_gps_raw_int_t *)&msg->payload[0];
	return (uint16_t)(p->epv);
}

/**
 * @brief Get field vel from gps_raw_int message
 *
 * @return GPS ground speed (m/s * 100). If unknown, set to: 65535
 */
static inline uint16_t mavlink_msg_gps_raw_int_get_vel(const mavlink_message_t* msg)
{
	mavlink_gps_raw_int_t *p = (mavlink_gps_raw_int_t *)&msg->payload[0];
	return (uint16_t)(p->vel);
}

/**
 * @brief Get field hdg from gps_raw_int message
 *
 * @return Compass heading in degrees * 100, 0.0..359.99 degrees. If unknown, set to: 65535
 */
static inline uint16_t mavlink_msg_gps_raw_int_get_hdg(const mavlink_message_t* msg)
{
	mavlink_gps_raw_int_t *p = (mavlink_gps_raw_int_t *)&msg->payload[0];
	return (uint16_t)(p->hdg);
}

/**
 * @brief Get field satellites_visible from gps_raw_int message
 *
 * @return Number of satellites visible. If unknown, set to 255
 */
static inline uint8_t mavlink_msg_gps_raw_int_get_satellites_visible(const mavlink_message_t* msg)
{
	mavlink_gps_raw_int_t *p = (mavlink_gps_raw_int_t *)&msg->payload[0];
	return (uint8_t)(p->satellites_visible);
}

/**
 * @brief Decode a gps_raw_int message into a struct
 *
 * @param msg The message to decode
 * @param gps_raw_int C-struct to decode the message contents into
 */
static inline void mavlink_msg_gps_raw_int_decode(const mavlink_message_t* msg, mavlink_gps_raw_int_t* gps_raw_int)
{
	memcpy( gps_raw_int, msg->payload, sizeof(mavlink_gps_raw_int_t));
}
