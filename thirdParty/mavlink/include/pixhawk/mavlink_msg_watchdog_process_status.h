// MESSAGE WATCHDOG_PROCESS_STATUS PACKING

#define MAVLINK_MSG_ID_WATCHDOG_PROCESS_STATUS 182

typedef struct __mavlink_watchdog_process_status_t
{
 int32_t pid; ///< PID
 uint16_t watchdog_id; ///< Watchdog ID
 uint16_t process_id; ///< Process ID
 uint16_t crashes; ///< Number of crashes
 uint8_t state; ///< Is running / finished / suspended / crashed
 uint8_t muted; ///< Is muted
} mavlink_watchdog_process_status_t;

#define MAVLINK_MSG_ID_WATCHDOG_PROCESS_STATUS_LEN 12
#define MAVLINK_MSG_ID_182_LEN 12



#define MAVLINK_MESSAGE_INFO_WATCHDOG_PROCESS_STATUS { \
	"WATCHDOG_PROCESS_STATUS", \
	6, \
	{  { "pid", MAVLINK_TYPE_INT32_T, 0, 0, offsetof(mavlink_watchdog_process_status_t, pid) }, \
         { "watchdog_id", MAVLINK_TYPE_UINT16_T, 0, 4, offsetof(mavlink_watchdog_process_status_t, watchdog_id) }, \
         { "process_id", MAVLINK_TYPE_UINT16_T, 0, 6, offsetof(mavlink_watchdog_process_status_t, process_id) }, \
         { "crashes", MAVLINK_TYPE_UINT16_T, 0, 8, offsetof(mavlink_watchdog_process_status_t, crashes) }, \
         { "state", MAVLINK_TYPE_UINT8_T, 0, 10, offsetof(mavlink_watchdog_process_status_t, state) }, \
         { "muted", MAVLINK_TYPE_UINT8_T, 0, 11, offsetof(mavlink_watchdog_process_status_t, muted) }, \
         } \
}


/**
 * @brief Pack a watchdog_process_status message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param watchdog_id Watchdog ID
 * @param process_id Process ID
 * @param state Is running / finished / suspended / crashed
 * @param muted Is muted
 * @param pid PID
 * @param crashes Number of crashes
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_watchdog_process_status_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint16_t watchdog_id, uint16_t process_id, uint8_t state, uint8_t muted, int32_t pid, uint16_t crashes)
{
	msg->msgid = MAVLINK_MSG_ID_WATCHDOG_PROCESS_STATUS;

	put_int32_t_by_index(msg, 0, pid); // PID
	put_uint16_t_by_index(msg, 4, watchdog_id); // Watchdog ID
	put_uint16_t_by_index(msg, 6, process_id); // Process ID
	put_uint16_t_by_index(msg, 8, crashes); // Number of crashes
	put_uint8_t_by_index(msg, 10, state); // Is running / finished / suspended / crashed
	put_uint8_t_by_index(msg, 11, muted); // Is muted

	return mavlink_finalize_message(msg, system_id, component_id, 12, 29);
}

/**
 * @brief Pack a watchdog_process_status message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param watchdog_id Watchdog ID
 * @param process_id Process ID
 * @param state Is running / finished / suspended / crashed
 * @param muted Is muted
 * @param pid PID
 * @param crashes Number of crashes
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_watchdog_process_status_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint16_t watchdog_id,uint16_t process_id,uint8_t state,uint8_t muted,int32_t pid,uint16_t crashes)
{
	msg->msgid = MAVLINK_MSG_ID_WATCHDOG_PROCESS_STATUS;

	put_int32_t_by_index(msg, 0, pid); // PID
	put_uint16_t_by_index(msg, 4, watchdog_id); // Watchdog ID
	put_uint16_t_by_index(msg, 6, process_id); // Process ID
	put_uint16_t_by_index(msg, 8, crashes); // Number of crashes
	put_uint8_t_by_index(msg, 10, state); // Is running / finished / suspended / crashed
	put_uint8_t_by_index(msg, 11, muted); // Is muted

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, 12, 29);
}

#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

/**
 * @brief Pack a watchdog_process_status message on a channel and send
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param watchdog_id Watchdog ID
 * @param process_id Process ID
 * @param state Is running / finished / suspended / crashed
 * @param muted Is muted
 * @param pid PID
 * @param crashes Number of crashes
 */
static inline void mavlink_msg_watchdog_process_status_pack_chan_send(mavlink_channel_t chan,
							   mavlink_message_t* msg,
						           uint16_t watchdog_id,uint16_t process_id,uint8_t state,uint8_t muted,int32_t pid,uint16_t crashes)
{
	msg->msgid = MAVLINK_MSG_ID_WATCHDOG_PROCESS_STATUS;

	put_int32_t_by_index(msg, 0, pid); // PID
	put_uint16_t_by_index(msg, 4, watchdog_id); // Watchdog ID
	put_uint16_t_by_index(msg, 6, process_id); // Process ID
	put_uint16_t_by_index(msg, 8, crashes); // Number of crashes
	put_uint8_t_by_index(msg, 10, state); // Is running / finished / suspended / crashed
	put_uint8_t_by_index(msg, 11, muted); // Is muted

	mavlink_finalize_message_chan_send(msg, chan, 12, 29);
}
#endif // MAVLINK_USE_CONVENIENCE_FUNCTIONS


/**
 * @brief Encode a watchdog_process_status struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param watchdog_process_status C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_watchdog_process_status_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_watchdog_process_status_t* watchdog_process_status)
{
	return mavlink_msg_watchdog_process_status_pack(system_id, component_id, msg, watchdog_process_status->watchdog_id, watchdog_process_status->process_id, watchdog_process_status->state, watchdog_process_status->muted, watchdog_process_status->pid, watchdog_process_status->crashes);
}

/**
 * @brief Send a watchdog_process_status message
 * @param chan MAVLink channel to send the message
 *
 * @param watchdog_id Watchdog ID
 * @param process_id Process ID
 * @param state Is running / finished / suspended / crashed
 * @param muted Is muted
 * @param pid PID
 * @param crashes Number of crashes
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_watchdog_process_status_send(mavlink_channel_t chan, uint16_t watchdog_id, uint16_t process_id, uint8_t state, uint8_t muted, int32_t pid, uint16_t crashes)
{
	MAVLINK_ALIGNED_MESSAGE(msg, 12);
	mavlink_msg_watchdog_process_status_pack_chan_send(chan, msg, watchdog_id, process_id, state, muted, pid, crashes);
}

#endif

// MESSAGE WATCHDOG_PROCESS_STATUS UNPACKING


/**
 * @brief Get field watchdog_id from watchdog_process_status message
 *
 * @return Watchdog ID
 */
static inline uint16_t mavlink_msg_watchdog_process_status_get_watchdog_id(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint16_t(msg,  4);
}

/**
 * @brief Get field process_id from watchdog_process_status message
 *
 * @return Process ID
 */
static inline uint16_t mavlink_msg_watchdog_process_status_get_process_id(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint16_t(msg,  6);
}

/**
 * @brief Get field state from watchdog_process_status message
 *
 * @return Is running / finished / suspended / crashed
 */
static inline uint8_t mavlink_msg_watchdog_process_status_get_state(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  10);
}

/**
 * @brief Get field muted from watchdog_process_status message
 *
 * @return Is muted
 */
static inline uint8_t mavlink_msg_watchdog_process_status_get_muted(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  11);
}

/**
 * @brief Get field pid from watchdog_process_status message
 *
 * @return PID
 */
static inline int32_t mavlink_msg_watchdog_process_status_get_pid(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_int32_t(msg,  0);
}

/**
 * @brief Get field crashes from watchdog_process_status message
 *
 * @return Number of crashes
 */
static inline uint16_t mavlink_msg_watchdog_process_status_get_crashes(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint16_t(msg,  8);
}

/**
 * @brief Decode a watchdog_process_status message into a struct
 *
 * @param msg The message to decode
 * @param watchdog_process_status C-struct to decode the message contents into
 */
static inline void mavlink_msg_watchdog_process_status_decode(const mavlink_message_t* msg, mavlink_watchdog_process_status_t* watchdog_process_status)
{
#if MAVLINK_NEED_BYTE_SWAP
	watchdog_process_status->pid = mavlink_msg_watchdog_process_status_get_pid(msg);
	watchdog_process_status->watchdog_id = mavlink_msg_watchdog_process_status_get_watchdog_id(msg);
	watchdog_process_status->process_id = mavlink_msg_watchdog_process_status_get_process_id(msg);
	watchdog_process_status->crashes = mavlink_msg_watchdog_process_status_get_crashes(msg);
	watchdog_process_status->state = mavlink_msg_watchdog_process_status_get_state(msg);
	watchdog_process_status->muted = mavlink_msg_watchdog_process_status_get_muted(msg);
#else
	memcpy(watchdog_process_status, MAVLINK_PAYLOAD(msg), 12);
#endif
}
