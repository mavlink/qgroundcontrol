// MESSAGE OBS_POSITION PACKING

#define MAVLINK_MSG_ID_OBS_POSITION 170

typedef struct __mavlink_obs_position_t 
{
	double pos[3]; ///< Position

} mavlink_obs_position_t;

#define MAVLINK_MSG_OBS_POSITION_FIELD_POS_LEN 3


/**
 * @brief Pack a obs_position message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param pos Position
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_obs_position_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const double* pos)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_OBS_POSITION;

	i += put_array_by_index((const int8_t*)pos, sizeof(double)*3, i, msg->payload); // Position

	return mavlink_finalize_message(msg, system_id, component_id, i);
}

/**
 * @brief Pack a obs_position message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param pos Position
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_obs_position_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, const double* pos)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_OBS_POSITION;

	i += put_array_by_index((const int8_t*)pos, sizeof(double)*3, i, msg->payload); // Position

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
	return mavlink_msg_obs_position_pack(system_id, component_id, msg, obs_position->pos);
}

/**
 * @brief Send a obs_position message
 * @param chan MAVLink channel to send the message
 *
 * @param pos Position
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_obs_position_send(mavlink_channel_t chan, const double* pos)
{
	mavlink_message_t msg;
	mavlink_msg_obs_position_pack_chan(mavlink_system.sysid, mavlink_system.compid, chan, &msg, pos);
	mavlink_send_uart(chan, &msg);
}

#endif
// MESSAGE OBS_POSITION UNPACKING

/**
 * @brief Get field pos from obs_position message
 *
 * @return Position
 */
static inline uint16_t mavlink_msg_obs_position_get_pos(const mavlink_message_t* msg, double* r_data)
{

	memcpy(r_data, msg->payload, sizeof(double)*3);
	return sizeof(double)*3;
}

/**
 * @brief Decode a obs_position message into a struct
 *
 * @param msg The message to decode
 * @param obs_position C-struct to decode the message contents into
 */
static inline void mavlink_msg_obs_position_decode(const mavlink_message_t* msg, mavlink_obs_position_t* obs_position)
{
	mavlink_msg_obs_position_get_pos(msg, obs_position->pos);
}
