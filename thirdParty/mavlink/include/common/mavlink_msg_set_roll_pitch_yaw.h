// MESSAGE SET_ROLL_PITCH_YAW PACKING

#define MAVLINK_MSG_ID_SET_ROLL_PITCH_YAW 55
#define MAVLINK_MSG_ID_SET_ROLL_PITCH_YAW_LEN 14
#define MAVLINK_MSG_55_LEN 14

typedef struct __mavlink_set_roll_pitch_yaw_t 
{
	float roll; ///< Desired roll angle in radians
	float pitch; ///< Desired pitch angle in radians
	float yaw; ///< Desired yaw angle in radians
	uint8_t target_system; ///< System ID
	uint8_t target_component; ///< Component ID

} mavlink_set_roll_pitch_yaw_t;

/**
 * @brief Pack a set_roll_pitch_yaw message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param roll Desired roll angle in radians
 * @param pitch Desired pitch angle in radians
 * @param yaw Desired yaw angle in radians
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_set_roll_pitch_yaw_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component, float roll, float pitch, float yaw)
{
	mavlink_set_roll_pitch_yaw_t *p = (mavlink_set_roll_pitch_yaw_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SET_ROLL_PITCH_YAW;

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	p->roll = roll; // float:Desired roll angle in radians
	p->pitch = pitch; // float:Desired pitch angle in radians
	p->yaw = yaw; // float:Desired yaw angle in radians

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_SET_ROLL_PITCH_YAW_LEN);
}

/**
 * @brief Pack a set_roll_pitch_yaw message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system System ID
 * @param target_component Component ID
 * @param roll Desired roll angle in radians
 * @param pitch Desired pitch angle in radians
 * @param yaw Desired yaw angle in radians
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_set_roll_pitch_yaw_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component, float roll, float pitch, float yaw)
{
	mavlink_set_roll_pitch_yaw_t *p = (mavlink_set_roll_pitch_yaw_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SET_ROLL_PITCH_YAW;

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	p->roll = roll; // float:Desired roll angle in radians
	p->pitch = pitch; // float:Desired pitch angle in radians
	p->yaw = yaw; // float:Desired yaw angle in radians

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_SET_ROLL_PITCH_YAW_LEN);
}

/**
 * @brief Encode a set_roll_pitch_yaw struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param set_roll_pitch_yaw C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_set_roll_pitch_yaw_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_set_roll_pitch_yaw_t* set_roll_pitch_yaw)
{
	return mavlink_msg_set_roll_pitch_yaw_pack(system_id, component_id, msg, set_roll_pitch_yaw->target_system, set_roll_pitch_yaw->target_component, set_roll_pitch_yaw->roll, set_roll_pitch_yaw->pitch, set_roll_pitch_yaw->yaw);
}

/**
 * @brief Send a set_roll_pitch_yaw message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param roll Desired roll angle in radians
 * @param pitch Desired pitch angle in radians
 * @param yaw Desired yaw angle in radians
 */


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_set_roll_pitch_yaw_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, float roll, float pitch, float yaw)
{
	mavlink_header_t hdr;
	mavlink_set_roll_pitch_yaw_t payload;
	uint16_t checksum;
	mavlink_set_roll_pitch_yaw_t *p = &payload;

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	p->roll = roll; // float:Desired roll angle in radians
	p->pitch = pitch; // float:Desired pitch angle in radians
	p->yaw = yaw; // float:Desired yaw angle in radians

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_SET_ROLL_PITCH_YAW_LEN;
	hdr.msgid = MAVLINK_MSG_ID_SET_ROLL_PITCH_YAW;
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
// MESSAGE SET_ROLL_PITCH_YAW UNPACKING

/**
 * @brief Get field target_system from set_roll_pitch_yaw message
 *
 * @return System ID
 */
static inline uint8_t mavlink_msg_set_roll_pitch_yaw_get_target_system(const mavlink_message_t* msg)
{
	mavlink_set_roll_pitch_yaw_t *p = (mavlink_set_roll_pitch_yaw_t *)&msg->payload[0];
	return (uint8_t)(p->target_system);
}

/**
 * @brief Get field target_component from set_roll_pitch_yaw message
 *
 * @return Component ID
 */
static inline uint8_t mavlink_msg_set_roll_pitch_yaw_get_target_component(const mavlink_message_t* msg)
{
	mavlink_set_roll_pitch_yaw_t *p = (mavlink_set_roll_pitch_yaw_t *)&msg->payload[0];
	return (uint8_t)(p->target_component);
}

/**
 * @brief Get field roll from set_roll_pitch_yaw message
 *
 * @return Desired roll angle in radians
 */
static inline float mavlink_msg_set_roll_pitch_yaw_get_roll(const mavlink_message_t* msg)
{
	mavlink_set_roll_pitch_yaw_t *p = (mavlink_set_roll_pitch_yaw_t *)&msg->payload[0];
	return (float)(p->roll);
}

/**
 * @brief Get field pitch from set_roll_pitch_yaw message
 *
 * @return Desired pitch angle in radians
 */
static inline float mavlink_msg_set_roll_pitch_yaw_get_pitch(const mavlink_message_t* msg)
{
	mavlink_set_roll_pitch_yaw_t *p = (mavlink_set_roll_pitch_yaw_t *)&msg->payload[0];
	return (float)(p->pitch);
}

/**
 * @brief Get field yaw from set_roll_pitch_yaw message
 *
 * @return Desired yaw angle in radians
 */
static inline float mavlink_msg_set_roll_pitch_yaw_get_yaw(const mavlink_message_t* msg)
{
	mavlink_set_roll_pitch_yaw_t *p = (mavlink_set_roll_pitch_yaw_t *)&msg->payload[0];
	return (float)(p->yaw);
}

/**
 * @brief Decode a set_roll_pitch_yaw message into a struct
 *
 * @param msg The message to decode
 * @param set_roll_pitch_yaw C-struct to decode the message contents into
 */
static inline void mavlink_msg_set_roll_pitch_yaw_decode(const mavlink_message_t* msg, mavlink_set_roll_pitch_yaw_t* set_roll_pitch_yaw)
{
	memcpy( set_roll_pitch_yaw, msg->payload, sizeof(mavlink_set_roll_pitch_yaw_t));
}
