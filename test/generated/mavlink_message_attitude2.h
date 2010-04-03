// MESSAGE ATTITUDE2 PACKING

#define MESSAGE_ID_ATTITUDE2 90

typedef struct __attitude2_t 
{
	float roll; ///< Roll angle (rad)
	float pitch; ///< Pitch angle (rad)
	float yaw; ///< Yaw angle (rad)

} attitude2_t;

/**
 * @brief Send a attitude2 message
 *
 * @param roll Roll angle (rad)
 * @param pitch Pitch angle (rad)
 * @param yaw Yaw angle (rad)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t message_attitude2_pack(uint8_t system_id, CommMessage_t* msg, float roll, float pitch, float yaw)
{
	msg->msgid = MESSAGE_ID_ATTITUDE2;
	uint16_t i = 0;

	i += put_float_by_index(roll, i, msg->payload); //Roll angle (rad)
	i += put_float_by_index(pitch, i, msg->payload); //Pitch angle (rad)
	i += put_float_by_index(yaw, i, msg->payload); //Yaw angle (rad)

	return finalize_message(msg, system_id, i);
}

static inline uint16_t message_attitude2_encode(uint8_t system_id, CommMessage_t* msg, const attitude2_t* attitude2)
{
	message_attitude2_pack(system_id, msg, attitude2->roll, attitude2->pitch, attitude2->yaw);
}
// MESSAGE ATTITUDE2 UNPACKING

/**
 * @brief Get field roll from attitude2 message
 *
 * @return Roll angle (rad)
 */
static inline float message_attitude2_get_roll(CommMessage_t* msg)
{
	generic_32bit r;
	r.b[3] = (msg->payload)[0];
	r.b[2] = (msg->payload)[1];
	r.b[1] = (msg->payload)[2];
	r.b[0] = (msg->payload)[3];
	return (float)r.f;
}

/**
 * @brief Get field pitch from attitude2 message
 *
 * @return Pitch angle (rad)
 */
static inline float message_attitude2_get_pitch(CommMessage_t* msg)
{
	generic_32bit r;
	r.b[3] = (msg->payload+sizeof(float))[0];
	r.b[2] = (msg->payload+sizeof(float))[1];
	r.b[1] = (msg->payload+sizeof(float))[2];
	r.b[0] = (msg->payload+sizeof(float))[3];
	return (float)r.f;
}

/**
 * @brief Get field yaw from attitude2 message
 *
 * @return Yaw angle (rad)
 */
static inline float message_attitude2_get_yaw(CommMessage_t* msg)
{
	generic_32bit r;
	r.b[3] = (msg->payload+sizeof(float)+sizeof(float))[0];
	r.b[2] = (msg->payload+sizeof(float)+sizeof(float))[1];
	r.b[1] = (msg->payload+sizeof(float)+sizeof(float))[2];
	r.b[0] = (msg->payload+sizeof(float)+sizeof(float))[3];
	return (float)r.f;
}

static inline void message_attitude2_decode(CommMessage_t* msg, attitude2_t* attitude2)
{
	attitude2->roll = message_attitude2_get_roll(msg);
	attitude2->pitch = message_attitude2_get_pitch(msg);
	attitude2->yaw = message_attitude2_get_yaw(msg);
}
