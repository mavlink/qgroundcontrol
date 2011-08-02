// MESSAGE OBS_BIAS PACKING

#define MAVLINK_MSG_ID_OBS_BIAS 180

typedef struct __mavlink_obs_bias_t 
{
	float accBias[3]; ///< accelerometer bias
	float gyroBias[3]; ///< gyroscope bias

} mavlink_obs_bias_t;

#define MAVLINK_MSG_OBS_BIAS_FIELD_ACCBIAS_LEN 3
#define MAVLINK_MSG_OBS_BIAS_FIELD_GYROBIAS_LEN 3


/**
 * @brief Pack a obs_bias message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param accBias accelerometer bias
 * @param gyroBias gyroscope bias
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_obs_bias_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const float* accBias, const float* gyroBias)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_OBS_BIAS;

	i += put_array_by_index((const int8_t*)accBias, sizeof(float)*3, i, msg->payload); // accelerometer bias
	i += put_array_by_index((const int8_t*)gyroBias, sizeof(float)*3, i, msg->payload); // gyroscope bias

	return mavlink_finalize_message(msg, system_id, component_id, i);
}

/**
 * @brief Pack a obs_bias message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param accBias accelerometer bias
 * @param gyroBias gyroscope bias
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_obs_bias_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const float* accBias, const float* gyroBias)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_OBS_BIAS;

	i += put_array_by_index((const int8_t*)accBias, sizeof(float)*3, i, msg->payload); // accelerometer bias
	i += put_array_by_index((const int8_t*)gyroBias, sizeof(float)*3, i, msg->payload); // gyroscope bias

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, i);
}

/**
 * @brief Encode a obs_bias struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param obs_bias C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_obs_bias_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_obs_bias_t* obs_bias)
{
	return mavlink_msg_obs_bias_pack(system_id, component_id, msg, obs_bias->accBias, obs_bias->gyroBias);
}

/**
 * @brief Send a obs_bias message
 * @param chan MAVLink channel to send the message
 *
 * @param accBias accelerometer bias
 * @param gyroBias gyroscope bias
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_obs_bias_send(mavlink_channel_t chan, const float* accBias, const float* gyroBias)
{
	mavlink_message_t msg;
	mavlink_msg_obs_bias_pack_chan(mavlink_system.sysid, mavlink_system.compid, chan, &msg, accBias, gyroBias);
	mavlink_send_uart(chan, &msg);
}

#endif
// MESSAGE OBS_BIAS UNPACKING

/**
 * @brief Get field accBias from obs_bias message
 *
 * @return accelerometer bias
 */
static inline uint16_t mavlink_msg_obs_bias_get_accBias(const mavlink_message_t* msg, float* r_data)
{

	memcpy(r_data, msg->payload, sizeof(float)*3);
	return sizeof(float)*3;
}

/**
 * @brief Get field gyroBias from obs_bias message
 *
 * @return gyroscope bias
 */
static inline uint16_t mavlink_msg_obs_bias_get_gyroBias(const mavlink_message_t* msg, float* r_data)
{

	memcpy(r_data, msg->payload+sizeof(float)*3, sizeof(float)*3);
	return sizeof(float)*3;
}

/**
 * @brief Decode a obs_bias message into a struct
 *
 * @param msg The message to decode
 * @param obs_bias C-struct to decode the message contents into
 */
static inline void mavlink_msg_obs_bias_decode(const mavlink_message_t* msg, mavlink_obs_bias_t* obs_bias)
{
	mavlink_msg_obs_bias_get_accBias(msg, obs_bias->accBias);
	mavlink_msg_obs_bias_get_gyroBias(msg, obs_bias->gyroBias);
}
