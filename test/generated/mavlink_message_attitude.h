// MESSAGE ATTITUDE PACKING

#define MESSAGE_ID_ATTITUDE 90

/**
 * @brief Send a attitude message
 *
 * @param roll Roll angle (rad)
 * @param pitch Pitch angle (rad)
 * @param yaw Yaw angle (rad)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t message_attitude_pack(uint8_t system_id, CommMessage_t* msg, float roll, float pitch, float yaw)
{
	msg->msgid = MESSAGE_ID_ATTITUDE;
	uint16_t i = 0;

	i += put_float_by_index(roll, i, msg->payload); //Roll angle (rad)
	i += put_float_by_index(pitch, i, msg->payload); //Pitch angle (rad)
	i += put_float_by_index(yaw, i, msg->payload); //Yaw angle (rad)

	return finalize_message(msg, system_id, i);
}

// MESSAGE ATTITUDE UNPACKING

/**
 * @brief Get field roll from attitude message
 *
 * @return Roll angle (rad)
 */
static inline float message_attitude_get_roll(CommMessage_t* msg)
{
	return *((float*) (void*)msg->payload);
}

/**
 * @brief Get field pitch from attitude message
 *
 * @return Pitch angle (rad)
 */
static inline float message_attitude_get_pitch(CommMessage_t* msg)
{
	return *((float*) (void*)msg->payload+sizeof(float));
}

/**
 * @brief Get field yaw from attitude message
 *
 * @return Yaw angle (rad)
 */
static inline float message_attitude_get_yaw(CommMessage_t* msg)
{
	return *((float*) (void*)msg->payload+sizeof(float)+sizeof(float));
}

