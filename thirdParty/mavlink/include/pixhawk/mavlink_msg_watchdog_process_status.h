// MESSAGE WATCHDOG_PROCESS_STATUS PACKING

#define MAVLINK_MSG_ID_WATCHDOG_PROCESS_STATUS 152
#define MAVLINK_MSG_ID_WATCHDOG_PROCESS_STATUS_LEN 12
#define MAVLINK_MSG_152_LEN 12

typedef struct __mavlink_watchdog_process_status_t 
{
	int32_t pid; ///< PID
	uint16_t watchdog_id; ///< Watchdog ID
	uint16_t process_id; ///< Process ID
	uint16_t crashes; ///< Number of crashes
	uint8_t state; ///< Is running / finished / suspended / crashed
	uint8_t muted; ///< Is muted

} mavlink_watchdog_process_status_t;

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
static inline uint16_t mavlink_msg_watchdog_process_status_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint16_t watchdog_id, uint16_t process_id, uint8_t state, uint8_t muted, int32_t pid, uint16_t crashes)
{
	mavlink_watchdog_process_status_t *p = (mavlink_watchdog_process_status_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_WATCHDOG_PROCESS_STATUS;

	p->watchdog_id = watchdog_id; // uint16_t:Watchdog ID
	p->process_id = process_id; // uint16_t:Process ID
	p->state = state; // uint8_t:Is running / finished / suspended / crashed
	p->muted = muted; // uint8_t:Is muted
	p->pid = pid; // int32_t:PID
	p->crashes = crashes; // uint16_t:Number of crashes

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_WATCHDOG_PROCESS_STATUS_LEN);
}

/**
 * @brief Pack a watchdog_process_status message
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
static inline uint16_t mavlink_msg_watchdog_process_status_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint16_t watchdog_id, uint16_t process_id, uint8_t state, uint8_t muted, int32_t pid, uint16_t crashes)
{
	mavlink_watchdog_process_status_t *p = (mavlink_watchdog_process_status_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_WATCHDOG_PROCESS_STATUS;

	p->watchdog_id = watchdog_id; // uint16_t:Watchdog ID
	p->process_id = process_id; // uint16_t:Process ID
	p->state = state; // uint8_t:Is running / finished / suspended / crashed
	p->muted = muted; // uint8_t:Is muted
	p->pid = pid; // int32_t:PID
	p->crashes = crashes; // uint16_t:Number of crashes

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_WATCHDOG_PROCESS_STATUS_LEN);
}

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
	mavlink_header_t hdr;
	mavlink_watchdog_process_status_t payload;
	uint16_t checksum;
	mavlink_watchdog_process_status_t *p = &payload;

	p->watchdog_id = watchdog_id; // uint16_t:Watchdog ID
	p->process_id = process_id; // uint16_t:Process ID
	p->state = state; // uint8_t:Is running / finished / suspended / crashed
	p->muted = muted; // uint8_t:Is muted
	p->pid = pid; // int32_t:PID
	p->crashes = crashes; // uint16_t:Number of crashes

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_WATCHDOG_PROCESS_STATUS_LEN;
	hdr.msgid = MAVLINK_MSG_ID_WATCHDOG_PROCESS_STATUS;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );

	crc_init(&checksum);
	checksum = crc_calculate_mem((uint8_t *)&hdr.len, &checksum, MAVLINK_CORE_HEADER_LEN);
	checksum = crc_calculate_mem((uint8_t *)&payload, &checksum, hdr.len );
	hdr.ck_a = (uint8_t)(checksum & 0xFF); ///< Low byte
	hdr.ck_b = (uint8_t)(checksum >> 8); ///< High byte

	mavlink_send_mem(chan, (uint8_t *)&payload, hdr.len);
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck_a, MAVLINK_NUM_CHECKSUM_BYTES);
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
	mavlink_watchdog_process_status_t *p = (mavlink_watchdog_process_status_t *)&msg->payload[0];
	return (uint16_t)(p->watchdog_id);
}

/**
 * @brief Get field process_id from watchdog_process_status message
 *
 * @return Process ID
 */
static inline uint16_t mavlink_msg_watchdog_process_status_get_process_id(const mavlink_message_t* msg)
{
	mavlink_watchdog_process_status_t *p = (mavlink_watchdog_process_status_t *)&msg->payload[0];
	return (uint16_t)(p->process_id);
}

/**
 * @brief Get field state from watchdog_process_status message
 *
 * @return Is running / finished / suspended / crashed
 */
static inline uint8_t mavlink_msg_watchdog_process_status_get_state(const mavlink_message_t* msg)
{
	mavlink_watchdog_process_status_t *p = (mavlink_watchdog_process_status_t *)&msg->payload[0];
	return (uint8_t)(p->state);
}

/**
 * @brief Get field muted from watchdog_process_status message
 *
 * @return Is muted
 */
static inline uint8_t mavlink_msg_watchdog_process_status_get_muted(const mavlink_message_t* msg)
{
	mavlink_watchdog_process_status_t *p = (mavlink_watchdog_process_status_t *)&msg->payload[0];
	return (uint8_t)(p->muted);
}

/**
 * @brief Get field pid from watchdog_process_status message
 *
 * @return PID
 */
static inline int32_t mavlink_msg_watchdog_process_status_get_pid(const mavlink_message_t* msg)
{
	mavlink_watchdog_process_status_t *p = (mavlink_watchdog_process_status_t *)&msg->payload[0];
	return (int32_t)(p->pid);
}

/**
 * @brief Get field crashes from watchdog_process_status message
 *
 * @return Number of crashes
 */
static inline uint16_t mavlink_msg_watchdog_process_status_get_crashes(const mavlink_message_t* msg)
{
	mavlink_watchdog_process_status_t *p = (mavlink_watchdog_process_status_t *)&msg->payload[0];
	return (uint16_t)(p->crashes);
}

/**
 * @brief Decode a watchdog_process_status message into a struct
 *
 * @param msg The message to decode
 * @param watchdog_process_status C-struct to decode the message contents into
 */
static inline void mavlink_msg_watchdog_process_status_decode(const mavlink_message_t* msg, mavlink_watchdog_process_status_t* watchdog_process_status)
{
	memcpy( watchdog_process_status, msg->payload, sizeof(mavlink_watchdog_process_status_t));
}
