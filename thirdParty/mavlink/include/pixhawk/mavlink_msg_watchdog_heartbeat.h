// MESSAGE WATCHDOG_HEARTBEAT PACKING

#define MAVLINK_MSG_ID_WATCHDOG_HEARTBEAT 150
#define MAVLINK_MSG_ID_WATCHDOG_HEARTBEAT_LEN 4
#define MAVLINK_MSG_150_LEN 4

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
	mavlink_watchdog_heartbeat_t *p = (mavlink_watchdog_heartbeat_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_WATCHDOG_HEARTBEAT;

	p->watchdog_id = watchdog_id; // uint16_t:Watchdog ID
	p->process_count = process_count; // uint16_t:Number of processes

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_WATCHDOG_HEARTBEAT_LEN);
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
	mavlink_watchdog_heartbeat_t *p = (mavlink_watchdog_heartbeat_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_WATCHDOG_HEARTBEAT;

	p->watchdog_id = watchdog_id; // uint16_t:Watchdog ID
	p->process_count = process_count; // uint16_t:Number of processes

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_WATCHDOG_HEARTBEAT_LEN);
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
	uint16_t checksum;
	mavlink_watchdog_heartbeat_t *p = (mavlink_watchdog_heartbeat_t *)&msg.payload[0];

	p->watchdog_id = watchdog_id; // uint16_t:Watchdog ID
	p->process_count = process_count; // uint16_t:Number of processes

	msg.STX = MAVLINK_STX;
	msg.len = MAVLINK_MSG_ID_WATCHDOG_HEARTBEAT_LEN;
	msg.msgid = MAVLINK_MSG_ID_WATCHDOG_HEARTBEAT;
	msg.sysid = mavlink_system.sysid;
	msg.compid = mavlink_system.compid;
	msg.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = msg.seq + 1;
	checksum = crc_calculate_msg(&msg, msg.len + MAVLINK_CORE_HEADER_LEN);
	msg.ck_a = (uint8_t)(checksum & 0xFF); ///< Low byte
	msg.ck_b = (uint8_t)(checksum >> 8); ///< High byte

	mavlink_send_msg(chan, &msg);
}

#endif

#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS_SMALL
static inline void mavlink_msg_watchdog_heartbeat_send(mavlink_channel_t chan, uint16_t watchdog_id, uint16_t process_count)
{
	mavlink_header_t hdr;
	mavlink_watchdog_heartbeat_t payload;
	uint16_t checksum;
	mavlink_watchdog_heartbeat_t *p = &payload;

	p->watchdog_id = watchdog_id; // uint16_t:Watchdog ID
	p->process_count = process_count; // uint16_t:Number of processes

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_WATCHDOG_HEARTBEAT_LEN;
	hdr.msgid = MAVLINK_MSG_ID_WATCHDOG_HEARTBEAT;
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
// MESSAGE WATCHDOG_HEARTBEAT UNPACKING

/**
 * @brief Get field watchdog_id from watchdog_heartbeat message
 *
 * @return Watchdog ID
 */
static inline uint16_t mavlink_msg_watchdog_heartbeat_get_watchdog_id(const mavlink_message_t* msg)
{
	mavlink_watchdog_heartbeat_t *p = (mavlink_watchdog_heartbeat_t *)&msg->payload[0];
	return (uint16_t)(p->watchdog_id);
}

/**
 * @brief Get field process_count from watchdog_heartbeat message
 *
 * @return Number of processes
 */
static inline uint16_t mavlink_msg_watchdog_heartbeat_get_process_count(const mavlink_message_t* msg)
{
	mavlink_watchdog_heartbeat_t *p = (mavlink_watchdog_heartbeat_t *)&msg->payload[0];
	return (uint16_t)(p->process_count);
}

/**
 * @brief Decode a watchdog_heartbeat message into a struct
 *
 * @param msg The message to decode
 * @param watchdog_heartbeat C-struct to decode the message contents into
 */
static inline void mavlink_msg_watchdog_heartbeat_decode(const mavlink_message_t* msg, mavlink_watchdog_heartbeat_t* watchdog_heartbeat)
{
	memcpy( watchdog_heartbeat, msg->payload, sizeof(mavlink_watchdog_heartbeat_t));
}
