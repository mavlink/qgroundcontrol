// MESSAGE STATE_CORRECTION PACKING

#define MAVLINK_MSG_ID_STATE_CORRECTION 64
#define MAVLINK_MSG_ID_STATE_CORRECTION_LEN 36
#define MAVLINK_MSG_64_LEN 36

typedef struct __mavlink_state_correction_t 
{
	float xErr; ///< x position error
	float yErr; ///< y position error
	float zErr; ///< z position error
	float rollErr; ///< roll error (radians)
	float pitchErr; ///< pitch error (radians)
	float yawErr; ///< yaw error (radians)
	float vxErr; ///< x velocity
	float vyErr; ///< y velocity
	float vzErr; ///< z velocity

} mavlink_state_correction_t;

/**
 * @brief Pack a state_correction message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param xErr x position error
 * @param yErr y position error
 * @param zErr z position error
 * @param rollErr roll error (radians)
 * @param pitchErr pitch error (radians)
 * @param yawErr yaw error (radians)
 * @param vxErr x velocity
 * @param vyErr y velocity
 * @param vzErr z velocity
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_state_correction_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, float xErr, float yErr, float zErr, float rollErr, float pitchErr, float yawErr, float vxErr, float vyErr, float vzErr)
{
	mavlink_state_correction_t *p = (mavlink_state_correction_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_STATE_CORRECTION;

	p->xErr = xErr; // float:x position error
	p->yErr = yErr; // float:y position error
	p->zErr = zErr; // float:z position error
	p->rollErr = rollErr; // float:roll error (radians)
	p->pitchErr = pitchErr; // float:pitch error (radians)
	p->yawErr = yawErr; // float:yaw error (radians)
	p->vxErr = vxErr; // float:x velocity
	p->vyErr = vyErr; // float:y velocity
	p->vzErr = vzErr; // float:z velocity

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_STATE_CORRECTION_LEN);
}

/**
 * @brief Pack a state_correction message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param xErr x position error
 * @param yErr y position error
 * @param zErr z position error
 * @param rollErr roll error (radians)
 * @param pitchErr pitch error (radians)
 * @param yawErr yaw error (radians)
 * @param vxErr x velocity
 * @param vyErr y velocity
 * @param vzErr z velocity
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_state_correction_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, float xErr, float yErr, float zErr, float rollErr, float pitchErr, float yawErr, float vxErr, float vyErr, float vzErr)
{
	mavlink_state_correction_t *p = (mavlink_state_correction_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_STATE_CORRECTION;

	p->xErr = xErr; // float:x position error
	p->yErr = yErr; // float:y position error
	p->zErr = zErr; // float:z position error
	p->rollErr = rollErr; // float:roll error (radians)
	p->pitchErr = pitchErr; // float:pitch error (radians)
	p->yawErr = yawErr; // float:yaw error (radians)
	p->vxErr = vxErr; // float:x velocity
	p->vyErr = vyErr; // float:y velocity
	p->vzErr = vzErr; // float:z velocity

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_STATE_CORRECTION_LEN);
}

/**
 * @brief Encode a state_correction struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param state_correction C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_state_correction_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_state_correction_t* state_correction)
{
	return mavlink_msg_state_correction_pack(system_id, component_id, msg, state_correction->xErr, state_correction->yErr, state_correction->zErr, state_correction->rollErr, state_correction->pitchErr, state_correction->yawErr, state_correction->vxErr, state_correction->vyErr, state_correction->vzErr);
}

/**
 * @brief Send a state_correction message
 * @param chan MAVLink channel to send the message
 *
 * @param xErr x position error
 * @param yErr y position error
 * @param zErr z position error
 * @param rollErr roll error (radians)
 * @param pitchErr pitch error (radians)
 * @param yawErr yaw error (radians)
 * @param vxErr x velocity
 * @param vyErr y velocity
 * @param vzErr z velocity
 */


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_state_correction_send(mavlink_channel_t chan, float xErr, float yErr, float zErr, float rollErr, float pitchErr, float yawErr, float vxErr, float vyErr, float vzErr)
{
	mavlink_header_t hdr;
	mavlink_state_correction_t payload;
	uint16_t checksum;
	mavlink_state_correction_t *p = &payload;

	p->xErr = xErr; // float:x position error
	p->yErr = yErr; // float:y position error
	p->zErr = zErr; // float:z position error
	p->rollErr = rollErr; // float:roll error (radians)
	p->pitchErr = pitchErr; // float:pitch error (radians)
	p->yawErr = yawErr; // float:yaw error (radians)
	p->vxErr = vxErr; // float:x velocity
	p->vyErr = vyErr; // float:y velocity
	p->vzErr = vzErr; // float:z velocity

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_STATE_CORRECTION_LEN;
	hdr.msgid = MAVLINK_MSG_ID_STATE_CORRECTION;
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
// MESSAGE STATE_CORRECTION UNPACKING

/**
 * @brief Get field xErr from state_correction message
 *
 * @return x position error
 */
static inline float mavlink_msg_state_correction_get_xErr(const mavlink_message_t* msg)
{
	mavlink_state_correction_t *p = (mavlink_state_correction_t *)&msg->payload[0];
	return (float)(p->xErr);
}

/**
 * @brief Get field yErr from state_correction message
 *
 * @return y position error
 */
static inline float mavlink_msg_state_correction_get_yErr(const mavlink_message_t* msg)
{
	mavlink_state_correction_t *p = (mavlink_state_correction_t *)&msg->payload[0];
	return (float)(p->yErr);
}

/**
 * @brief Get field zErr from state_correction message
 *
 * @return z position error
 */
static inline float mavlink_msg_state_correction_get_zErr(const mavlink_message_t* msg)
{
	mavlink_state_correction_t *p = (mavlink_state_correction_t *)&msg->payload[0];
	return (float)(p->zErr);
}

/**
 * @brief Get field rollErr from state_correction message
 *
 * @return roll error (radians)
 */
static inline float mavlink_msg_state_correction_get_rollErr(const mavlink_message_t* msg)
{
	mavlink_state_correction_t *p = (mavlink_state_correction_t *)&msg->payload[0];
	return (float)(p->rollErr);
}

/**
 * @brief Get field pitchErr from state_correction message
 *
 * @return pitch error (radians)
 */
static inline float mavlink_msg_state_correction_get_pitchErr(const mavlink_message_t* msg)
{
	mavlink_state_correction_t *p = (mavlink_state_correction_t *)&msg->payload[0];
	return (float)(p->pitchErr);
}

/**
 * @brief Get field yawErr from state_correction message
 *
 * @return yaw error (radians)
 */
static inline float mavlink_msg_state_correction_get_yawErr(const mavlink_message_t* msg)
{
	mavlink_state_correction_t *p = (mavlink_state_correction_t *)&msg->payload[0];
	return (float)(p->yawErr);
}

/**
 * @brief Get field vxErr from state_correction message
 *
 * @return x velocity
 */
static inline float mavlink_msg_state_correction_get_vxErr(const mavlink_message_t* msg)
{
	mavlink_state_correction_t *p = (mavlink_state_correction_t *)&msg->payload[0];
	return (float)(p->vxErr);
}

/**
 * @brief Get field vyErr from state_correction message
 *
 * @return y velocity
 */
static inline float mavlink_msg_state_correction_get_vyErr(const mavlink_message_t* msg)
{
	mavlink_state_correction_t *p = (mavlink_state_correction_t *)&msg->payload[0];
	return (float)(p->vyErr);
}

/**
 * @brief Get field vzErr from state_correction message
 *
 * @return z velocity
 */
static inline float mavlink_msg_state_correction_get_vzErr(const mavlink_message_t* msg)
{
	mavlink_state_correction_t *p = (mavlink_state_correction_t *)&msg->payload[0];
	return (float)(p->vzErr);
}

/**
 * @brief Decode a state_correction message into a struct
 *
 * @param msg The message to decode
 * @param state_correction C-struct to decode the message contents into
 */
static inline void mavlink_msg_state_correction_decode(const mavlink_message_t* msg, mavlink_state_correction_t* state_correction)
{
	memcpy( state_correction, msg->payload, sizeof(mavlink_state_correction_t));
}
