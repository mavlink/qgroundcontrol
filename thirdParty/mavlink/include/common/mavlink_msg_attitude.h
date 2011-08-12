// MESSAGE ATTITUDE PACKING

#define MAVLINK_MSG_ID_ATTITUDE 30
#define MAVLINK_MSG_ID_ATTITUDE_LEN 32
#define MAVLINK_MSG_30_LEN 32
#define MAVLINK_MSG_ID_ATTITUDE_KEY 0xF3
#define MAVLINK_MSG_30_KEY 0xF3

typedef struct __mavlink_attitude_t 
{
	uint64_t usec;	///< Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	float roll;	///< Roll angle (rad)
	float pitch;	///< Pitch angle (rad)
	float yaw;	///< Yaw angle (rad)
	float rollspeed;	///< Roll angular speed (rad/s)
	float pitchspeed;	///< Pitch angular speed (rad/s)
	float yawspeed;	///< Yaw angular speed (rad/s)

} mavlink_attitude_t;

/**
 * @brief Pack a attitude message
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
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_attitude_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint64_t usec, float roll, float pitch, float yaw, float rollspeed, float pitchspeed, float yawspeed)
{
	mavlink_attitude_t *p = (mavlink_attitude_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_ATTITUDE;

	p->usec = usec;	// uint64_t:Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	p->roll = roll;	// float:Roll angle (rad)
	p->pitch = pitch;	// float:Pitch angle (rad)
	p->yaw = yaw;	// float:Yaw angle (rad)
	p->rollspeed = rollspeed;	// float:Roll angular speed (rad/s)
	p->pitchspeed = pitchspeed;	// float:Pitch angular speed (rad/s)
	p->yawspeed = yawspeed;	// float:Yaw angular speed (rad/s)

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_ATTITUDE_LEN);
}

/**
 * @brief Pack a attitude message
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
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_attitude_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint64_t usec, float roll, float pitch, float yaw, float rollspeed, float pitchspeed, float yawspeed)
{
	mavlink_attitude_t *p = (mavlink_attitude_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_ATTITUDE;

	p->usec = usec;	// uint64_t:Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	p->roll = roll;	// float:Roll angle (rad)
	p->pitch = pitch;	// float:Pitch angle (rad)
	p->yaw = yaw;	// float:Yaw angle (rad)
	p->rollspeed = rollspeed;	// float:Roll angular speed (rad/s)
	p->pitchspeed = pitchspeed;	// float:Pitch angular speed (rad/s)
	p->yawspeed = yawspeed;	// float:Yaw angular speed (rad/s)

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_ATTITUDE_LEN);
}

/**
 * @brief Encode a attitude struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param attitude C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_attitude_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_attitude_t* attitude)
{
	return mavlink_msg_attitude_pack(system_id, component_id, msg, attitude->usec, attitude->roll, attitude->pitch, attitude->yaw, attitude->rollspeed, attitude->pitchspeed, attitude->yawspeed);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a attitude message
 * @param chan MAVLink channel to send the message
 *
 * @param usec Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 * @param roll Roll angle (rad)
 * @param pitch Pitch angle (rad)
 * @param yaw Yaw angle (rad)
 * @param rollspeed Roll angular speed (rad/s)
 * @param pitchspeed Pitch angular speed (rad/s)
 * @param yawspeed Yaw angular speed (rad/s)
 */
static inline void mavlink_msg_attitude_send(mavlink_channel_t chan, uint64_t usec, float roll, float pitch, float yaw, float rollspeed, float pitchspeed, float yawspeed)
{
	mavlink_header_t hdr;
	mavlink_attitude_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_ATTITUDE_LEN )
	payload.usec = usec;	// uint64_t:Timestamp (microseconds since UNIX epoch or microseconds since system boot)
	payload.roll = roll;	// float:Roll angle (rad)
	payload.pitch = pitch;	// float:Pitch angle (rad)
	payload.yaw = yaw;	// float:Yaw angle (rad)
	payload.rollspeed = rollspeed;	// float:Roll angular speed (rad/s)
	payload.pitchspeed = pitchspeed;	// float:Pitch angular speed (rad/s)
	payload.yawspeed = yawspeed;	// float:Yaw angular speed (rad/s)

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_ATTITUDE_LEN;
	hdr.msgid = MAVLINK_MSG_ID_ATTITUDE;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0xF3, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE ATTITUDE UNPACKING

/**
 * @brief Get field usec from attitude message
 *
 * @return Timestamp (microseconds since UNIX epoch or microseconds since system boot)
 */
static inline uint64_t mavlink_msg_attitude_get_usec(const mavlink_message_t* msg)
{
	mavlink_attitude_t *p = (mavlink_attitude_t *)&msg->payload[0];
	return (uint64_t)(p->usec);
}

/**
 * @brief Get field roll from attitude message
 *
 * @return Roll angle (rad)
 */
static inline float mavlink_msg_attitude_get_roll(const mavlink_message_t* msg)
{
	mavlink_attitude_t *p = (mavlink_attitude_t *)&msg->payload[0];
	return (float)(p->roll);
}

/**
 * @brief Get field pitch from attitude message
 *
 * @return Pitch angle (rad)
 */
static inline float mavlink_msg_attitude_get_pitch(const mavlink_message_t* msg)
{
	mavlink_attitude_t *p = (mavlink_attitude_t *)&msg->payload[0];
	return (float)(p->pitch);
}

/**
 * @brief Get field yaw from attitude message
 *
 * @return Yaw angle (rad)
 */
static inline float mavlink_msg_attitude_get_yaw(const mavlink_message_t* msg)
{
	mavlink_attitude_t *p = (mavlink_attitude_t *)&msg->payload[0];
	return (float)(p->yaw);
}

/**
 * @brief Get field rollspeed from attitude message
 *
 * @return Roll angular speed (rad/s)
 */
static inline float mavlink_msg_attitude_get_rollspeed(const mavlink_message_t* msg)
{
	mavlink_attitude_t *p = (mavlink_attitude_t *)&msg->payload[0];
	return (float)(p->rollspeed);
}

/**
 * @brief Get field pitchspeed from attitude message
 *
 * @return Pitch angular speed (rad/s)
 */
static inline float mavlink_msg_attitude_get_pitchspeed(const mavlink_message_t* msg)
{
	mavlink_attitude_t *p = (mavlink_attitude_t *)&msg->payload[0];
	return (float)(p->pitchspeed);
}

/**
 * @brief Get field yawspeed from attitude message
 *
 * @return Yaw angular speed (rad/s)
 */
static inline float mavlink_msg_attitude_get_yawspeed(const mavlink_message_t* msg)
{
	mavlink_attitude_t *p = (mavlink_attitude_t *)&msg->payload[0];
	return (float)(p->yawspeed);
}

/**
 * @brief Decode a attitude message into a struct
 *
 * @param msg The message to decode
 * @param attitude C-struct to decode the message contents into
 */
static inline void mavlink_msg_attitude_decode(const mavlink_message_t* msg, mavlink_attitude_t* attitude)
{
	memcpy( attitude, msg->payload, sizeof(mavlink_attitude_t));
}
