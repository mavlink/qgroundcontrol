// MESSAGE SENSESOAR_MODE_CHNG PACKING

#define MAVLINK_MSG_ID_SENSESOAR_MODE_CHNG 166

typedef struct __mavlink_sensesoar_mode_chng_t 
{
	uint8_t target; ///< Target ID
	uint8_t mode; ///< Mode as desribed in the sensesoar_mode

} mavlink_sensesoar_mode_chng_t;



/**
 * @brief Pack a sensesoar_mode_chng message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target Target ID
 * @param mode Mode as desribed in the sensesoar_mode
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_sensesoar_mode_chng_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t target, uint8_t mode)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_SENSESOAR_MODE_CHNG;

	i += put_uint8_t_by_index(target, i, msg->payload); // Target ID
	i += put_uint8_t_by_index(mode, i, msg->payload); // Mode as desribed in the sensesoar_mode

	return mavlink_finalize_message(msg, system_id, component_id, i);
}

/**
 * @brief Pack a sensesoar_mode_chng message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target Target ID
 * @param mode Mode as desribed in the sensesoar_mode
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_sensesoar_mode_chng_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t target, uint8_t mode)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_SENSESOAR_MODE_CHNG;

	i += put_uint8_t_by_index(target, i, msg->payload); // Target ID
	i += put_uint8_t_by_index(mode, i, msg->payload); // Mode as desribed in the sensesoar_mode

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, i);
}

/**
 * @brief Encode a sensesoar_mode_chng struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param sensesoar_mode_chng C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_sensesoar_mode_chng_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_sensesoar_mode_chng_t* sensesoar_mode_chng)
{
	return mavlink_msg_sensesoar_mode_chng_pack(system_id, component_id, msg, sensesoar_mode_chng->target, sensesoar_mode_chng->mode);
}

/**
 * @brief Send a sensesoar_mode_chng message
 * @param chan MAVLink channel to send the message
 *
 * @param target Target ID
 * @param mode Mode as desribed in the sensesoar_mode
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_sensesoar_mode_chng_send(mavlink_channel_t chan, uint8_t target, uint8_t mode)
{
	mavlink_message_t msg;
	mavlink_msg_sensesoar_mode_chng_pack_chan(mavlink_system.sysid, mavlink_system.compid, chan, &msg, target, mode);
	mavlink_send_uart(chan, &msg);
}

#endif
// MESSAGE SENSESOAR_MODE_CHNG UNPACKING

/**
 * @brief Get field target from sensesoar_mode_chng message
 *
 * @return Target ID
 */
static inline uint8_t mavlink_msg_sensesoar_mode_chng_get_target(const mavlink_message_t* msg)
{
	return (uint8_t)(msg->payload)[0];
}

/**
 * @brief Get field mode from sensesoar_mode_chng message
 *
 * @return Mode as desribed in the sensesoar_mode
 */
static inline uint8_t mavlink_msg_sensesoar_mode_chng_get_mode(const mavlink_message_t* msg)
{
	return (uint8_t)(msg->payload+sizeof(uint8_t))[0];
}

/**
 * @brief Decode a sensesoar_mode_chng message into a struct
 *
 * @param msg The message to decode
 * @param sensesoar_mode_chng C-struct to decode the message contents into
 */
static inline void mavlink_msg_sensesoar_mode_chng_decode(const mavlink_message_t* msg, mavlink_sensesoar_mode_chng_t* sensesoar_mode_chng)
{
	sensesoar_mode_chng->target = mavlink_msg_sensesoar_mode_chng_get_target(msg);
	sensesoar_mode_chng->mode = mavlink_msg_sensesoar_mode_chng_get_mode(msg);
}
