// MESSAGE CMD_AIRSPEED_ACK PACKING

#define MAVLINK_MSG_ID_CMD_AIRSPEED_ACK 194

typedef struct __mavlink_cmd_airspeed_ack_t 
{
	float spCmd; ///< commanded airspeed
	uint8_t ack; ///< 0:ack, 1:nack

} mavlink_cmd_airspeed_ack_t;



/**
 * @brief Pack a cmd_airspeed_ack message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param spCmd commanded airspeed
 * @param ack 0:ack, 1:nack
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_cmd_airspeed_ack_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, float spCmd, uint8_t ack)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_CMD_AIRSPEED_ACK;

	i += put_float_by_index(spCmd, i, msg->payload); // commanded airspeed
	i += put_uint8_t_by_index(ack, i, msg->payload); // 0:ack, 1:nack

	return mavlink_finalize_message(msg, system_id, component_id, i);
}

/**
 * @brief Pack a cmd_airspeed_ack message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param spCmd commanded airspeed
 * @param ack 0:ack, 1:nack
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_cmd_airspeed_ack_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, float spCmd, uint8_t ack)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_CMD_AIRSPEED_ACK;

	i += put_float_by_index(spCmd, i, msg->payload); // commanded airspeed
	i += put_uint8_t_by_index(ack, i, msg->payload); // 0:ack, 1:nack

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, i);
}

/**
 * @brief Encode a cmd_airspeed_ack struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param cmd_airspeed_ack C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_cmd_airspeed_ack_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_cmd_airspeed_ack_t* cmd_airspeed_ack)
{
	return mavlink_msg_cmd_airspeed_ack_pack(system_id, component_id, msg, cmd_airspeed_ack->spCmd, cmd_airspeed_ack->ack);
}

/**
 * @brief Send a cmd_airspeed_ack message
 * @param chan MAVLink channel to send the message
 *
 * @param spCmd commanded airspeed
 * @param ack 0:ack, 1:nack
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_cmd_airspeed_ack_send(mavlink_channel_t chan, float spCmd, uint8_t ack)
{
	mavlink_message_t msg;
	mavlink_msg_cmd_airspeed_ack_pack_chan(mavlink_system.sysid, mavlink_system.compid, chan, &msg, spCmd, ack);
	mavlink_send_uart(chan, &msg);
}

#endif
// MESSAGE CMD_AIRSPEED_ACK UNPACKING

/**
 * @brief Get field spCmd from cmd_airspeed_ack message
 *
 * @return commanded airspeed
 */
static inline float mavlink_msg_cmd_airspeed_ack_get_spCmd(const mavlink_message_t* msg)
{
	generic_32bit r;
	r.b[3] = (msg->payload)[0];
	r.b[2] = (msg->payload)[1];
	r.b[1] = (msg->payload)[2];
	r.b[0] = (msg->payload)[3];
	return (float)r.f;
}

/**
 * @brief Get field ack from cmd_airspeed_ack message
 *
 * @return 0:ack, 1:nack
 */
static inline uint8_t mavlink_msg_cmd_airspeed_ack_get_ack(const mavlink_message_t* msg)
{
	return (uint8_t)(msg->payload+sizeof(float))[0];
}

/**
 * @brief Decode a cmd_airspeed_ack message into a struct
 *
 * @param msg The message to decode
 * @param cmd_airspeed_ack C-struct to decode the message contents into
 */
static inline void mavlink_msg_cmd_airspeed_ack_decode(const mavlink_message_t* msg, mavlink_cmd_airspeed_ack_t* cmd_airspeed_ack)
{
	cmd_airspeed_ack->spCmd = mavlink_msg_cmd_airspeed_ack_get_spCmd(msg);
	cmd_airspeed_ack->ack = mavlink_msg_cmd_airspeed_ack_get_ack(msg);
}
