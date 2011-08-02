// MESSAGE OBS_VELOCITY PACKING

#define MAVLINK_MSG_ID_OBS_VELOCITY 172

typedef struct __mavlink_obs_velocity_t 
{
	float vel[3]; ///< Velocity

} mavlink_obs_velocity_t;

#define MAVLINK_MSG_OBS_VELOCITY_FIELD_VEL_LEN 3


/**
 * @brief Pack a obs_velocity message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param vel Velocity
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_obs_velocity_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const float* vel)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_OBS_VELOCITY;

	i += put_array_by_index((const int8_t*)vel, sizeof(float)*3, i, msg->payload); // Velocity

	return mavlink_finalize_message(msg, system_id, component_id, i);
}

/**
 * @brief Pack a obs_velocity message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param vel Velocity
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_obs_velocity_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const float* vel)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_OBS_VELOCITY;

	i += put_array_by_index((const int8_t*)vel, sizeof(float)*3, i, msg->payload); // Velocity

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, i);
}

/**
 * @brief Encode a obs_velocity struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param obs_velocity C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_obs_velocity_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_obs_velocity_t* obs_velocity)
{
	return mavlink_msg_obs_velocity_pack(system_id, component_id, msg, obs_velocity->vel);
}

/**
 * @brief Send a obs_velocity message
 * @param chan MAVLink channel to send the message
 *
 * @param vel Velocity
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_obs_velocity_send(mavlink_channel_t chan, const float* vel)
{
	mavlink_message_t msg;
	mavlink_msg_obs_velocity_pack_chan(mavlink_system.sysid, mavlink_system.compid, chan, &msg, vel);
	mavlink_send_uart(chan, &msg);
}

#endif
// MESSAGE OBS_VELOCITY UNPACKING

/**
 * @brief Get field vel from obs_velocity message
 *
 * @return Velocity
 */
static inline uint16_t mavlink_msg_obs_velocity_get_vel(const mavlink_message_t* msg, float* r_data)
{

	memcpy(r_data, msg->payload, sizeof(float)*3);
	return sizeof(float)*3;
}

/**
 * @brief Decode a obs_velocity message into a struct
 *
 * @param msg The message to decode
 * @param obs_velocity C-struct to decode the message contents into
 */
static inline void mavlink_msg_obs_velocity_decode(const mavlink_message_t* msg, mavlink_obs_velocity_t* obs_velocity)
{
	mavlink_msg_obs_velocity_get_vel(msg, obs_velocity->vel);
}
