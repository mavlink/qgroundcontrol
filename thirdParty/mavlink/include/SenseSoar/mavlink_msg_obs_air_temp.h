// MESSAGE OBS_AIR_TEMP PACKING

#define MAVLINK_MSG_ID_OBS_AIR_TEMP 183

typedef struct __mavlink_obs_air_temp_t 
{
	float airT; ///< Air Temperatur

} mavlink_obs_air_temp_t;



/**
 * @brief Pack a obs_air_temp message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param airT Air Temperatur
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_obs_air_temp_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, float airT)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_OBS_AIR_TEMP;

	i += put_float_by_index(airT, i, msg->payload); // Air Temperatur

	return mavlink_finalize_message(msg, system_id, component_id, i);
}

/**
 * @brief Pack a obs_air_temp message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param airT Air Temperatur
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_obs_air_temp_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, float airT)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_OBS_AIR_TEMP;

	i += put_float_by_index(airT, i, msg->payload); // Air Temperatur

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, i);
}

/**
 * @brief Encode a obs_air_temp struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param obs_air_temp C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_obs_air_temp_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_obs_air_temp_t* obs_air_temp)
{
	return mavlink_msg_obs_air_temp_pack(system_id, component_id, msg, obs_air_temp->airT);
}

/**
 * @brief Send a obs_air_temp message
 * @param chan MAVLink channel to send the message
 *
 * @param airT Air Temperatur
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_obs_air_temp_send(mavlink_channel_t chan, float airT)
{
	mavlink_message_t msg;
	mavlink_msg_obs_air_temp_pack_chan(mavlink_system.sysid, mavlink_system.compid, chan, &msg, airT);
	mavlink_send_uart(chan, &msg);
}

#endif
// MESSAGE OBS_AIR_TEMP UNPACKING

/**
 * @brief Get field airT from obs_air_temp message
 *
 * @return Air Temperatur
 */
static inline float mavlink_msg_obs_air_temp_get_airT(const mavlink_message_t* msg)
{
	generic_32bit r;
	r.b[3] = (msg->payload)[0];
	r.b[2] = (msg->payload)[1];
	r.b[1] = (msg->payload)[2];
	r.b[0] = (msg->payload)[3];
	return (float)r.f;
}

/**
 * @brief Decode a obs_air_temp message into a struct
 *
 * @param msg The message to decode
 * @param obs_air_temp C-struct to decode the message contents into
 */
static inline void mavlink_msg_obs_air_temp_decode(const mavlink_message_t* msg, mavlink_obs_air_temp_t* obs_air_temp)
{
	obs_air_temp->airT = mavlink_msg_obs_air_temp_get_airT(msg);
}
