// MESSAGE OBS_ATTITUDE PACKING

#define MAVLINK_MSG_ID_OBS_ATTITUDE 174

typedef struct __mavlink_obs_attitude_t 
{
	float quat[4]; ///< Quaternion re;im

} mavlink_obs_attitude_t;

#define MAVLINK_MSG_OBS_ATTITUDE_FIELD_QUAT_LEN 4


/**
 * @brief Pack a obs_attitude message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param quat Quaternion re;im
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_obs_attitude_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const float* quat)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_OBS_ATTITUDE;

	i += put_array_by_index((const int8_t*)quat, sizeof(float)*4, i, msg->payload); // Quaternion re;im

	return mavlink_finalize_message(msg, system_id, component_id, i);
}

/**
 * @brief Pack a obs_attitude message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param quat Quaternion re;im
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_obs_attitude_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const float* quat)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_OBS_ATTITUDE;

	i += put_array_by_index((const int8_t*)quat, sizeof(float)*4, i, msg->payload); // Quaternion re;im

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, i);
}

/**
 * @brief Encode a obs_attitude struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param obs_attitude C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_obs_attitude_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_obs_attitude_t* obs_attitude)
{
	return mavlink_msg_obs_attitude_pack(system_id, component_id, msg, obs_attitude->quat);
}

/**
 * @brief Send a obs_attitude message
 * @param chan MAVLink channel to send the message
 *
 * @param quat Quaternion re;im
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_obs_attitude_send(mavlink_channel_t chan, const float* quat)
{
	mavlink_message_t msg;
	mavlink_msg_obs_attitude_pack_chan(mavlink_system.sysid, mavlink_system.compid, chan, &msg, quat);
	mavlink_send_uart(chan, &msg);
}

#endif
// MESSAGE OBS_ATTITUDE UNPACKING

/**
 * @brief Get field quat from obs_attitude message
 *
 * @return Quaternion re;im
 */
static inline uint16_t mavlink_msg_obs_attitude_get_quat(const mavlink_message_t* msg, float* r_data)
{

	memcpy(r_data, msg->payload, sizeof(float)*4);
	return sizeof(float)*4;
}

/**
 * @brief Decode a obs_attitude message into a struct
 *
 * @param msg The message to decode
 * @param obs_attitude C-struct to decode the message contents into
 */
static inline void mavlink_msg_obs_attitude_decode(const mavlink_message_t* msg, mavlink_obs_attitude_t* obs_attitude)
{
	mavlink_msg_obs_attitude_get_quat(msg, obs_attitude->quat);
}
