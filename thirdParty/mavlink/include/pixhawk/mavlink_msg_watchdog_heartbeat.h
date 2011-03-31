// MESSAGE WATCHDOG_HEARTBEAT PACKING

#define MAVLINK_MSG_ID_WATCHDOG_HEARTBEAT 150

typedef struct __mavlink_watchdog_heartbeat_t 
{
	uint16_t watchdog_id; ///< Watchdog ID
	uint16_t process_count; ///< Number of processes

} mavlink_watchdog_heartbeat_t;



/**
 * @brief Pack a watchdog_heartbeat message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param watchdog_id Watchdog ID
 * @param process_count Number of processes
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_watchdog_heartbeat_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint16_t watchdog_id, uint16_t process_count)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_WATCHDOG_HEARTBEAT;

	i += put_uint16_t_by_index(watchdog_id, i, msg->payload); // Watchdog ID
	i += put_uint16_t_by_index(process_count, i, msg->payload); // Number of processes

	return mavlink_finalize_message(msg, system_id, component_id, i);
}

/**
 * @brief Pack a watchdog_heartbeat message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param watchdog_id Watchdog ID
 * @param process_count Number of processes
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_watchdog_heartbeat_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint16_t watchdog_id, uint16_t process_count)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_WATCHDOG_HEARTBEAT;

	i += put_uint16_t_by_index(watchdog_id, i, msg->payload); // Watchdog ID
	i += put_uint16_t_by_index(process_count, i, msg->payload); // Number of processes

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, i);
}

/**
 * @brief Encode a watchdog_heartbeat struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param watchdog_heartbeat C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_watchdog_heartbeat_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_watchdog_heartbeat_t* watchdog_heartbeat)
{
	return mavlink_msg_watchdog_heartbeat_pack(system_id, component_id, msg, watchdog_heartbeat->watchdog_id, watchdog_heartbeat->process_count);
}

/**
 * @brief Send a watchdog_heartbeat message
 * @param chan MAVLink channel to send the message
 *
 * @param watchdog_id Watchdog ID
 * @param process_count Number of processes
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_watchdog_heartbeat_send(mavlink_channel_t chan, uint16_t watchdog_id, uint16_t process_count)
{
	mavlink_message_t msg;
	mavlink_msg_watchdog_heartbeat_pack_chan(mavlink_system.sysid, mavlink_system.compid, chan, &msg, watchdog_id, process_count);
	mavlink_send_uart(chan, &msg);
}

#endif
// MESSAGE WATCHDOG_HEARTBEAT UNPACKING

/**
 * @brief Get field watchdog_id from watchdog_heartbeat message
 *
 * @return Watchdog ID
 */
static inline uint16_t mavlink_msg_watchdog_heartbeat_get_watchdog_id(const mavlink_message_t* msg)
{
	generic_16bit r;
	r.b[1] = (msg->payload)[0];
	r.b[0] = (msg->payload)[1];
	return (uint16_t)r.s;
}

/**
 * @brief Get field process_count from watchdog_heartbeat message
 *
 * @return Number of processes
 */
static inline uint16_t mavlink_msg_watchdog_heartbeat_get_process_count(const mavlink_message_t* msg)
{
	generic_16bit r;
	r.b[1] = (msg->payload+sizeof(uint16_t))[0];
	r.b[0] = (msg->payload+sizeof(uint16_t))[1];
	return (uint16_t)r.s;
}

/**
 * @brief Decode a watchdog_heartbeat message into a struct
 *
 * @param msg The message to decode
 * @param watchdog_heartbeat C-struct to decode the message contents into
 */
static inline void mavlink_msg_watchdog_heartbeat_decode(const mavlink_message_t* msg, mavlink_watchdog_heartbeat_t* watchdog_heartbeat)
{
	watchdog_heartbeat->watchdog_id = mavlink_msg_watchdog_heartbeat_get_watchdog_id(msg);
	watchdog_heartbeat->process_count = mavlink_msg_watchdog_heartbeat_get_process_count(msg);
}
