// MESSAGE ROLL_PITCH_YAW_SPEED_SETPOINT PACKING

#define MAVLINK_MSG_ID_ROLL_PITCH_YAW_SPEED_SETPOINT 58
#define MAVLINK_MSG_ID_ROLL_PITCH_YAW_SPEED_SETPOINT_LEN 16
#define MAVLINK_MSG_58_LEN 16

typedef struct __mavlink_roll_pitch_yaw_speed_setpoint_t 
{
	uint32_t time_ms; ///< Timestamp in milliseconds since system boot
	float roll_speed; ///< Desired roll angular speed in rad/s
	float pitch_speed; ///< Desired pitch angular speed in rad/s
	float yaw_speed; ///< Desired yaw angular speed in rad/s

} mavlink_roll_pitch_yaw_speed_setpoint_t;

/**
 * @brief Pack a roll_pitch_yaw_speed_setpoint message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_ms Timestamp in milliseconds since system boot
 * @param roll_speed Desired roll angular speed in rad/s
 * @param pitch_speed Desired pitch angular speed in rad/s
 * @param yaw_speed Desired yaw angular speed in rad/s
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_roll_pitch_yaw_speed_setpoint_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint32_t time_ms, float roll_speed, float pitch_speed, float yaw_speed)
{
	mavlink_roll_pitch_yaw_speed_setpoint_t *p = (mavlink_roll_pitch_yaw_speed_setpoint_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_ROLL_PITCH_YAW_SPEED_SETPOINT;

	p->time_ms = time_ms; // uint32_t:Timestamp in milliseconds since system boot
	p->roll_speed = roll_speed; // float:Desired roll angular speed in rad/s
	p->pitch_speed = pitch_speed; // float:Desired pitch angular speed in rad/s
	p->yaw_speed = yaw_speed; // float:Desired yaw angular speed in rad/s

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_ROLL_PITCH_YAW_SPEED_SETPOINT_LEN);
}

/**
 * @brief Pack a roll_pitch_yaw_speed_setpoint message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param time_ms Timestamp in milliseconds since system boot
 * @param roll_speed Desired roll angular speed in rad/s
 * @param pitch_speed Desired pitch angular speed in rad/s
 * @param yaw_speed Desired yaw angular speed in rad/s
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_roll_pitch_yaw_speed_setpoint_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint32_t time_ms, float roll_speed, float pitch_speed, float yaw_speed)
{
	mavlink_roll_pitch_yaw_speed_setpoint_t *p = (mavlink_roll_pitch_yaw_speed_setpoint_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_ROLL_PITCH_YAW_SPEED_SETPOINT;

	p->time_ms = time_ms; // uint32_t:Timestamp in milliseconds since system boot
	p->roll_speed = roll_speed; // float:Desired roll angular speed in rad/s
	p->pitch_speed = pitch_speed; // float:Desired pitch angular speed in rad/s
	p->yaw_speed = yaw_speed; // float:Desired yaw angular speed in rad/s

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_ROLL_PITCH_YAW_SPEED_SETPOINT_LEN);
}

/**
 * @brief Encode a roll_pitch_yaw_speed_setpoint struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param roll_pitch_yaw_speed_setpoint C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_roll_pitch_yaw_speed_setpoint_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_roll_pitch_yaw_speed_setpoint_t* roll_pitch_yaw_speed_setpoint)
{
	return mavlink_msg_roll_pitch_yaw_speed_setpoint_pack(system_id, component_id, msg, roll_pitch_yaw_speed_setpoint->time_ms, roll_pitch_yaw_speed_setpoint->roll_speed, roll_pitch_yaw_speed_setpoint->pitch_speed, roll_pitch_yaw_speed_setpoint->yaw_speed);
}

/**
 * @brief Send a roll_pitch_yaw_speed_setpoint message
 * @param chan MAVLink channel to send the message
 *
 * @param time_ms Timestamp in milliseconds since system boot
 * @param roll_speed Desired roll angular speed in rad/s
 * @param pitch_speed Desired pitch angular speed in rad/s
 * @param yaw_speed Desired yaw angular speed in rad/s
 */


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_roll_pitch_yaw_speed_setpoint_send(mavlink_channel_t chan, uint32_t time_ms, float roll_speed, float pitch_speed, float yaw_speed)
{
	mavlink_header_t hdr;
	mavlink_roll_pitch_yaw_speed_setpoint_t payload;
	uint16_t checksum;
	mavlink_roll_pitch_yaw_speed_setpoint_t *p = &payload;

	p->time_ms = time_ms; // uint32_t:Timestamp in milliseconds since system boot
	p->roll_speed = roll_speed; // float:Desired roll angular speed in rad/s
	p->pitch_speed = pitch_speed; // float:Desired pitch angular speed in rad/s
	p->yaw_speed = yaw_speed; // float:Desired yaw angular speed in rad/s

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_ROLL_PITCH_YAW_SPEED_SETPOINT_LEN;
	hdr.msgid = MAVLINK_MSG_ID_ROLL_PITCH_YAW_SPEED_SETPOINT;
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
// MESSAGE ROLL_PITCH_YAW_SPEED_SETPOINT UNPACKING

/**
 * @brief Get field time_ms from roll_pitch_yaw_speed_setpoint message
 *
 * @return Timestamp in milliseconds since system boot
 */
static inline uint32_t mavlink_msg_roll_pitch_yaw_speed_setpoint_get_time_ms(const mavlink_message_t* msg)
{
	mavlink_roll_pitch_yaw_speed_setpoint_t *p = (mavlink_roll_pitch_yaw_speed_setpoint_t *)&msg->payload[0];
	return (uint32_t)(p->time_ms);
}

/**
 * @brief Get field roll_speed from roll_pitch_yaw_speed_setpoint message
 *
 * @return Desired roll angular speed in rad/s
 */
static inline float mavlink_msg_roll_pitch_yaw_speed_setpoint_get_roll_speed(const mavlink_message_t* msg)
{
	mavlink_roll_pitch_yaw_speed_setpoint_t *p = (mavlink_roll_pitch_yaw_speed_setpoint_t *)&msg->payload[0];
	return (float)(p->roll_speed);
}

/**
 * @brief Get field pitch_speed from roll_pitch_yaw_speed_setpoint message
 *
 * @return Desired pitch angular speed in rad/s
 */
static inline float mavlink_msg_roll_pitch_yaw_speed_setpoint_get_pitch_speed(const mavlink_message_t* msg)
{
	mavlink_roll_pitch_yaw_speed_setpoint_t *p = (mavlink_roll_pitch_yaw_speed_setpoint_t *)&msg->payload[0];
	return (float)(p->pitch_speed);
}

/**
 * @brief Get field yaw_speed from roll_pitch_yaw_speed_setpoint message
 *
 * @return Desired yaw angular speed in rad/s
 */
static inline float mavlink_msg_roll_pitch_yaw_speed_setpoint_get_yaw_speed(const mavlink_message_t* msg)
{
	mavlink_roll_pitch_yaw_speed_setpoint_t *p = (mavlink_roll_pitch_yaw_speed_setpoint_t *)&msg->payload[0];
	return (float)(p->yaw_speed);
}

/**
 * @brief Decode a roll_pitch_yaw_speed_setpoint message into a struct
 *
 * @param msg The message to decode
 * @param roll_pitch_yaw_speed_setpoint C-struct to decode the message contents into
 */
static inline void mavlink_msg_roll_pitch_yaw_speed_setpoint_decode(const mavlink_message_t* msg, mavlink_roll_pitch_yaw_speed_setpoint_t* roll_pitch_yaw_speed_setpoint)
{
	memcpy( roll_pitch_yaw_speed_setpoint, msg->payload, sizeof(mavlink_roll_pitch_yaw_speed_setpoint_t));
}
