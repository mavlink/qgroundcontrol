// MESSAGE VFR_HUD PACKING

#define MAVLINK_MSG_ID_VFR_HUD 74
#define MAVLINK_MSG_ID_VFR_HUD_LEN 20
#define MAVLINK_MSG_74_LEN 20
#define MAVLINK_MSG_ID_VFR_HUD_KEY 0xFB
#define MAVLINK_MSG_74_KEY 0xFB

typedef struct __mavlink_vfr_hud_t 
{
	float airspeed;	///< Current airspeed in m/s
	float groundspeed;	///< Current ground speed in m/s
	float alt;	///< Current altitude (MSL), in meters
	float climb;	///< Current climb rate in meters/second
	int16_t heading;	///< Current heading in degrees, in compass units (0..360, 0=north)
	uint16_t throttle;	///< Current throttle setting in integer percent, 0 to 100

} mavlink_vfr_hud_t;

/**
 * @brief Pack a vfr_hud message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param airspeed Current airspeed in m/s
 * @param groundspeed Current ground speed in m/s
 * @param heading Current heading in degrees, in compass units (0..360, 0=north)
 * @param throttle Current throttle setting in integer percent, 0 to 100
 * @param alt Current altitude (MSL), in meters
 * @param climb Current climb rate in meters/second
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_vfr_hud_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, float airspeed, float groundspeed, int16_t heading, uint16_t throttle, float alt, float climb)
{
	mavlink_vfr_hud_t *p = (mavlink_vfr_hud_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_VFR_HUD;

	p->airspeed = airspeed;	// float:Current airspeed in m/s
	p->groundspeed = groundspeed;	// float:Current ground speed in m/s
	p->heading = heading;	// int16_t:Current heading in degrees, in compass units (0..360, 0=north)
	p->throttle = throttle;	// uint16_t:Current throttle setting in integer percent, 0 to 100
	p->alt = alt;	// float:Current altitude (MSL), in meters
	p->climb = climb;	// float:Current climb rate in meters/second

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_VFR_HUD_LEN);
}

/**
 * @brief Pack a vfr_hud message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param airspeed Current airspeed in m/s
 * @param groundspeed Current ground speed in m/s
 * @param heading Current heading in degrees, in compass units (0..360, 0=north)
 * @param throttle Current throttle setting in integer percent, 0 to 100
 * @param alt Current altitude (MSL), in meters
 * @param climb Current climb rate in meters/second
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_vfr_hud_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, float airspeed, float groundspeed, int16_t heading, uint16_t throttle, float alt, float climb)
{
	mavlink_vfr_hud_t *p = (mavlink_vfr_hud_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_VFR_HUD;

	p->airspeed = airspeed;	// float:Current airspeed in m/s
	p->groundspeed = groundspeed;	// float:Current ground speed in m/s
	p->heading = heading;	// int16_t:Current heading in degrees, in compass units (0..360, 0=north)
	p->throttle = throttle;	// uint16_t:Current throttle setting in integer percent, 0 to 100
	p->alt = alt;	// float:Current altitude (MSL), in meters
	p->climb = climb;	// float:Current climb rate in meters/second

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_VFR_HUD_LEN);
}

/**
 * @brief Encode a vfr_hud struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param vfr_hud C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_vfr_hud_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_vfr_hud_t* vfr_hud)
{
	return mavlink_msg_vfr_hud_pack(system_id, component_id, msg, vfr_hud->airspeed, vfr_hud->groundspeed, vfr_hud->heading, vfr_hud->throttle, vfr_hud->alt, vfr_hud->climb);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a vfr_hud message
 * @param chan MAVLink channel to send the message
 *
 * @param airspeed Current airspeed in m/s
 * @param groundspeed Current ground speed in m/s
 * @param heading Current heading in degrees, in compass units (0..360, 0=north)
 * @param throttle Current throttle setting in integer percent, 0 to 100
 * @param alt Current altitude (MSL), in meters
 * @param climb Current climb rate in meters/second
 */
static inline void mavlink_msg_vfr_hud_send(mavlink_channel_t chan, float airspeed, float groundspeed, int16_t heading, uint16_t throttle, float alt, float climb)
{
	mavlink_header_t hdr;
	mavlink_vfr_hud_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_VFR_HUD_LEN )
	payload.airspeed = airspeed;	// float:Current airspeed in m/s
	payload.groundspeed = groundspeed;	// float:Current ground speed in m/s
	payload.heading = heading;	// int16_t:Current heading in degrees, in compass units (0..360, 0=north)
	payload.throttle = throttle;	// uint16_t:Current throttle setting in integer percent, 0 to 100
	payload.alt = alt;	// float:Current altitude (MSL), in meters
	payload.climb = climb;	// float:Current climb rate in meters/second

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_VFR_HUD_LEN;
	hdr.msgid = MAVLINK_MSG_ID_VFR_HUD;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0xFB, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE VFR_HUD UNPACKING

/**
 * @brief Get field airspeed from vfr_hud message
 *
 * @return Current airspeed in m/s
 */
static inline float mavlink_msg_vfr_hud_get_airspeed(const mavlink_message_t* msg)
{
	mavlink_vfr_hud_t *p = (mavlink_vfr_hud_t *)&msg->payload[0];
	return (float)(p->airspeed);
}

/**
 * @brief Get field groundspeed from vfr_hud message
 *
 * @return Current ground speed in m/s
 */
static inline float mavlink_msg_vfr_hud_get_groundspeed(const mavlink_message_t* msg)
{
	mavlink_vfr_hud_t *p = (mavlink_vfr_hud_t *)&msg->payload[0];
	return (float)(p->groundspeed);
}

/**
 * @brief Get field heading from vfr_hud message
 *
 * @return Current heading in degrees, in compass units (0..360, 0=north)
 */
static inline int16_t mavlink_msg_vfr_hud_get_heading(const mavlink_message_t* msg)
{
	mavlink_vfr_hud_t *p = (mavlink_vfr_hud_t *)&msg->payload[0];
	return (int16_t)(p->heading);
}

/**
 * @brief Get field throttle from vfr_hud message
 *
 * @return Current throttle setting in integer percent, 0 to 100
 */
static inline uint16_t mavlink_msg_vfr_hud_get_throttle(const mavlink_message_t* msg)
{
	mavlink_vfr_hud_t *p = (mavlink_vfr_hud_t *)&msg->payload[0];
	return (uint16_t)(p->throttle);
}

/**
 * @brief Get field alt from vfr_hud message
 *
 * @return Current altitude (MSL), in meters
 */
static inline float mavlink_msg_vfr_hud_get_alt(const mavlink_message_t* msg)
{
	mavlink_vfr_hud_t *p = (mavlink_vfr_hud_t *)&msg->payload[0];
	return (float)(p->alt);
}

/**
 * @brief Get field climb from vfr_hud message
 *
 * @return Current climb rate in meters/second
 */
static inline float mavlink_msg_vfr_hud_get_climb(const mavlink_message_t* msg)
{
	mavlink_vfr_hud_t *p = (mavlink_vfr_hud_t *)&msg->payload[0];
	return (float)(p->climb);
}

/**
 * @brief Decode a vfr_hud message into a struct
 *
 * @param msg The message to decode
 * @param vfr_hud C-struct to decode the message contents into
 */
static inline void mavlink_msg_vfr_hud_decode(const mavlink_message_t* msg, mavlink_vfr_hud_t* vfr_hud)
{
	memcpy( vfr_hud, msg->payload, sizeof(mavlink_vfr_hud_t));
}
