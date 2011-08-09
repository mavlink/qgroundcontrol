// MESSAGE SET_ROLL_PITCH_YAW_SPEED PACKING

#define MAVLINK_MSG_ID_SET_ROLL_PITCH_YAW_SPEED 56
#define MAVLINK_MSG_ID_SET_ROLL_PITCH_YAW_SPEED_LEN 14
#define MAVLINK_MSG_56_LEN 14

typedef struct __mavlink_set_roll_pitch_yaw_speed_t 
{
	float roll_speed; ///< Desired roll angular speed in rad/s
	float pitch_speed; ///< Desired pitch angular speed in rad/s
	float yaw_speed; ///< Desired yaw angular speed in rad/s
	uint8_t target_system; ///< System ID
	uint8_t target_component; ///< Component ID

} mavlink_set_roll_pitch_yaw_speed_t;

/**
 * @brief Pack a set_roll_pitch_yaw_speed message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param roll_speed Desired roll angular speed in rad/s
 * @param pitch_speed Desired pitch angular speed in rad/s
 * @param yaw_speed Desired yaw angular speed in rad/s
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_set_roll_pitch_yaw_speed_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component, float roll_speed, float pitch_speed, float yaw_speed)
{
	mavlink_set_roll_pitch_yaw_speed_t *p = (mavlink_set_roll_pitch_yaw_speed_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SET_ROLL_PITCH_YAW_SPEED;

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	p->roll_speed = roll_speed; // float:Desired roll angular speed in rad/s
	p->pitch_speed = pitch_speed; // float:Desired pitch angular speed in rad/s
	p->yaw_speed = yaw_speed; // float:Desired yaw angular speed in rad/s

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_SET_ROLL_PITCH_YAW_SPEED_LEN);
}

/**
 * @brief Pack a set_roll_pitch_yaw_speed message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system System ID
 * @param target_component Component ID
 * @param roll_speed Desired roll angular speed in rad/s
 * @param pitch_speed Desired pitch angular speed in rad/s
 * @param yaw_speed Desired yaw angular speed in rad/s
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_set_roll_pitch_yaw_speed_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component, float roll_speed, float pitch_speed, float yaw_speed)
{
	mavlink_set_roll_pitch_yaw_speed_t *p = (mavlink_set_roll_pitch_yaw_speed_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SET_ROLL_PITCH_YAW_SPEED;

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	p->roll_speed = roll_speed; // float:Desired roll angular speed in rad/s
	p->pitch_speed = pitch_speed; // float:Desired pitch angular speed in rad/s
	p->yaw_speed = yaw_speed; // float:Desired yaw angular speed in rad/s

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_SET_ROLL_PITCH_YAW_SPEED_LEN);
}

/**
 * @brief Encode a set_roll_pitch_yaw_speed struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param set_roll_pitch_yaw_speed C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_set_roll_pitch_yaw_speed_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_set_roll_pitch_yaw_speed_t* set_roll_pitch_yaw_speed)
{
	return mavlink_msg_set_roll_pitch_yaw_speed_pack(system_id, component_id, msg, set_roll_pitch_yaw_speed->target_system, set_roll_pitch_yaw_speed->target_component, set_roll_pitch_yaw_speed->roll_speed, set_roll_pitch_yaw_speed->pitch_speed, set_roll_pitch_yaw_speed->yaw_speed);
}

/**
 * @brief Send a set_roll_pitch_yaw_speed message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param roll_speed Desired roll angular speed in rad/s
 * @param pitch_speed Desired pitch angular speed in rad/s
 * @param yaw_speed Desired yaw angular speed in rad/s
 */


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_set_roll_pitch_yaw_speed_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, float roll_speed, float pitch_speed, float yaw_speed)
{
	mavlink_header_t hdr;
	mavlink_set_roll_pitch_yaw_speed_t payload;
	uint16_t checksum;
	mavlink_set_roll_pitch_yaw_speed_t *p = &payload;

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	p->roll_speed = roll_speed; // float:Desired roll angular speed in rad/s
	p->pitch_speed = pitch_speed; // float:Desired pitch angular speed in rad/s
	p->yaw_speed = yaw_speed; // float:Desired yaw angular speed in rad/s

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_SET_ROLL_PITCH_YAW_SPEED_LEN;
	hdr.msgid = MAVLINK_MSG_ID_SET_ROLL_PITCH_YAW_SPEED;
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
// MESSAGE SET_ROLL_PITCH_YAW_SPEED UNPACKING

/**
 * @brief Get field target_system from set_roll_pitch_yaw_speed message
 *
 * @return System ID
 */
static inline uint8_t mavlink_msg_set_roll_pitch_yaw_speed_get_target_system(const mavlink_message_t* msg)
{
	mavlink_set_roll_pitch_yaw_speed_t *p = (mavlink_set_roll_pitch_yaw_speed_t *)&msg->payload[0];
	return (uint8_t)(p->target_system);
}

/**
 * @brief Get field target_component from set_roll_pitch_yaw_speed message
 *
 * @return Component ID
 */
static inline uint8_t mavlink_msg_set_roll_pitch_yaw_speed_get_target_component(const mavlink_message_t* msg)
{
	mavlink_set_roll_pitch_yaw_speed_t *p = (mavlink_set_roll_pitch_yaw_speed_t *)&msg->payload[0];
	return (uint8_t)(p->target_component);
}

/**
 * @brief Get field roll_speed from set_roll_pitch_yaw_speed message
 *
 * @return Desired roll angular speed in rad/s
 */
static inline float mavlink_msg_set_roll_pitch_yaw_speed_get_roll_speed(const mavlink_message_t* msg)
{
	mavlink_set_roll_pitch_yaw_speed_t *p = (mavlink_set_roll_pitch_yaw_speed_t *)&msg->payload[0];
	return (float)(p->roll_speed);
}

/**
 * @brief Get field pitch_speed from set_roll_pitch_yaw_speed message
 *
 * @return Desired pitch angular speed in rad/s
 */
static inline float mavlink_msg_set_roll_pitch_yaw_speed_get_pitch_speed(const mavlink_message_t* msg)
{
	mavlink_set_roll_pitch_yaw_speed_t *p = (mavlink_set_roll_pitch_yaw_speed_t *)&msg->payload[0];
	return (float)(p->pitch_speed);
}

/**
 * @brief Get field yaw_speed from set_roll_pitch_yaw_speed message
 *
 * @return Desired yaw angular speed in rad/s
 */
static inline float mavlink_msg_set_roll_pitch_yaw_speed_get_yaw_speed(const mavlink_message_t* msg)
{
	mavlink_set_roll_pitch_yaw_speed_t *p = (mavlink_set_roll_pitch_yaw_speed_t *)&msg->payload[0];
	return (float)(p->yaw_speed);
}

/**
 * @brief Decode a set_roll_pitch_yaw_speed message into a struct
 *
 * @param msg The message to decode
 * @param set_roll_pitch_yaw_speed C-struct to decode the message contents into
 */
static inline void mavlink_msg_set_roll_pitch_yaw_speed_decode(const mavlink_message_t* msg, mavlink_set_roll_pitch_yaw_speed_t* set_roll_pitch_yaw_speed)
{
	memcpy( set_roll_pitch_yaw_speed, msg->payload, sizeof(mavlink_set_roll_pitch_yaw_speed_t));
}
