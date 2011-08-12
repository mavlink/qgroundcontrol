// MESSAGE ROLL_PITCH_YAW_SETPOINT PACKING

#define MAVLINK_MSG_ID_ROLL_PITCH_YAW_SETPOINT 57
#define MAVLINK_MSG_ID_ROLL_PITCH_YAW_SETPOINT_LEN 16
#define MAVLINK_MSG_57_LEN 16

typedef struct __mavlink_roll_pitch_yaw_setpoint_t 
{
	uint32_t time_ms; ///< Timestamp in milliseconds since system boot
	float roll; ///< Desired roll angle in radians
	float pitch; ///< Desired pitch angle in radians
	float yaw; ///< Desired yaw angle in radians

} mavlink_roll_pitch_yaw_setpoint_t;

/**
 * @brief Pack a roll_pitch_yaw_setpoint message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param time_ms Timestamp in milliseconds since system boot
 * @param roll Desired roll angle in radians
 * @param pitch Desired pitch angle in radians
 * @param yaw Desired yaw angle in radians
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_roll_pitch_yaw_setpoint_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint32_t time_ms, float roll, float pitch, float yaw)
{
	mavlink_roll_pitch_yaw_setpoint_t *p = (mavlink_roll_pitch_yaw_setpoint_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_ROLL_PITCH_YAW_SETPOINT;

	p->time_ms = time_ms; // uint32_t:Timestamp in milliseconds since system boot
	p->roll = roll; // float:Desired roll angle in radians
	p->pitch = pitch; // float:Desired pitch angle in radians
	p->yaw = yaw; // float:Desired yaw angle in radians

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_ROLL_PITCH_YAW_SETPOINT_LEN);
}

/**
 * @brief Pack a roll_pitch_yaw_setpoint message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param time_ms Timestamp in milliseconds since system boot
 * @param roll Desired roll angle in radians
 * @param pitch Desired pitch angle in radians
 * @param yaw Desired yaw angle in radians
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_roll_pitch_yaw_setpoint_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint32_t time_ms, float roll, float pitch, float yaw)
{
	mavlink_roll_pitch_yaw_setpoint_t *p = (mavlink_roll_pitch_yaw_setpoint_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_ROLL_PITCH_YAW_SETPOINT;

	p->time_ms = time_ms; // uint32_t:Timestamp in milliseconds since system boot
	p->roll = roll; // float:Desired roll angle in radians
	p->pitch = pitch; // float:Desired pitch angle in radians
	p->yaw = yaw; // float:Desired yaw angle in radians

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_ROLL_PITCH_YAW_SETPOINT_LEN);
}

/**
 * @brief Encode a roll_pitch_yaw_setpoint struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param roll_pitch_yaw_setpoint C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_roll_pitch_yaw_setpoint_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_roll_pitch_yaw_setpoint_t* roll_pitch_yaw_setpoint)
{
	return mavlink_msg_roll_pitch_yaw_setpoint_pack(system_id, component_id, msg, roll_pitch_yaw_setpoint->time_ms, roll_pitch_yaw_setpoint->roll, roll_pitch_yaw_setpoint->pitch, roll_pitch_yaw_setpoint->yaw);
}

/**
 * @brief Send a roll_pitch_yaw_setpoint message
 * @param chan MAVLink channel to send the message
 *
 * @param time_ms Timestamp in milliseconds since system boot
 * @param roll Desired roll angle in radians
 * @param pitch Desired pitch angle in radians
 * @param yaw Desired yaw angle in radians
 */


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_roll_pitch_yaw_setpoint_send(mavlink_channel_t chan, uint32_t time_ms, float roll, float pitch, float yaw)
{
	mavlink_header_t hdr;
	mavlink_roll_pitch_yaw_setpoint_t payload;
	uint16_t checksum;
	mavlink_roll_pitch_yaw_setpoint_t *p = &payload;

	p->time_ms = time_ms; // uint32_t:Timestamp in milliseconds since system boot
	p->roll = roll; // float:Desired roll angle in radians
	p->pitch = pitch; // float:Desired pitch angle in radians
	p->yaw = yaw; // float:Desired yaw angle in radians

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_ROLL_PITCH_YAW_SETPOINT_LEN;
	hdr.msgid = MAVLINK_MSG_ID_ROLL_PITCH_YAW_SETPOINT;
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
// MESSAGE ROLL_PITCH_YAW_SETPOINT UNPACKING

/**
 * @brief Get field time_ms from roll_pitch_yaw_setpoint message
 *
 * @return Timestamp in milliseconds since system boot
 */
static inline uint32_t mavlink_msg_roll_pitch_yaw_setpoint_get_time_ms(const mavlink_message_t* msg)
{
	mavlink_roll_pitch_yaw_setpoint_t *p = (mavlink_roll_pitch_yaw_setpoint_t *)&msg->payload[0];
	return (uint32_t)(p->time_ms);
}

/**
 * @brief Get field roll from roll_pitch_yaw_setpoint message
 *
 * @return Desired roll angle in radians
 */
static inline float mavlink_msg_roll_pitch_yaw_setpoint_get_roll(const mavlink_message_t* msg)
{
	mavlink_roll_pitch_yaw_setpoint_t *p = (mavlink_roll_pitch_yaw_setpoint_t *)&msg->payload[0];
	return (float)(p->roll);
}

/**
 * @brief Get field pitch from roll_pitch_yaw_setpoint message
 *
 * @return Desired pitch angle in radians
 */
static inline float mavlink_msg_roll_pitch_yaw_setpoint_get_pitch(const mavlink_message_t* msg)
{
	mavlink_roll_pitch_yaw_setpoint_t *p = (mavlink_roll_pitch_yaw_setpoint_t *)&msg->payload[0];
	return (float)(p->pitch);
}

/**
 * @brief Get field yaw from roll_pitch_yaw_setpoint message
 *
 * @return Desired yaw angle in radians
 */
static inline float mavlink_msg_roll_pitch_yaw_setpoint_get_yaw(const mavlink_message_t* msg)
{
	mavlink_roll_pitch_yaw_setpoint_t *p = (mavlink_roll_pitch_yaw_setpoint_t *)&msg->payload[0];
	return (float)(p->yaw);
}

/**
 * @brief Decode a roll_pitch_yaw_setpoint message into a struct
 *
 * @param msg The message to decode
 * @param roll_pitch_yaw_setpoint C-struct to decode the message contents into
 */
static inline void mavlink_msg_roll_pitch_yaw_setpoint_decode(const mavlink_message_t* msg, mavlink_roll_pitch_yaw_setpoint_t* roll_pitch_yaw_setpoint)
{
	memcpy( roll_pitch_yaw_setpoint, msg->payload, sizeof(mavlink_roll_pitch_yaw_setpoint_t));
}
