// MESSAGE GPS_STATUS PACKING

#define MAVLINK_MSG_ID_GPS_STATUS 27
#define MAVLINK_MSG_ID_GPS_STATUS_LEN 101
#define MAVLINK_MSG_27_LEN 101
#define MAVLINK_MSG_ID_GPS_STATUS_KEY 0x63
#define MAVLINK_MSG_27_KEY 0x63

typedef struct __mavlink_gps_status_t 
{
	uint8_t satellites_visible;	///< Number of satellites visible
	uint8_t satellite_prn[20];	///< Global satellite ID
	uint8_t satellite_used[20];	///< 0: Satellite not used, 1: used for localization
	uint8_t satellite_elevation[20];	///< Elevation (0: right on top of receiver, 90: on the horizon) of satellite
	uint8_t satellite_azimuth[20];	///< Direction of satellite, 0: 0 deg, 255: 360 deg.
	uint8_t satellite_snr[20];	///< Signal to noise ratio of satellite

} mavlink_gps_status_t;
#define MAVLINK_MSG_GPS_STATUS_FIELD_SATELLITE_PRN_LEN 20
#define MAVLINK_MSG_GPS_STATUS_FIELD_SATELLITE_USED_LEN 20
#define MAVLINK_MSG_GPS_STATUS_FIELD_SATELLITE_ELEVATION_LEN 20
#define MAVLINK_MSG_GPS_STATUS_FIELD_SATELLITE_AZIMUTH_LEN 20
#define MAVLINK_MSG_GPS_STATUS_FIELD_SATELLITE_SNR_LEN 20

/**
 * @brief Pack a gps_status message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param satellites_visible Number of satellites visible
 * @param satellite_prn Global satellite ID
 * @param satellite_used 0: Satellite not used, 1: used for localization
 * @param satellite_elevation Elevation (0: right on top of receiver, 90: on the horizon) of satellite
 * @param satellite_azimuth Direction of satellite, 0: 0 deg, 255: 360 deg.
 * @param satellite_snr Signal to noise ratio of satellite
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_gps_status_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t satellites_visible, const uint8_t* satellite_prn, const uint8_t* satellite_used, const uint8_t* satellite_elevation, const uint8_t* satellite_azimuth, const uint8_t* satellite_snr)
{
	mavlink_gps_status_t *p = (mavlink_gps_status_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_GPS_STATUS;

	p->satellites_visible = satellites_visible;	// uint8_t:Number of satellites visible
	memcpy(p->satellite_prn, satellite_prn, sizeof(p->satellite_prn));	// uint8_t[20]:Global satellite ID
	memcpy(p->satellite_used, satellite_used, sizeof(p->satellite_used));	// uint8_t[20]:0: Satellite not used, 1: used for localization
	memcpy(p->satellite_elevation, satellite_elevation, sizeof(p->satellite_elevation));	// uint8_t[20]:Elevation (0: right on top of receiver, 90: on the horizon) of satellite
	memcpy(p->satellite_azimuth, satellite_azimuth, sizeof(p->satellite_azimuth));	// uint8_t[20]:Direction of satellite, 0: 0 deg, 255: 360 deg.
	memcpy(p->satellite_snr, satellite_snr, sizeof(p->satellite_snr));	// uint8_t[20]:Signal to noise ratio of satellite

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_GPS_STATUS_LEN);
}

/**
 * @brief Pack a gps_status message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param satellites_visible Number of satellites visible
 * @param satellite_prn Global satellite ID
 * @param satellite_used 0: Satellite not used, 1: used for localization
 * @param satellite_elevation Elevation (0: right on top of receiver, 90: on the horizon) of satellite
 * @param satellite_azimuth Direction of satellite, 0: 0 deg, 255: 360 deg.
 * @param satellite_snr Signal to noise ratio of satellite
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_gps_status_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t satellites_visible, const uint8_t* satellite_prn, const uint8_t* satellite_used, const uint8_t* satellite_elevation, const uint8_t* satellite_azimuth, const uint8_t* satellite_snr)
{
	mavlink_gps_status_t *p = (mavlink_gps_status_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_GPS_STATUS;

	p->satellites_visible = satellites_visible;	// uint8_t:Number of satellites visible
	memcpy(p->satellite_prn, satellite_prn, sizeof(p->satellite_prn));	// uint8_t[20]:Global satellite ID
	memcpy(p->satellite_used, satellite_used, sizeof(p->satellite_used));	// uint8_t[20]:0: Satellite not used, 1: used for localization
	memcpy(p->satellite_elevation, satellite_elevation, sizeof(p->satellite_elevation));	// uint8_t[20]:Elevation (0: right on top of receiver, 90: on the horizon) of satellite
	memcpy(p->satellite_azimuth, satellite_azimuth, sizeof(p->satellite_azimuth));	// uint8_t[20]:Direction of satellite, 0: 0 deg, 255: 360 deg.
	memcpy(p->satellite_snr, satellite_snr, sizeof(p->satellite_snr));	// uint8_t[20]:Signal to noise ratio of satellite

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_GPS_STATUS_LEN);
}

/**
 * @brief Encode a gps_status struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param gps_status C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_gps_status_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_gps_status_t* gps_status)
{
	return mavlink_msg_gps_status_pack(system_id, component_id, msg, gps_status->satellites_visible, gps_status->satellite_prn, gps_status->satellite_used, gps_status->satellite_elevation, gps_status->satellite_azimuth, gps_status->satellite_snr);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a gps_status message
 * @param chan MAVLink channel to send the message
 *
 * @param satellites_visible Number of satellites visible
 * @param satellite_prn Global satellite ID
 * @param satellite_used 0: Satellite not used, 1: used for localization
 * @param satellite_elevation Elevation (0: right on top of receiver, 90: on the horizon) of satellite
 * @param satellite_azimuth Direction of satellite, 0: 0 deg, 255: 360 deg.
 * @param satellite_snr Signal to noise ratio of satellite
 */
static inline void mavlink_msg_gps_status_send(mavlink_channel_t chan, uint8_t satellites_visible, const uint8_t* satellite_prn, const uint8_t* satellite_used, const uint8_t* satellite_elevation, const uint8_t* satellite_azimuth, const uint8_t* satellite_snr)
{
	mavlink_header_t hdr;
	mavlink_gps_status_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_GPS_STATUS_LEN )
	payload.satellites_visible = satellites_visible;	// uint8_t:Number of satellites visible
	memcpy(payload.satellite_prn, satellite_prn, sizeof(payload.satellite_prn));	// uint8_t[20]:Global satellite ID
	memcpy(payload.satellite_used, satellite_used, sizeof(payload.satellite_used));	// uint8_t[20]:0: Satellite not used, 1: used for localization
	memcpy(payload.satellite_elevation, satellite_elevation, sizeof(payload.satellite_elevation));	// uint8_t[20]:Elevation (0: right on top of receiver, 90: on the horizon) of satellite
	memcpy(payload.satellite_azimuth, satellite_azimuth, sizeof(payload.satellite_azimuth));	// uint8_t[20]:Direction of satellite, 0: 0 deg, 255: 360 deg.
	memcpy(payload.satellite_snr, satellite_snr, sizeof(payload.satellite_snr));	// uint8_t[20]:Signal to noise ratio of satellite

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_GPS_STATUS_LEN;
	hdr.msgid = MAVLINK_MSG_ID_GPS_STATUS;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0x63, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE GPS_STATUS UNPACKING

/**
 * @brief Get field satellites_visible from gps_status message
 *
 * @return Number of satellites visible
 */
static inline uint8_t mavlink_msg_gps_status_get_satellites_visible(const mavlink_message_t* msg)
{
	mavlink_gps_status_t *p = (mavlink_gps_status_t *)&msg->payload[0];
	return (uint8_t)(p->satellites_visible);
}

/**
 * @brief Get field satellite_prn from gps_status message
 *
 * @return Global satellite ID
 */
static inline uint16_t mavlink_msg_gps_status_get_satellite_prn(const mavlink_message_t* msg, uint8_t* satellite_prn)
{
	mavlink_gps_status_t *p = (mavlink_gps_status_t *)&msg->payload[0];

	memcpy(satellite_prn, p->satellite_prn, sizeof(p->satellite_prn));
	return sizeof(p->satellite_prn);
}

/**
 * @brief Get field satellite_used from gps_status message
 *
 * @return 0: Satellite not used, 1: used for localization
 */
static inline uint16_t mavlink_msg_gps_status_get_satellite_used(const mavlink_message_t* msg, uint8_t* satellite_used)
{
	mavlink_gps_status_t *p = (mavlink_gps_status_t *)&msg->payload[0];

	memcpy(satellite_used, p->satellite_used, sizeof(p->satellite_used));
	return sizeof(p->satellite_used);
}

/**
 * @brief Get field satellite_elevation from gps_status message
 *
 * @return Elevation (0: right on top of receiver, 90: on the horizon) of satellite
 */
static inline uint16_t mavlink_msg_gps_status_get_satellite_elevation(const mavlink_message_t* msg, uint8_t* satellite_elevation)
{
	mavlink_gps_status_t *p = (mavlink_gps_status_t *)&msg->payload[0];

	memcpy(satellite_elevation, p->satellite_elevation, sizeof(p->satellite_elevation));
	return sizeof(p->satellite_elevation);
}

/**
 * @brief Get field satellite_azimuth from gps_status message
 *
 * @return Direction of satellite, 0: 0 deg, 255: 360 deg.
 */
static inline uint16_t mavlink_msg_gps_status_get_satellite_azimuth(const mavlink_message_t* msg, uint8_t* satellite_azimuth)
{
	mavlink_gps_status_t *p = (mavlink_gps_status_t *)&msg->payload[0];

	memcpy(satellite_azimuth, p->satellite_azimuth, sizeof(p->satellite_azimuth));
	return sizeof(p->satellite_azimuth);
}

/**
 * @brief Get field satellite_snr from gps_status message
 *
 * @return Signal to noise ratio of satellite
 */
static inline uint16_t mavlink_msg_gps_status_get_satellite_snr(const mavlink_message_t* msg, uint8_t* satellite_snr)
{
	mavlink_gps_status_t *p = (mavlink_gps_status_t *)&msg->payload[0];

	memcpy(satellite_snr, p->satellite_snr, sizeof(p->satellite_snr));
	return sizeof(p->satellite_snr);
}

/**
 * @brief Decode a gps_status message into a struct
 *
 * @param msg The message to decode
 * @param gps_status C-struct to decode the message contents into
 */
static inline void mavlink_msg_gps_status_decode(const mavlink_message_t* msg, mavlink_gps_status_t* gps_status)
{
	memcpy( gps_status, msg->payload, sizeof(mavlink_gps_status_t));
}
