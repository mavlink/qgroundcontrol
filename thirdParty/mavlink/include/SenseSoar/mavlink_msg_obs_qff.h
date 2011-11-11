// MESSAGE OBS_QFF PACKING

#define MAVLINK_MSG_ID_OBS_QFF 182

typedef struct __mavlink_obs_qff_t 
{
	float qff; ///< Wind

} mavlink_obs_qff_t;



/**
 * @brief Pack a obs_qff message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param qff Wind
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_obs_qff_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, float qff)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_OBS_QFF;

	i += put_float_by_index(qff, i, msg->payload); // Wind

	return mavlink_finalize_message(msg, system_id, component_id, i);
}

/**
 * @brief Pack a obs_qff message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param qff Wind
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_obs_qff_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, float qff)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_OBS_QFF;

	i += put_float_by_index(qff, i, msg->payload); // Wind

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, i);
}

/**
 * @brief Encode a obs_qff struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param obs_qff C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_obs_qff_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_obs_qff_t* obs_qff)
{
	return mavlink_msg_obs_qff_pack(system_id, component_id, msg, obs_qff->qff);
}

/**
 * @brief Send a obs_qff message
 * @param chan MAVLink channel to send the message
 *
 * @param qff Wind
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_obs_qff_send(mavlink_channel_t chan, float qff)
{
	mavlink_message_t msg;
	mavlink_msg_obs_qff_pack_chan(mavlink_system.sysid, mavlink_system.compid, chan, &msg, qff);
	mavlink_send_uart(chan, &msg);
}

#endif
// MESSAGE OBS_QFF UNPACKING

/**
 * @brief Get field qff from obs_qff message
 *
 * @return Wind
 */
static inline float mavlink_msg_obs_qff_get_qff(const mavlink_message_t* msg)
{
	generic_32bit r;
	r.b[3] = (msg->payload)[0];
	r.b[2] = (msg->payload)[1];
	r.b[1] = (msg->payload)[2];
	r.b[0] = (msg->payload)[3];
	return (float)r.f;
}

/**
 * @brief Decode a obs_qff message into a struct
 *
 * @param msg The message to decode
 * @param obs_qff C-struct to decode the message contents into
 */
static inline void mavlink_msg_obs_qff_decode(const mavlink_message_t* msg, mavlink_obs_qff_t* obs_qff)
{
	obs_qff->qff = mavlink_msg_obs_qff_get_qff(msg);
}
