// MESSAGE ATTITUDE_CONTROL PACKING

#define MAVLINK_MSG_ID_ATTITUDE_CONTROL 85
#define MAVLINK_MSG_ID_ATTITUDE_CONTROL_LEN 21
#define MAVLINK_MSG_85_LEN 21

typedef struct __mavlink_attitude_control_t 
{
	float roll; ///< roll
	float pitch; ///< pitch
	float yaw; ///< yaw
	float thrust; ///< thrust
	uint8_t target; ///< The system to be controlled
	uint8_t roll_manual; ///< roll control enabled auto:0, manual:1
	uint8_t pitch_manual; ///< pitch auto:0, manual:1
	uint8_t yaw_manual; ///< yaw auto:0, manual:1
	uint8_t thrust_manual; ///< thrust auto:0, manual:1

} mavlink_attitude_control_t;

/**
 * @brief Pack a attitude_control message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target The system to be controlled
 * @param roll roll
 * @param pitch pitch
 * @param yaw yaw
 * @param thrust thrust
 * @param roll_manual roll control enabled auto:0, manual:1
 * @param pitch_manual pitch auto:0, manual:1
 * @param yaw_manual yaw auto:0, manual:1
 * @param thrust_manual thrust auto:0, manual:1
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_attitude_control_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t target, float roll, float pitch, float yaw, float thrust, uint8_t roll_manual, uint8_t pitch_manual, uint8_t yaw_manual, uint8_t thrust_manual)
{
	mavlink_attitude_control_t *p = (mavlink_attitude_control_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_ATTITUDE_CONTROL;

	p->target = target; // uint8_t:The system to be controlled
	p->roll = roll; // float:roll
	p->pitch = pitch; // float:pitch
	p->yaw = yaw; // float:yaw
	p->thrust = thrust; // float:thrust
	p->roll_manual = roll_manual; // uint8_t:roll control enabled auto:0, manual:1
	p->pitch_manual = pitch_manual; // uint8_t:pitch auto:0, manual:1
	p->yaw_manual = yaw_manual; // uint8_t:yaw auto:0, manual:1
	p->thrust_manual = thrust_manual; // uint8_t:thrust auto:0, manual:1

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_ATTITUDE_CONTROL_LEN);
}

/**
 * @brief Pack a attitude_control message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target The system to be controlled
 * @param roll roll
 * @param pitch pitch
 * @param yaw yaw
 * @param thrust thrust
 * @param roll_manual roll control enabled auto:0, manual:1
 * @param pitch_manual pitch auto:0, manual:1
 * @param yaw_manual yaw auto:0, manual:1
 * @param thrust_manual thrust auto:0, manual:1
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_attitude_control_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t target, float roll, float pitch, float yaw, float thrust, uint8_t roll_manual, uint8_t pitch_manual, uint8_t yaw_manual, uint8_t thrust_manual)
{
	mavlink_attitude_control_t *p = (mavlink_attitude_control_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_ATTITUDE_CONTROL;

	p->target = target; // uint8_t:The system to be controlled
	p->roll = roll; // float:roll
	p->pitch = pitch; // float:pitch
	p->yaw = yaw; // float:yaw
	p->thrust = thrust; // float:thrust
	p->roll_manual = roll_manual; // uint8_t:roll control enabled auto:0, manual:1
	p->pitch_manual = pitch_manual; // uint8_t:pitch auto:0, manual:1
	p->yaw_manual = yaw_manual; // uint8_t:yaw auto:0, manual:1
	p->thrust_manual = thrust_manual; // uint8_t:thrust auto:0, manual:1

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_ATTITUDE_CONTROL_LEN);
}

/**
 * @brief Encode a attitude_control struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param attitude_control C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_attitude_control_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_attitude_control_t* attitude_control)
{
	return mavlink_msg_attitude_control_pack(system_id, component_id, msg, attitude_control->target, attitude_control->roll, attitude_control->pitch, attitude_control->yaw, attitude_control->thrust, attitude_control->roll_manual, attitude_control->pitch_manual, attitude_control->yaw_manual, attitude_control->thrust_manual);
}

/**
 * @brief Send a attitude_control message
 * @param chan MAVLink channel to send the message
 *
 * @param target The system to be controlled
 * @param roll roll
 * @param pitch pitch
 * @param yaw yaw
 * @param thrust thrust
 * @param roll_manual roll control enabled auto:0, manual:1
 * @param pitch_manual pitch auto:0, manual:1
 * @param yaw_manual yaw auto:0, manual:1
 * @param thrust_manual thrust auto:0, manual:1
 */


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_attitude_control_send(mavlink_channel_t chan, uint8_t target, float roll, float pitch, float yaw, float thrust, uint8_t roll_manual, uint8_t pitch_manual, uint8_t yaw_manual, uint8_t thrust_manual)
{
	mavlink_header_t hdr;
	mavlink_attitude_control_t payload;
	uint16_t checksum;
	mavlink_attitude_control_t *p = &payload;

	p->target = target; // uint8_t:The system to be controlled
	p->roll = roll; // float:roll
	p->pitch = pitch; // float:pitch
	p->yaw = yaw; // float:yaw
	p->thrust = thrust; // float:thrust
	p->roll_manual = roll_manual; // uint8_t:roll control enabled auto:0, manual:1
	p->pitch_manual = pitch_manual; // uint8_t:pitch auto:0, manual:1
	p->yaw_manual = yaw_manual; // uint8_t:yaw auto:0, manual:1
	p->thrust_manual = thrust_manual; // uint8_t:thrust auto:0, manual:1

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_ATTITUDE_CONTROL_LEN;
	hdr.msgid = MAVLINK_MSG_ID_ATTITUDE_CONTROL;
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
// MESSAGE ATTITUDE_CONTROL UNPACKING

/**
 * @brief Get field target from attitude_control message
 *
 * @return The system to be controlled
 */
static inline uint8_t mavlink_msg_attitude_control_get_target(const mavlink_message_t* msg)
{
	mavlink_attitude_control_t *p = (mavlink_attitude_control_t *)&msg->payload[0];
	return (uint8_t)(p->target);
}

/**
 * @brief Get field roll from attitude_control message
 *
 * @return roll
 */
static inline float mavlink_msg_attitude_control_get_roll(const mavlink_message_t* msg)
{
	mavlink_attitude_control_t *p = (mavlink_attitude_control_t *)&msg->payload[0];
	return (float)(p->roll);
}

/**
 * @brief Get field pitch from attitude_control message
 *
 * @return pitch
 */
static inline float mavlink_msg_attitude_control_get_pitch(const mavlink_message_t* msg)
{
	mavlink_attitude_control_t *p = (mavlink_attitude_control_t *)&msg->payload[0];
	return (float)(p->pitch);
}

/**
 * @brief Get field yaw from attitude_control message
 *
 * @return yaw
 */
static inline float mavlink_msg_attitude_control_get_yaw(const mavlink_message_t* msg)
{
	mavlink_attitude_control_t *p = (mavlink_attitude_control_t *)&msg->payload[0];
	return (float)(p->yaw);
}

/**
 * @brief Get field thrust from attitude_control message
 *
 * @return thrust
 */
static inline float mavlink_msg_attitude_control_get_thrust(const mavlink_message_t* msg)
{
	mavlink_attitude_control_t *p = (mavlink_attitude_control_t *)&msg->payload[0];
	return (float)(p->thrust);
}

/**
 * @brief Get field roll_manual from attitude_control message
 *
 * @return roll control enabled auto:0, manual:1
 */
static inline uint8_t mavlink_msg_attitude_control_get_roll_manual(const mavlink_message_t* msg)
{
	mavlink_attitude_control_t *p = (mavlink_attitude_control_t *)&msg->payload[0];
	return (uint8_t)(p->roll_manual);
}

/**
 * @brief Get field pitch_manual from attitude_control message
 *
 * @return pitch auto:0, manual:1
 */
static inline uint8_t mavlink_msg_attitude_control_get_pitch_manual(const mavlink_message_t* msg)
{
	mavlink_attitude_control_t *p = (mavlink_attitude_control_t *)&msg->payload[0];
	return (uint8_t)(p->pitch_manual);
}

/**
 * @brief Get field yaw_manual from attitude_control message
 *
 * @return yaw auto:0, manual:1
 */
static inline uint8_t mavlink_msg_attitude_control_get_yaw_manual(const mavlink_message_t* msg)
{
	mavlink_attitude_control_t *p = (mavlink_attitude_control_t *)&msg->payload[0];
	return (uint8_t)(p->yaw_manual);
}

/**
 * @brief Get field thrust_manual from attitude_control message
 *
 * @return thrust auto:0, manual:1
 */
static inline uint8_t mavlink_msg_attitude_control_get_thrust_manual(const mavlink_message_t* msg)
{
	mavlink_attitude_control_t *p = (mavlink_attitude_control_t *)&msg->payload[0];
	return (uint8_t)(p->thrust_manual);
}

/**
 * @brief Decode a attitude_control message into a struct
 *
 * @param msg The message to decode
 * @param attitude_control C-struct to decode the message contents into
 */
static inline void mavlink_msg_attitude_control_decode(const mavlink_message_t* msg, mavlink_attitude_control_t* attitude_control)
{
	memcpy( attitude_control, msg->payload, sizeof(mavlink_attitude_control_t));
}
