// MESSAGE FULL_STATE PACKING

#define MAVLINK_MSG_ID_FULL_STATE 67
#define MAVLINK_MSG_ID_FULL_STATE_LEN 56
#define MAVLINK_MSG_67_LEN 56

typedef struct __mavlink_full_state_t 
{
	uint64_t usec; ///< Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	float roll; ///< Roll angle (rad)
	float pitch; ///< Pitch angle (rad)
	float yaw; ///< Yaw angle (rad)
	float rollspeed; ///< Roll angular speed (rad/s)
	float pitchspeed; ///< Pitch angular speed (rad/s)
	float yawspeed; ///< Yaw angular speed (rad/s)
	int32_t lat; ///< Latitude, expressed as * 1E7
	int32_t lon; ///< Longitude, expressed as * 1E7
	int32_t alt; ///< Altitude in meters, expressed as * 1000 (millimeters)
	int16_t vx; ///< Ground X Speed (Latitude), expressed as m/s * 100
	int16_t vy; ///< Ground Y Speed (Longitude), expressed as m/s * 100
	int16_t vz; ///< Ground Z Speed (Altitude), expressed as m/s * 100
	int16_t xacc; ///< X acceleration (mg)
	int16_t yacc; ///< Y acceleration (mg)
	int16_t zacc; ///< Z acceleration (mg)

} mavlink_full_state_t;

/**
 * @brief Pack a full_state message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param usec Timestamp (microseconds since UNIX epoch or microseconds since system boot)
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
static inline uint16_t mavlink_msg_full_state_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint64_t usec, float roll, float pitch, float yaw, float rollspeed, float pitchspeed, float yawspeed, int32_t lat, int32_t lon, int32_t alt, int16_t vx, int16_t vy, int16_t vz, int16_t xacc, int16_t yacc, int16_t zacc)
{
	mavlink_full_state_t *p = (mavlink_full_state_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_FULL_STATE;

	p->usec = usec; // uint64_t:Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	p->roll = roll; // float:Roll angle (rad)
	p->pitch = pitch; // float:Pitch angle (rad)
	p->yaw = yaw; // float:Yaw angle (rad)
	p->rollspeed = rollspeed; // float:Roll angular speed (rad/s)
	p->pitchspeed = pitchspeed; // float:Pitch angular speed (rad/s)
	p->yawspeed = yawspeed; // float:Yaw angular speed (rad/s)
	p->lat = lat; // int32_t:Latitude, expressed as * 1E7
	p->lon = lon; // int32_t:Longitude, expressed as * 1E7
	p->alt = alt; // int32_t:Altitude in meters, expressed as * 1000 (millimeters)
	p->vx = vx; // int16_t:Ground X Speed (Latitude), expressed as m/s * 100
	p->vy = vy; // int16_t:Ground Y Speed (Longitude), expressed as m/s * 100
	p->vz = vz; // int16_t:Ground Z Speed (Altitude), expressed as m/s * 100
	p->xacc = xacc; // int16_t:X acceleration (mg)
	p->yacc = yacc; // int16_t:Y acceleration (mg)
	p->zacc = zacc; // int16_t:Z acceleration (mg)

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_FULL_STATE_LEN);
}

/**
 * @brief Pack a full_state message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param usec Timestamp (microseconds since UNIX epoch or microseconds since system boot)
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
static inline uint16_t mavlink_msg_full_state_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint64_t usec, float roll, float pitch, float yaw, float rollspeed, float pitchspeed, float yawspeed, int32_t lat, int32_t lon, int32_t alt, int16_t vx, int16_t vy, int16_t vz, int16_t xacc, int16_t yacc, int16_t zacc)
{
	mavlink_full_state_t *p = (mavlink_full_state_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_FULL_STATE;

	p->usec = usec; // uint64_t:Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	p->roll = roll; // float:Roll angle (rad)
	p->pitch = pitch; // float:Pitch angle (rad)
	p->yaw = yaw; // float:Yaw angle (rad)
	p->rollspeed = rollspeed; // float:Roll angular speed (rad/s)
	p->pitchspeed = pitchspeed; // float:Pitch angular speed (rad/s)
	p->yawspeed = yawspeed; // float:Yaw angular speed (rad/s)
	p->lat = lat; // int32_t:Latitude, expressed as * 1E7
	p->lon = lon; // int32_t:Longitude, expressed as * 1E7
	p->alt = alt; // int32_t:Altitude in meters, expressed as * 1000 (millimeters)
	p->vx = vx; // int16_t:Ground X Speed (Latitude), expressed as m/s * 100
	p->vy = vy; // int16_t:Ground Y Speed (Longitude), expressed as m/s * 100
	p->vz = vz; // int16_t:Ground Z Speed (Altitude), expressed as m/s * 100
	p->xacc = xacc; // int16_t:X acceleration (mg)
	p->yacc = yacc; // int16_t:Y acceleration (mg)
	p->zacc = zacc; // int16_t:Z acceleration (mg)

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_FULL_STATE_LEN);
}

/**
 * @brief Encode a full_state struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param full_state C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_full_state_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_full_state_t* full_state)
{
	return mavlink_msg_full_state_pack(system_id, component_id, msg, full_state->usec, full_state->roll, full_state->pitch, full_state->yaw, full_state->rollspeed, full_state->pitchspeed, full_state->yawspeed, full_state->lat, full_state->lon, full_state->alt, full_state->vx, full_state->vy, full_state->vz, full_state->xacc, full_state->yacc, full_state->zacc);
}

/**
 * @brief Send a full_state message
 * @param chan MAVLink channel to send the message
 *
 * @param usec Timestamp (microseconds since UNIX epoch or microseconds since system boot)
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


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_full_state_send(mavlink_channel_t chan, uint64_t usec, float roll, float pitch, float yaw, float rollspeed, float pitchspeed, float yawspeed, int32_t lat, int32_t lon, int32_t alt, int16_t vx, int16_t vy, int16_t vz, int16_t xacc, int16_t yacc, int16_t zacc)
{
	mavlink_header_t hdr;
	mavlink_full_state_t payload;
	uint16_t checksum;
	mavlink_full_state_t *p = &payload;

	p->usec = usec; // uint64_t:Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	p->roll = roll; // float:Roll angle (rad)
	p->pitch = pitch; // float:Pitch angle (rad)
	p->yaw = yaw; // float:Yaw angle (rad)
	p->rollspeed = rollspeed; // float:Roll angular speed (rad/s)
	p->pitchspeed = pitchspeed; // float:Pitch angular speed (rad/s)
	p->yawspeed = yawspeed; // float:Yaw angular speed (rad/s)
	p->lat = lat; // int32_t:Latitude, expressed as * 1E7
	p->lon = lon; // int32_t:Longitude, expressed as * 1E7
	p->alt = alt; // int32_t:Altitude in meters, expressed as * 1000 (millimeters)
	p->vx = vx; // int16_t:Ground X Speed (Latitude), expressed as m/s * 100
	p->vy = vy; // int16_t:Ground Y Speed (Longitude), expressed as m/s * 100
	p->vz = vz; // int16_t:Ground Z Speed (Altitude), expressed as m/s * 100
	p->xacc = xacc; // int16_t:X acceleration (mg)
	p->yacc = yacc; // int16_t:Y acceleration (mg)
	p->zacc = zacc; // int16_t:Z acceleration (mg)

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_FULL_STATE_LEN;
	hdr.msgid = MAVLINK_MSG_ID_FULL_STATE;
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
// MESSAGE FULL_STATE UNPACKING

/**
 * @brief Get field usec from full_state message
 *
 * @return Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 */
static inline uint64_t mavlink_msg_full_state_get_usec(const mavlink_message_t* msg)
{
	mavlink_full_state_t *p = (mavlink_full_state_t *)&msg->payload[0];
	return (uint64_t)(p->usec);
}

/**
 * @brief Get field roll from full_state message
 *
 * @return Roll angle (rad)
 */
static inline float mavlink_msg_full_state_get_roll(const mavlink_message_t* msg)
{
	mavlink_full_state_t *p = (mavlink_full_state_t *)&msg->payload[0];
	return (float)(p->roll);
}

/**
 * @brief Get field pitch from full_state message
 *
 * @return Pitch angle (rad)
 */
static inline float mavlink_msg_full_state_get_pitch(const mavlink_message_t* msg)
{
	mavlink_full_state_t *p = (mavlink_full_state_t *)&msg->payload[0];
	return (float)(p->pitch);
}

/**
 * @brief Get field yaw from full_state message
 *
 * @return Yaw angle (rad)
 */
static inline float mavlink_msg_full_state_get_yaw(const mavlink_message_t* msg)
{
	mavlink_full_state_t *p = (mavlink_full_state_t *)&msg->payload[0];
	return (float)(p->yaw);
}

/**
 * @brief Get field rollspeed from full_state message
 *
 * @return Roll angular speed (rad/s)
 */
static inline float mavlink_msg_full_state_get_rollspeed(const mavlink_message_t* msg)
{
	mavlink_full_state_t *p = (mavlink_full_state_t *)&msg->payload[0];
	return (float)(p->rollspeed);
}

/**
 * @brief Get field pitchspeed from full_state message
 *
 * @return Pitch angular speed (rad/s)
 */
static inline float mavlink_msg_full_state_get_pitchspeed(const mavlink_message_t* msg)
{
	mavlink_full_state_t *p = (mavlink_full_state_t *)&msg->payload[0];
	return (float)(p->pitchspeed);
}

/**
 * @brief Get field yawspeed from full_state message
 *
 * @return Yaw angular speed (rad/s)
 */
static inline float mavlink_msg_full_state_get_yawspeed(const mavlink_message_t* msg)
{
	mavlink_full_state_t *p = (mavlink_full_state_t *)&msg->payload[0];
	return (float)(p->yawspeed);
}

/**
 * @brief Get field lat from full_state message
 *
 * @return Latitude, expressed as * 1E7
 */
static inline int32_t mavlink_msg_full_state_get_lat(const mavlink_message_t* msg)
{
	mavlink_full_state_t *p = (mavlink_full_state_t *)&msg->payload[0];
	return (int32_t)(p->lat);
}

/**
 * @brief Get field lon from full_state message
 *
 * @return Longitude, expressed as * 1E7
 */
static inline int32_t mavlink_msg_full_state_get_lon(const mavlink_message_t* msg)
{
	mavlink_full_state_t *p = (mavlink_full_state_t *)&msg->payload[0];
	return (int32_t)(p->lon);
}

/**
 * @brief Get field alt from full_state message
 *
 * @return Altitude in meters, expressed as * 1000 (millimeters)
 */
static inline int32_t mavlink_msg_full_state_get_alt(const mavlink_message_t* msg)
{
	mavlink_full_state_t *p = (mavlink_full_state_t *)&msg->payload[0];
	return (int32_t)(p->alt);
}

/**
 * @brief Get field vx from full_state message
 *
 * @return Ground X Speed (Latitude), expressed as m/s * 100
 */
static inline int16_t mavlink_msg_full_state_get_vx(const mavlink_message_t* msg)
{
	mavlink_full_state_t *p = (mavlink_full_state_t *)&msg->payload[0];
	return (int16_t)(p->vx);
}

/**
 * @brief Get field vy from full_state message
 *
 * @return Ground Y Speed (Longitude), expressed as m/s * 100
 */
static inline int16_t mavlink_msg_full_state_get_vy(const mavlink_message_t* msg)
{
	mavlink_full_state_t *p = (mavlink_full_state_t *)&msg->payload[0];
	return (int16_t)(p->vy);
}

/**
 * @brief Get field vz from full_state message
 *
 * @return Ground Z Speed (Altitude), expressed as m/s * 100
 */
static inline int16_t mavlink_msg_full_state_get_vz(const mavlink_message_t* msg)
{
	mavlink_full_state_t *p = (mavlink_full_state_t *)&msg->payload[0];
	return (int16_t)(p->vz);
}

/**
 * @brief Get field xacc from full_state message
 *
 * @return X acceleration (mg)
 */
static inline int16_t mavlink_msg_full_state_get_xacc(const mavlink_message_t* msg)
{
	mavlink_full_state_t *p = (mavlink_full_state_t *)&msg->payload[0];
	return (int16_t)(p->xacc);
}

/**
 * @brief Get field yacc from full_state message
 *
 * @return Y acceleration (mg)
 */
static inline int16_t mavlink_msg_full_state_get_yacc(const mavlink_message_t* msg)
{
	mavlink_full_state_t *p = (mavlink_full_state_t *)&msg->payload[0];
	return (int16_t)(p->yacc);
}

/**
 * @brief Get field zacc from full_state message
 *
 * @return Z acceleration (mg)
 */
static inline int16_t mavlink_msg_full_state_get_zacc(const mavlink_message_t* msg)
{
	mavlink_full_state_t *p = (mavlink_full_state_t *)&msg->payload[0];
	return (int16_t)(p->zacc);
}

/**
 * @brief Decode a full_state message into a struct
 *
 * @param msg The message to decode
 * @param full_state C-struct to decode the message contents into
 */
static inline void mavlink_msg_full_state_decode(const mavlink_message_t* msg, mavlink_full_state_t* full_state)
{
	memcpy( full_state, msg->payload, sizeof(mavlink_full_state_t));
}
