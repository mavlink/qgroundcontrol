// MESSAGE OBS_WIND PACKING

#define MAVLINK_MSG_ID_OBS_WIND 176

typedef struct __mavlink_obs_wind_t 
{
	float wind[3]; ///< Wind

} mavlink_obs_wind_t;

#define MAVLINK_MSG_OBS_WIND_FIELD_WIND_LEN 3


/**
 * @brief Pack a obs_wind message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param wind Wind
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_obs_wind_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const float* wind)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_OBS_WIND;

	i += put_array_by_index((const int8_t*)wind, sizeof(float)*3, i, msg->payload); // Wind

	return mavlink_finalize_message(msg, system_id, component_id, i);
}

/**
 * @brief Pack a obs_wind message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param wind Wind
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_obs_wind_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const float* wind)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_OBS_WIND;

	i += put_array_by_index((const int8_t*)wind, sizeof(float)*3, i, msg->payload); // Wind

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, i);
}

/**
 * @brief Encode a obs_wind struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param obs_wind C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_obs_wind_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_obs_wind_t* obs_wind)
{
	return mavlink_msg_obs_wind_pack(system_id, component_id, msg, obs_wind->wind);
}

/**
 * @brief Send a obs_wind message
 * @param chan MAVLink channel to send the message
 *
 * @param wind Wind
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_obs_wind_send(mavlink_channel_t chan, const float* wind)
{
	mavlink_message_t msg;
	mavlink_msg_obs_wind_pack_chan(mavlink_system.sysid, mavlink_system.compid, chan, &msg, wind);
	mavlink_send_uart(chan, &msg);
}

#endif
// MESSAGE OBS_WIND UNPACKING

/**
 * @brief Get field wind from obs_wind message
 *
 * @return Wind
 */
static inline uint16_t mavlink_msg_obs_wind_get_wind(const mavlink_message_t* msg, float* r_data)
{

	memcpy(r_data, msg->payload, sizeof(float)*3);
	return sizeof(float)*3;
}

/**
 * @brief Decode a obs_wind message into a struct
 *
 * @param msg The message to decode
 * @param obs_wind C-struct to decode the message contents into
 */
static inline void mavlink_msg_obs_wind_decode(const mavlink_message_t* msg, mavlink_obs_wind_t* obs_wind)
{
	mavlink_msg_obs_wind_get_wind(msg, obs_wind->wind);
}
