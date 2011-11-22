// MESSAGE OBS_POSITION PACKING

#define MAVLINK_MSG_ID_OBS_POSITION 170

typedef struct __mavlink_obs_position_t 
{
	int32_t lon; ///< Longitude expressed in 1E7
	int32_t lat; ///< Latitude expressed in 1E7
	int32_t alt; ///< Altitude expressed in milimeters

} mavlink_obs_position_t;



/**
 * @brief Pack a obs_position message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param lon Longitude expressed in 1E7
 * @param lat Latitude expressed in 1E7
 * @param alt Altitude expressed in milimeters
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_obs_position_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, int32_t lon, int32_t lat, int32_t alt)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_OBS_POSITION;

	i += put_int32_t_by_index(lon, i, msg->payload); // Longitude expressed in 1E7
	i += put_int32_t_by_index(lat, i, msg->payload); // Latitude expressed in 1E7
	i += put_int32_t_by_index(alt, i, msg->payload); // Altitude expressed in milimeters

	return mavlink_finalize_message(msg, system_id, component_id, i);
}

/**
 * @brief Pack a obs_position message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param lon Longitude expressed in 1E7
 * @param lat Latitude expressed in 1E7
 * @param alt Altitude expressed in milimeters
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_obs_position_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, int32_t lon, int32_t lat, int32_t alt)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_OBS_POSITION;

	i += put_int32_t_by_index(lon, i, msg->payload); // Longitude expressed in 1E7
	i += put_int32_t_by_index(lat, i, msg->payload); // Latitude expressed in 1E7
	i += put_int32_t_by_index(alt, i, msg->payload); // Altitude expressed in milimeters

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, i);
}

/**
 * @brief Encode a obs_position struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param obs_position C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_obs_position_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_obs_position_t* obs_position)
{
	return mavlink_msg_obs_position_pack(system_id, component_id, msg, obs_position->lon, obs_position->lat, obs_position->alt);
}

/**
 * @brief Send a obs_position message
 * @param chan MAVLink channel to send the message
 *
 * @param lon Longitude expressed in 1E7
 * @param lat Latitude expressed in 1E7
 * @param alt Altitude expressed in milimeters
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_obs_position_send(mavlink_channel_t chan, int32_t lon, int32_t lat, int32_t alt)
{
	mavlink_message_t msg;
	mavlink_msg_obs_position_pack_chan(mavlink_system.sysid, mavlink_system.compid, chan, &msg, lon, lat, alt);
	mavlink_send_uart(chan, &msg);
}

#endif
// MESSAGE OBS_POSITION UNPACKING

/**
 * @brief Get field lon from obs_position message
 *
 * @return Longitude expressed in 1E7
 */
static inline int32_t mavlink_msg_obs_position_get_lon(const mavlink_message_t* msg)
{
	generic_32bit r;
	r.b[3] = (msg->payload)[0];
	r.b[2] = (msg->payload)[1];
	r.b[1] = (msg->payload)[2];
	r.b[0] = (msg->payload)[3];
	return (int32_t)r.i;
}

/**
 * @brief Get field lat from obs_position message
 *
 * @return Latitude expressed in 1E7
 */
static inline int32_t mavlink_msg_obs_position_get_lat(const mavlink_message_t* msg)
{
	generic_32bit r;
	r.b[3] = (msg->payload+sizeof(int32_t))[0];
	r.b[2] = (msg->payload+sizeof(int32_t))[1];
	r.b[1] = (msg->payload+sizeof(int32_t))[2];
	r.b[0] = (msg->payload+sizeof(int32_t))[3];
	return (int32_t)r.i;
}

/**
 * @brief Get field alt from obs_position message
 *
 * @return Altitude expressed in milimeters
 */
static inline int32_t mavlink_msg_obs_position_get_alt(const mavlink_message_t* msg)
{
	generic_32bit r;
	r.b[3] = (msg->payload+sizeof(int32_t)+sizeof(int32_t))[0];
	r.b[2] = (msg->payload+sizeof(int32_t)+sizeof(int32_t))[1];
	r.b[1] = (msg->payload+sizeof(int32_t)+sizeof(int32_t))[2];
	r.b[0] = (msg->payload+sizeof(int32_t)+sizeof(int32_t))[3];
	return (int32_t)r.i;
}

/**
 * @brief Decode a obs_position message into a struct
 *
 * @param msg The message to decode
 * @param obs_position C-struct to decode the message contents into
 */
static inline void mavlink_msg_obs_position_decode(const mavlink_message_t* msg, mavlink_obs_position_t* obs_position)
{
	obs_position->lon = mavlink_msg_obs_position_get_lon(msg);
	obs_position->lat = mavlink_msg_obs_position_get_lat(msg);
	obs_position->alt = mavlink_msg_obs_position_get_alt(msg);
}
