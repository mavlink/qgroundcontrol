// MESSAGE OBS_AIR_VELOCITY PACKING

#define MAVLINK_MSG_ID_OBS_AIR_VELOCITY 178

typedef struct __mavlink_obs_air_velocity_t 
{
	float magnitude; ///< Air speed
	float aoa; ///< angle of attack
	float slip; ///< slip angle

} mavlink_obs_air_velocity_t;



/**
 * @brief Pack a obs_air_velocity message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param magnitude Air speed
 * @param aoa angle of attack
 * @param slip slip angle
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_obs_air_velocity_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, float magnitude, float aoa, float slip)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_OBS_AIR_VELOCITY;

	i += put_float_by_index(magnitude, i, msg->payload); // Air speed
	i += put_float_by_index(aoa, i, msg->payload); // angle of attack
	i += put_float_by_index(slip, i, msg->payload); // slip angle

	return mavlink_finalize_message(msg, system_id, component_id, i);
}

/**
 * @brief Pack a obs_air_velocity message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param magnitude Air speed
 * @param aoa angle of attack
 * @param slip slip angle
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_obs_air_velocity_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, float magnitude, float aoa, float slip)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_OBS_AIR_VELOCITY;

	i += put_float_by_index(magnitude, i, msg->payload); // Air speed
	i += put_float_by_index(aoa, i, msg->payload); // angle of attack
	i += put_float_by_index(slip, i, msg->payload); // slip angle

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, i);
}

/**
 * @brief Encode a obs_air_velocity struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param obs_air_velocity C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_obs_air_velocity_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_obs_air_velocity_t* obs_air_velocity)
{
	return mavlink_msg_obs_air_velocity_pack(system_id, component_id, msg, obs_air_velocity->magnitude, obs_air_velocity->aoa, obs_air_velocity->slip);
}

/**
 * @brief Send a obs_air_velocity message
 * @param chan MAVLink channel to send the message
 *
 * @param magnitude Air speed
 * @param aoa angle of attack
 * @param slip slip angle
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_obs_air_velocity_send(mavlink_channel_t chan, float magnitude, float aoa, float slip)
{
	mavlink_message_t msg;
	mavlink_msg_obs_air_velocity_pack_chan(mavlink_system.sysid, mavlink_system.compid, chan, &msg, magnitude, aoa, slip);
	mavlink_send_uart(chan, &msg);
}

#endif
// MESSAGE OBS_AIR_VELOCITY UNPACKING

/**
 * @brief Get field magnitude from obs_air_velocity message
 *
 * @return Air speed
 */
static inline float mavlink_msg_obs_air_velocity_get_magnitude(const mavlink_message_t* msg)
{
	generic_32bit r;
	r.b[3] = (msg->payload)[0];
	r.b[2] = (msg->payload)[1];
	r.b[1] = (msg->payload)[2];
	r.b[0] = (msg->payload)[3];
	return (float)r.f;
}

/**
 * @brief Get field aoa from obs_air_velocity message
 *
 * @return angle of attack
 */
static inline float mavlink_msg_obs_air_velocity_get_aoa(const mavlink_message_t* msg)
{
	generic_32bit r;
	r.b[3] = (msg->payload+sizeof(float))[0];
	r.b[2] = (msg->payload+sizeof(float))[1];
	r.b[1] = (msg->payload+sizeof(float))[2];
	r.b[0] = (msg->payload+sizeof(float))[3];
	return (float)r.f;
}

/**
 * @brief Get field slip from obs_air_velocity message
 *
 * @return slip angle
 */
static inline float mavlink_msg_obs_air_velocity_get_slip(const mavlink_message_t* msg)
{
	generic_32bit r;
	r.b[3] = (msg->payload+sizeof(float)+sizeof(float))[0];
	r.b[2] = (msg->payload+sizeof(float)+sizeof(float))[1];
	r.b[1] = (msg->payload+sizeof(float)+sizeof(float))[2];
	r.b[0] = (msg->payload+sizeof(float)+sizeof(float))[3];
	return (float)r.f;
}

/**
 * @brief Decode a obs_air_velocity message into a struct
 *
 * @param msg The message to decode
 * @param obs_air_velocity C-struct to decode the message contents into
 */
static inline void mavlink_msg_obs_air_velocity_decode(const mavlink_message_t* msg, mavlink_obs_air_velocity_t* obs_air_velocity)
{
	obs_air_velocity->magnitude = mavlink_msg_obs_air_velocity_get_magnitude(msg);
	obs_air_velocity->aoa = mavlink_msg_obs_air_velocity_get_aoa(msg);
	obs_air_velocity->slip = mavlink_msg_obs_air_velocity_get_slip(msg);
}
