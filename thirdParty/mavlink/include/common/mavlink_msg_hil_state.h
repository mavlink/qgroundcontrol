// MESSAGE HIL_STATE PACKING

#define MAVLINK_MSG_ID_HIL_STATE 90
#define MAVLINK_MSG_ID_HIL_STATE_LEN 56
#define MAVLINK_MSG_90_LEN 56
#define MAVLINK_MSG_ID_HIL_STATE_KEY 0x12
#define MAVLINK_MSG_90_KEY 0x12

typedef struct __mavlink_hil_state_t 
{
	uint64_t time_us;	///< Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	float roll;	///< Roll angle (rad)
	float pitch;	///< Pitch angle (rad)
	float yaw;	///< Yaw angle (rad)
	float rollspeed;	///< Roll angular speed (rad/s)
	float pitchspeed;	///< Pitch angular speed (rad/s)
	float yawspeed;	///< Yaw angular speed (rad/s)
	int32_t lat;	///< Latitude, expressed as * 1E7
	int32_t lon;	///< Longitude, expressed as * 1E7
	int32_t alt;	///< Altitude in meters, expressed as * 1000 (millimeters)
	int16_t vx;	///< Ground X Speed (Latitude), expressed as m/s * 100
	int16_t vy;	///< Ground Y Speed (Longitude), expressed as m/s * 100
	int16_t vz;	///< Ground Z Speed (Altitude), expressed as m/s * 100
	int16_t xacc;	///< X acceleration (mg)
	int16_t yacc;	///< Y acceleration (mg)
	int16_t zacc;	///< Z acceleration (mg)

} mavlink_hil_state_t;

/**
 * @brief Pack a hil_state message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_us Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 * @param roll Roll angle (rad)
 * @param pitch Pitch angle (rad)
 * @param yaw Yaw angle (rad)
 * @param rollspeed Roll angular speed (rad/s)
 * @param pitchspeed Pitch angular speed (rad/s)
 * @param yawspeed Yaw angular speed (rad/s)
 * @param lat Latitude, expressed as * 1E7
 * @param lon Longitude, expressed as * 1E7
 * @param alt Altitude in meters, expressed as * 1000 (millimeters)
 * @param vx Ground X Speed (Latitude), expressed as m/s * 100
 * @param vy Ground Y Speed (Longitude), expressed as m/s * 100
 * @param vz Ground Z Speed (Altitude), expressed as m/s * 100
 * @param xacc X acceleration (mg)
 * @param yacc Y acceleration (mg)
 * @param zacc Z acceleration (mg)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_hil_state_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint64_t time_us, float roll, float pitch, float yaw, float rollspeed, float pitchspeed, float yawspeed, int32_t lat, int32_t lon, int32_t alt, int16_t vx, int16_t vy, int16_t vz, int16_t xacc, int16_t yacc, int16_t zacc)
{
	mavlink_hil_state_t *p = (mavlink_hil_state_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_HIL_STATE;

	p->time_us = time_us;	// uint64_t:Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	p->roll = roll;	// float:Roll angle (rad)
	p->pitch = pitch;	// float:Pitch angle (rad)
	p->yaw = yaw;	// float:Yaw angle (rad)
	p->rollspeed = rollspeed;	// float:Roll angular speed (rad/s)
	p->pitchspeed = pitchspeed;	// float:Pitch angular speed (rad/s)
	p->yawspeed = yawspeed;	// float:Yaw angular speed (rad/s)
	p->lat = lat;	// int32_t:Latitude, expressed as * 1E7
	p->lon = lon;	// int32_t:Longitude, expressed as * 1E7
	p->alt = alt;	// int32_t:Altitude in meters, expressed as * 1000 (millimeters)
	p->vx = vx;	// int16_t:Ground X Speed (Latitude), expressed as m/s * 100
	p->vy = vy;	// int16_t:Ground Y Speed (Longitude), expressed as m/s * 100
	p->vz = vz;	// int16_t:Ground Z Speed (Altitude), expressed as m/s * 100
	p->xacc = xacc;	// int16_t:X acceleration (mg)
	p->yacc = yacc;	// int16_t:Y acceleration (mg)
	p->zacc = zacc;	// int16_t:Z acceleration (mg)

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_HIL_STATE_LEN);
}

/**
 * @brief Pack a hil_state message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param time_us Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 * @param roll Roll angle (rad)
 * @param pitch Pitch angle (rad)
 * @param yaw Yaw angle (rad)
 * @param rollspeed Roll angular speed (rad/s)
 * @param pitchspeed Pitch angular speed (rad/s)
 * @param yawspeed Yaw angular speed (rad/s)
 * @param lat Latitude, expressed as * 1E7
 * @param lon Longitude, expressed as * 1E7
 * @param alt Altitude in meters, expressed as * 1000 (millimeters)
 * @param vx Ground X Speed (Latitude), expressed as m/s * 100
 * @param vy Ground Y Speed (Longitude), expressed as m/s * 100
 * @param vz Ground Z Speed (Altitude), expressed as m/s * 100
 * @param xacc X acceleration (mg)
 * @param yacc Y acceleration (mg)
 * @param zacc Z acceleration (mg)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_hil_state_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint64_t time_us, float roll, float pitch, float yaw, float rollspeed, float pitchspeed, float yawspeed, int32_t lat, int32_t lon, int32_t alt, int16_t vx, int16_t vy, int16_t vz, int16_t xacc, int16_t yacc, int16_t zacc)
{
	mavlink_hil_state_t *p = (mavlink_hil_state_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_HIL_STATE;

	p->time_us = time_us;	// uint64_t:Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	p->roll = roll;	// float:Roll angle (rad)
	p->pitch = pitch;	// float:Pitch angle (rad)
	p->yaw = yaw;	// float:Yaw angle (rad)
	p->rollspeed = rollspeed;	// float:Roll angular speed (rad/s)
	p->pitchspeed = pitchspeed;	// float:Pitch angular speed (rad/s)
	p->yawspeed = yawspeed;	// float:Yaw angular speed (rad/s)
	p->lat = lat;	// int32_t:Latitude, expressed as * 1E7
	p->lon = lon;	// int32_t:Longitude, expressed as * 1E7
	p->alt = alt;	// int32_t:Altitude in meters, expressed as * 1000 (millimeters)
	p->vx = vx;	// int16_t:Ground X Speed (Latitude), expressed as m/s * 100
	p->vy = vy;	// int16_t:Ground Y Speed (Longitude), expressed as m/s * 100
	p->vz = vz;	// int16_t:Ground Z Speed (Altitude), expressed as m/s * 100
	p->xacc = xacc;	// int16_t:X acceleration (mg)
	p->yacc = yacc;	// int16_t:Y acceleration (mg)
	p->zacc = zacc;	// int16_t:Z acceleration (mg)

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_HIL_STATE_LEN);
}

/**
 * @brief Encode a hil_state struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param hil_state C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_hil_state_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_hil_state_t* hil_state)
{
	return mavlink_msg_hil_state_pack(system_id, component_id, msg, hil_state->time_us, hil_state->roll, hil_state->pitch, hil_state->yaw, hil_state->rollspeed, hil_state->pitchspeed, hil_state->yawspeed, hil_state->lat, hil_state->lon, hil_state->alt, hil_state->vx, hil_state->vy, hil_state->vz, hil_state->xacc, hil_state->yacc, hil_state->zacc);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a hil_state message
 * @param chan MAVLink channel to send the message
 *
 * @param time_us Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 * @param roll Roll angle (rad)
 * @param pitch Pitch angle (rad)
 * @param yaw Yaw angle (rad)
 * @param rollspeed Roll angular speed (rad/s)
 * @param pitchspeed Pitch angular speed (rad/s)
 * @param yawspeed Yaw angular speed (rad/s)
 * @param lat Latitude, expressed as * 1E7
 * @param lon Longitude, expressed as * 1E7
 * @param alt Altitude in meters, expressed as * 1000 (millimeters)
 * @param vx Ground X Speed (Latitude), expressed as m/s * 100
 * @param vy Ground Y Speed (Longitude), expressed as m/s * 100
 * @param vz Ground Z Speed (Altitude), expressed as m/s * 100
 * @param xacc X acceleration (mg)
 * @param yacc Y acceleration (mg)
 * @param zacc Z acceleration (mg)
 */
static inline void mavlink_msg_hil_state_send(mavlink_channel_t chan, uint64_t time_us, float roll, float pitch, float yaw, float rollspeed, float pitchspeed, float yawspeed, int32_t lat, int32_t lon, int32_t alt, int16_t vx, int16_t vy, int16_t vz, int16_t xacc, int16_t yacc, int16_t zacc)
{
	mavlink_header_t hdr;
	mavlink_hil_state_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_HIL_STATE_LEN )
	payload.time_us = time_us;	// uint64_t:Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	payload.roll = roll;	// float:Roll angle (rad)
	payload.pitch = pitch;	// float:Pitch angle (rad)
	payload.yaw = yaw;	// float:Yaw angle (rad)
	payload.rollspeed = rollspeed;	// float:Roll angular speed (rad/s)
	payload.pitchspeed = pitchspeed;	// float:Pitch angular speed (rad/s)
	payload.yawspeed = yawspeed;	// float:Yaw angular speed (rad/s)
	payload.lat = lat;	// int32_t:Latitude, expressed as * 1E7
	payload.lon = lon;	// int32_t:Longitude, expressed as * 1E7
	payload.alt = alt;	// int32_t:Altitude in meters, expressed as * 1000 (millimeters)
	payload.vx = vx;	// int16_t:Ground X Speed (Latitude), expressed as m/s * 100
	payload.vy = vy;	// int16_t:Ground Y Speed (Longitude), expressed as m/s * 100
	payload.vz = vz;	// int16_t:Ground Z Speed (Altitude), expressed as m/s * 100
	payload.xacc = xacc;	// int16_t:X acceleration (mg)
	payload.yacc = yacc;	// int16_t:Y acceleration (mg)
	payload.zacc = zacc;	// int16_t:Z acceleration (mg)

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_HIL_STATE_LEN;
	hdr.msgid = MAVLINK_MSG_ID_HIL_STATE;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0x12, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE HIL_STATE UNPACKING

/**
 * @brief Get field time_us from hil_state message
 *
 * @return Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 */
static inline uint64_t mavlink_msg_hil_state_get_time_us(const mavlink_message_t* msg)
{
	mavlink_hil_state_t *p = (mavlink_hil_state_t *)&msg->payload[0];
	return (uint64_t)(p->time_us);
}

/**
 * @brief Get field roll from hil_state message
 *
 * @return Roll angle (rad)
 */
static inline float mavlink_msg_hil_state_get_roll(const mavlink_message_t* msg)
{
	mavlink_hil_state_t *p = (mavlink_hil_state_t *)&msg->payload[0];
	return (float)(p->roll);
}

/**
 * @brief Get field pitch from hil_state message
 *
 * @return Pitch angle (rad)
 */
static inline float mavlink_msg_hil_state_get_pitch(const mavlink_message_t* msg)
{
	mavlink_hil_state_t *p = (mavlink_hil_state_t *)&msg->payload[0];
	return (float)(p->pitch);
}

/**
 * @brief Get field yaw from hil_state message
 *
 * @return Yaw angle (rad)
 */
static inline float mavlink_msg_hil_state_get_yaw(const mavlink_message_t* msg)
{
	mavlink_hil_state_t *p = (mavlink_hil_state_t *)&msg->payload[0];
	return (float)(p->yaw);
}

/**
 * @brief Get field rollspeed from hil_state message
 *
 * @return Roll angular speed (rad/s)
 */
static inline float mavlink_msg_hil_state_get_rollspeed(const mavlink_message_t* msg)
{
	mavlink_hil_state_t *p = (mavlink_hil_state_t *)&msg->payload[0];
	return (float)(p->rollspeed);
}

/**
 * @brief Get field pitchspeed from hil_state message
 *
 * @return Pitch angular speed (rad/s)
 */
static inline float mavlink_msg_hil_state_get_pitchspeed(const mavlink_message_t* msg)
{
	mavlink_hil_state_t *p = (mavlink_hil_state_t *)&msg->payload[0];
	return (float)(p->pitchspeed);
}

/**
 * @brief Get field yawspeed from hil_state message
 *
 * @return Yaw angular speed (rad/s)
 */
static inline float mavlink_msg_hil_state_get_yawspeed(const mavlink_message_t* msg)
{
	mavlink_hil_state_t *p = (mavlink_hil_state_t *)&msg->payload[0];
	return (float)(p->yawspeed);
}

/**
 * @brief Get field lat from hil_state message
 *
 * @return Latitude, expressed as * 1E7
 */
static inline int32_t mavlink_msg_hil_state_get_lat(const mavlink_message_t* msg)
{
	mavlink_hil_state_t *p = (mavlink_hil_state_t *)&msg->payload[0];
	return (int32_t)(p->lat);
}

/**
 * @brief Get field lon from hil_state message
 *
 * @return Longitude, expressed as * 1E7
 */
static inline int32_t mavlink_msg_hil_state_get_lon(const mavlink_message_t* msg)
{
	mavlink_hil_state_t *p = (mavlink_hil_state_t *)&msg->payload[0];
	return (int32_t)(p->lon);
}

/**
 * @brief Get field alt from hil_state message
 *
 * @return Altitude in meters, expressed as * 1000 (millimeters)
 */
static inline int32_t mavlink_msg_hil_state_get_alt(const mavlink_message_t* msg)
{
	mavlink_hil_state_t *p = (mavlink_hil_state_t *)&msg->payload[0];
	return (int32_t)(p->alt);
}

/**
 * @brief Get field vx from hil_state message
 *
 * @return Ground X Speed (Latitude), expressed as m/s * 100
 */
static inline int16_t mavlink_msg_hil_state_get_vx(const mavlink_message_t* msg)
{
	mavlink_hil_state_t *p = (mavlink_hil_state_t *)&msg->payload[0];
	return (int16_t)(p->vx);
}

/**
 * @brief Get field vy from hil_state message
 *
 * @return Ground Y Speed (Longitude), expressed as m/s * 100
 */
static inline int16_t mavlink_msg_hil_state_get_vy(const mavlink_message_t* msg)
{
	mavlink_hil_state_t *p = (mavlink_hil_state_t *)&msg->payload[0];
	return (int16_t)(p->vy);
}

/**
 * @brief Get field vz from hil_state message
 *
 * @return Ground Z Speed (Altitude), expressed as m/s * 100
 */
static inline int16_t mavlink_msg_hil_state_get_vz(const mavlink_message_t* msg)
{
	mavlink_hil_state_t *p = (mavlink_hil_state_t *)&msg->payload[0];
	return (int16_t)(p->vz);
}

/**
 * @brief Get field xacc from hil_state message
 *
 * @return X acceleration (mg)
 */
static inline int16_t mavlink_msg_hil_state_get_xacc(const mavlink_message_t* msg)
{
	mavlink_hil_state_t *p = (mavlink_hil_state_t *)&msg->payload[0];
	return (int16_t)(p->xacc);
}

/**
 * @brief Get field yacc from hil_state message
 *
 * @return Y acceleration (mg)
 */
static inline int16_t mavlink_msg_hil_state_get_yacc(const mavlink_message_t* msg)
{
	mavlink_hil_state_t *p = (mavlink_hil_state_t *)&msg->payload[0];
	return (int16_t)(p->yacc);
}

/**
 * @brief Get field zacc from hil_state message
 *
 * @return Z acceleration (mg)
 */
static inline int16_t mavlink_msg_hil_state_get_zacc(const mavlink_message_t* msg)
{
	mavlink_hil_state_t *p = (mavlink_hil_state_t *)&msg->payload[0];
	return (int16_t)(p->zacc);
}

/**
 * @brief Decode a hil_state message into a struct
 *
 * @param msg The message to decode
 * @param hil_state C-struct to decode the message contents into
 */
static inline void mavlink_msg_hil_state_decode(const mavlink_message_t* msg, mavlink_hil_state_t* hil_state)
{
	memcpy( hil_state, msg->payload, sizeof(mavlink_hil_state_t));
}
