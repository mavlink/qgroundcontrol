// MESSAGE SET_ROLL_PITCH_YAW_THRUST PACKING

#define MAVLINK_MSG_ID_SET_ROLL_PITCH_YAW_THRUST 55
#define MAVLINK_MSG_ID_SET_ROLL_PITCH_YAW_THRUST_LEN 18
#define MAVLINK_MSG_55_LEN 18
#define MAVLINK_MSG_ID_SET_ROLL_PITCH_YAW_THRUST_KEY 0xA1
#define MAVLINK_MSG_55_KEY 0xA1

typedef struct __mavlink_set_roll_pitch_yaw_thrust_t 
{
	float roll;	///< Desired roll angle in radians
	float pitch;	///< Desired pitch angle in radians
	float yaw;	///< Desired yaw angle in radians
	float thrust;	///< Collective thrust, normalized to 0 .. 1
	uint8_t target_system;	///< System ID
	uint8_t target_component;	///< Component ID

} mavlink_set_roll_pitch_yaw_thrust_t;

/**
 * @brief Pack a set_roll_pitch_yaw_thrust message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param roll Desired roll angle in radians
 * @param pitch Desired pitch angle in radians
 * @param yaw Desired yaw angle in radians
 * @param thrust Collective thrust, normalized to 0 .. 1
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_set_roll_pitch_yaw_thrust_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component, float roll, float pitch, float yaw, float thrust)
{
	mavlink_set_roll_pitch_yaw_thrust_t *p = (mavlink_set_roll_pitch_yaw_thrust_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SET_ROLL_PITCH_YAW_THRUST;

	p->target_system = target_system;	// uint8_t:System ID
	p->target_component = target_component;	// uint8_t:Component ID
	p->roll = roll;	// float:Desired roll angle in radians
	p->pitch = pitch;	// float:Desired pitch angle in radians
	p->yaw = yaw;	// float:Desired yaw angle in radians
	p->thrust = thrust;	// float:Collective thrust, normalized to 0 .. 1

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_SET_ROLL_PITCH_YAW_THRUST_LEN);
}

/**
 * @brief Pack a set_roll_pitch_yaw_thrust message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system System ID
 * @param target_component Component ID
 * @param roll Desired roll angle in radians
 * @param pitch Desired pitch angle in radians
 * @param yaw Desired yaw angle in radians
 * @param thrust Collective thrust, normalized to 0 .. 1
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_set_roll_pitch_yaw_thrust_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component, float roll, float pitch, float yaw, float thrust)
{
	mavlink_set_roll_pitch_yaw_thrust_t *p = (mavlink_set_roll_pitch_yaw_thrust_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SET_ROLL_PITCH_YAW_THRUST;

	p->target_system = target_system;	// uint8_t:System ID
	p->target_component = target_component;	// uint8_t:Component ID
	p->roll = roll;	// float:Desired roll angle in radians
	p->pitch = pitch;	// float:Desired pitch angle in radians
	p->yaw = yaw;	// float:Desired yaw angle in radians
	p->thrust = thrust;	// float:Collective thrust, normalized to 0 .. 1

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_SET_ROLL_PITCH_YAW_THRUST_LEN);
}

/**
 * @brief Encode a set_roll_pitch_yaw_thrust struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param set_roll_pitch_yaw_thrust C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_set_roll_pitch_yaw_thrust_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_set_roll_pitch_yaw_thrust_t* set_roll_pitch_yaw_thrust)
{
	return mavlink_msg_set_roll_pitch_yaw_thrust_pack(system_id, component_id, msg, set_roll_pitch_yaw_thrust->target_system, set_roll_pitch_yaw_thrust->target_component, set_roll_pitch_yaw_thrust->roll, set_roll_pitch_yaw_thrust->pitch, set_roll_pitch_yaw_thrust->yaw, set_roll_pitch_yaw_thrust->thrust);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a set_roll_pitch_yaw_thrust message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param roll Desired roll angle in radians
 * @param pitch Desired pitch angle in radians
 * @param yaw Desired yaw angle in radians
 * @param thrust Collective thrust, normalized to 0 .. 1
 */
static inline void mavlink_msg_set_roll_pitch_yaw_thrust_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, float roll, float pitch, float yaw, float thrust)
{
	mavlink_header_t hdr;
	mavlink_set_roll_pitch_yaw_thrust_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_SET_ROLL_PITCH_YAW_THRUST_LEN )
	payload.target_system = target_system;	// uint8_t:System ID
	payload.target_component = target_component;	// uint8_t:Component ID
	payload.roll = roll;	// float:Desired roll angle in radians
	payload.pitch = pitch;	// float:Desired pitch angle in radians
	payload.yaw = yaw;	// float:Desired yaw angle in radians
	payload.thrust = thrust;	// float:Collective thrust, normalized to 0 .. 1

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_SET_ROLL_PITCH_YAW_THRUST_LEN;
	hdr.msgid = MAVLINK_MSG_ID_SET_ROLL_PITCH_YAW_THRUST;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0xA1, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE SET_ROLL_PITCH_YAW_THRUST UNPACKING

/**
 * @brief Get field target_system from set_roll_pitch_yaw_thrust message
 *
 * @return System ID
 */
static inline uint8_t mavlink_msg_set_roll_pitch_yaw_thrust_get_target_system(const mavlink_message_t* msg)
{
	mavlink_set_roll_pitch_yaw_thrust_t *p = (mavlink_set_roll_pitch_yaw_thrust_t *)&msg->payload[0];
	return (uint8_t)(p->target_system);
}

/**
 * @brief Get field target_component from set_roll_pitch_yaw_thrust message
 *
 * @return Component ID
 */
static inline uint8_t mavlink_msg_set_roll_pitch_yaw_thrust_get_target_component(const mavlink_message_t* msg)
{
	mavlink_set_roll_pitch_yaw_thrust_t *p = (mavlink_set_roll_pitch_yaw_thrust_t *)&msg->payload[0];
	return (uint8_t)(p->target_component);
}

/**
 * @brief Get field roll from set_roll_pitch_yaw_thrust message
 *
 * @return Desired roll angle in radians
 */
static inline float mavlink_msg_set_roll_pitch_yaw_thrust_get_roll(const mavlink_message_t* msg)
{
	mavlink_set_roll_pitch_yaw_thrust_t *p = (mavlink_set_roll_pitch_yaw_thrust_t *)&msg->payload[0];
	return (float)(p->roll);
}

/**
 * @brief Get field pitch from set_roll_pitch_yaw_thrust message
 *
 * @return Desired pitch angle in radians
 */
static inline float mavlink_msg_set_roll_pitch_yaw_thrust_get_pitch(const mavlink_message_t* msg)
{
	mavlink_set_roll_pitch_yaw_thrust_t *p = (mavlink_set_roll_pitch_yaw_thrust_t *)&msg->payload[0];
	return (float)(p->pitch);
}

/**
 * @brief Get field yaw from set_roll_pitch_yaw_thrust message
 *
 * @return Desired yaw angle in radians
 */
static inline float mavlink_msg_set_roll_pitch_yaw_thrust_get_yaw(const mavlink_message_t* msg)
{
	mavlink_set_roll_pitch_yaw_thrust_t *p = (mavlink_set_roll_pitch_yaw_thrust_t *)&msg->payload[0];
	return (float)(p->yaw);
}

/**
 * @brief Get field thrust from set_roll_pitch_yaw_thrust message
 *
 * @return Collective thrust, normalized to 0 .. 1
 */
static inline float mavlink_msg_set_roll_pitch_yaw_thrust_get_thrust(const mavlink_message_t* msg)
{
	mavlink_set_roll_pitch_yaw_thrust_t *p = (mavlink_set_roll_pitch_yaw_thrust_t *)&msg->payload[0];
	return (float)(p->thrust);
}

/**
 * @brief Decode a set_roll_pitch_yaw_thrust message into a struct
 *
 * @param msg The message to decode
 * @param set_roll_pitch_yaw_thrust C-struct to decode the message contents into
 */
static inline void mavlink_msg_set_roll_pitch_yaw_thrust_decode(const mavlink_message_t* msg, mavlink_set_roll_pitch_yaw_thrust_t* set_roll_pitch_yaw_thrust)
{
	memcpy( set_roll_pitch_yaw_thrust, msg->payload, sizeof(mavlink_set_roll_pitch_yaw_thrust_t));
}
