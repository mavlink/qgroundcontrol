// MESSAGE SYS_STAT PACKING

#define MAVLINK_MSG_ID_SYS_STAT 190

typedef struct __mavlink_sys_stat_t 
{
	uint8_t gps; ///< gps status
	uint8_t act; ///< actuator status
	uint8_t mod; ///< module status
	uint8_t commRssi; ///< module status

} mavlink_sys_stat_t;



/**
 * @brief Pack a sys_stat message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param gps gps status
 * @param act actuator status
 * @param mod module status
 * @param commRssi module status
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_sys_stat_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t gps, uint8_t act, uint8_t mod, uint8_t commRssi)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_SYS_STAT;

	i += put_uint8_t_by_index(gps, i, msg->payload); // gps status
	i += put_uint8_t_by_index(act, i, msg->payload); // actuator status
	i += put_uint8_t_by_index(mod, i, msg->payload); // module status
	i += put_uint8_t_by_index(commRssi, i, msg->payload); // module status

	return mavlink_finalize_message(msg, system_id, component_id, i);
}

/**
 * @brief Pack a sys_stat message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param gps gps status
 * @param act actuator status
 * @param mod module status
 * @param commRssi module status
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_sys_stat_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t gps, uint8_t act, uint8_t mod, uint8_t commRssi)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_SYS_STAT;

	i += put_uint8_t_by_index(gps, i, msg->payload); // gps status
	i += put_uint8_t_by_index(act, i, msg->payload); // actuator status
	i += put_uint8_t_by_index(mod, i, msg->payload); // module status
	i += put_uint8_t_by_index(commRssi, i, msg->payload); // module status

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, i);
}

/**
 * @brief Encode a sys_stat struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param sys_stat C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_sys_stat_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_sys_stat_t* sys_stat)
{
	return mavlink_msg_sys_stat_pack(system_id, component_id, msg, sys_stat->gps, sys_stat->act, sys_stat->mod, sys_stat->commRssi);
}

/**
 * @brief Send a sys_stat message
 * @param chan MAVLink channel to send the message
 *
 * @param gps gps status
 * @param act actuator status
 * @param mod module status
 * @param commRssi module status
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_sys_stat_send(mavlink_channel_t chan, uint8_t gps, uint8_t act, uint8_t mod, uint8_t commRssi)
{
	mavlink_message_t msg;
	mavlink_msg_sys_stat_pack_chan(mavlink_system.sysid, mavlink_system.compid, chan, &msg, gps, act, mod, commRssi);
	mavlink_send_uart(chan, &msg);
}

#endif
// MESSAGE SYS_STAT UNPACKING

/**
 * @brief Get field gps from sys_stat message
 *
 * @return gps status
 */
static inline uint8_t mavlink_msg_sys_stat_get_gps(const mavlink_message_t* msg)
{
	return (uint8_t)(msg->payload)[0];
}

/**
 * @brief Get field act from sys_stat message
 *
 * @return actuator status
 */
static inline uint8_t mavlink_msg_sys_stat_get_act(const mavlink_message_t* msg)
{
	return (uint8_t)(msg->payload+sizeof(uint8_t))[0];
}

/**
 * @brief Get field mod from sys_stat message
 *
 * @return module status
 */
static inline uint8_t mavlink_msg_sys_stat_get_mod(const mavlink_message_t* msg)
{
	return (uint8_t)(msg->payload+sizeof(uint8_t)+sizeof(uint8_t))[0];
}

/**
 * @brief Get field commRssi from sys_stat message
 *
 * @return module status
 */
static inline uint8_t mavlink_msg_sys_stat_get_commRssi(const mavlink_message_t* msg)
{
	return (uint8_t)(msg->payload+sizeof(uint8_t)+sizeof(uint8_t)+sizeof(uint8_t))[0];
}

/**
 * @brief Decode a sys_stat message into a struct
 *
 * @param msg The message to decode
 * @param sys_stat C-struct to decode the message contents into
 */
static inline void mavlink_msg_sys_stat_decode(const mavlink_message_t* msg, mavlink_sys_stat_t* sys_stat)
{
	sys_stat->gps = mavlink_msg_sys_stat_get_gps(msg);
	sys_stat->act = mavlink_msg_sys_stat_get_act(msg);
	sys_stat->mod = mavlink_msg_sys_stat_get_mod(msg);
	sys_stat->commRssi = mavlink_msg_sys_stat_get_commRssi(msg);
}
