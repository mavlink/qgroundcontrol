// MESSAGE CMD_AIRSPEED_CHNG PACKING

#define MAVLINK_MSG_ID_CMD_AIRSPEED_CHNG 192

typedef struct __mavlink_cmd_airspeed_chng_t 
{
	uint8_t target; ///< Target ID
	float spCmd; ///< commanded airspeed

} mavlink_cmd_airspeed_chng_t;



/**
 * @brief Pack a cmd_airspeed_chng message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target Target ID
 * @param spCmd commanded airspeed
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_cmd_airspeed_chng_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t target, float spCmd)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_CMD_AIRSPEED_CHNG;

	i += put_uint8_t_by_index(target, i, msg->payload); // Target ID
	i += put_float_by_index(spCmd, i, msg->payload); // commanded airspeed

	return mavlink_finalize_message(msg, system_id, component_id, i);
}

/**
 * @brief Pack a cmd_airspeed_chng message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target Target ID
 * @param spCmd commanded airspeed
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_cmd_airspeed_chng_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t target, float spCmd)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_CMD_AIRSPEED_CHNG;

	i += put_uint8_t_by_index(target, i, msg->payload); // Target ID
	i += put_float_by_index(spCmd, i, msg->payload); // commanded airspeed

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, i);
}

/**
 * @brief Encode a cmd_airspeed_chng struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param cmd_airspeed_chng C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_cmd_airspeed_chng_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_cmd_airspeed_chng_t* cmd_airspeed_chng)
{
	return mavlink_msg_cmd_airspeed_chng_pack(system_id, component_id, msg, cmd_airspeed_chng->target, cmd_airspeed_chng->spCmd);
}

/**
 * @brief Send a cmd_airspeed_chng message
 * @param chan MAVLink channel to send the message
 *
 * @param target Target ID
 * @param spCmd commanded airspeed
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_cmd_airspeed_chng_send(mavlink_channel_t chan, uint8_t target, float spCmd)
{
	mavlink_message_t msg;
	mavlink_msg_cmd_airspeed_chng_pack_chan(mavlink_system.sysid, mavlink_system.compid, chan, &msg, target, spCmd);
	mavlink_send_uart(chan, &msg);
}

#endif
// MESSAGE CMD_AIRSPEED_CHNG UNPACKING

/**
 * @brief Get field target from cmd_airspeed_chng message
 *
 * @return Target ID
 */
static inline uint8_t mavlink_msg_cmd_airspeed_chng_get_target(const mavlink_message_t* msg)
{
	return (uint8_t)(msg->payload)[0];
}

/**
 * @brief Get field spCmd from cmd_airspeed_chng message
 *
 * @return commanded airspeed
 */
static inline float mavlink_msg_cmd_airspeed_chng_get_spCmd(const mavlink_message_t* msg)
{
	generic_32bit r;
	r.b[3] = (msg->payload+sizeof(uint8_t))[0];
	r.b[2] = (msg->payload+sizeof(uint8_t))[1];
	r.b[1] = (msg->payload+sizeof(uint8_t))[2];
	r.b[0] = (msg->payload+sizeof(uint8_t))[3];
	return (float)r.f;
}

/**
 * @brief Decode a cmd_airspeed_chng message into a struct
 *
 * @param msg The message to decode
 * @param cmd_airspeed_chng C-struct to decode the message contents into
 */
static inline void mavlink_msg_cmd_airspeed_chng_decode(const mavlink_message_t* msg, mavlink_cmd_airspeed_chng_t* cmd_airspeed_chng)
{
	cmd_airspeed_chng->target = mavlink_msg_cmd_airspeed_chng_get_target(msg);
	cmd_airspeed_chng->spCmd = mavlink_msg_cmd_airspeed_chng_get_spCmd(msg);
}
