// MESSAGE WATCHDOG_PROCESS_INFO PACKING

#define MAVLINK_MSG_ID_WATCHDOG_PROCESS_INFO 151
#define MAVLINK_MSG_ID_WATCHDOG_PROCESS_INFO_LEN 255
#define MAVLINK_MSG_151_LEN 255

typedef struct __mavlink_watchdog_process_info_t 
{
	int32_t timeout; ///< Timeout (seconds)
	uint16_t watchdog_id; ///< Watchdog ID
	uint16_t process_id; ///< Process ID
	uint8_t name[100]; ///< Process name
	uint8_t arguments[147]; ///< Process arguments

} mavlink_watchdog_process_info_t;
#define MAVLINK_MSG_WATCHDOG_PROCESS_INFO_FIELD_NAME_LEN 100
#define MAVLINK_MSG_WATCHDOG_PROCESS_INFO_FIELD_ARGUMENTS_LEN 147

/**
 * @brief Pack a watchdog_process_info message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param watchdog_id Watchdog ID
 * @param process_id Process ID
 * @param name Process name
 * @param arguments Process arguments
 * @param timeout Timeout (seconds)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_watchdog_process_info_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint16_t watchdog_id, uint16_t process_id, const uint8_t* name, const uint8_t* arguments, int32_t timeout)
{
	mavlink_watchdog_process_info_t *p = (mavlink_watchdog_process_info_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_WATCHDOG_PROCESS_INFO;

	p->watchdog_id = watchdog_id; // uint16_t:Watchdog ID
	p->process_id = process_id; // uint16_t:Process ID
	memcpy(p->name, name, sizeof(p->name)); // uint8_t[100]:Process name
	memcpy(p->arguments, arguments, sizeof(p->arguments)); // uint8_t[147]:Process arguments
	p->timeout = timeout; // int32_t:Timeout (seconds)

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_WATCHDOG_PROCESS_INFO_LEN);
}

/**
 * @brief Pack a watchdog_process_info message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param watchdog_id Watchdog ID
 * @param process_id Process ID
 * @param name Process name
 * @param arguments Process arguments
 * @param timeout Timeout (seconds)
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_watchdog_process_info_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint16_t watchdog_id, uint16_t process_id, const uint8_t* name, const uint8_t* arguments, int32_t timeout)
{
	mavlink_watchdog_process_info_t *p = (mavlink_watchdog_process_info_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_WATCHDOG_PROCESS_INFO;

	p->watchdog_id = watchdog_id; // uint16_t:Watchdog ID
	p->process_id = process_id; // uint16_t:Process ID
	memcpy(p->name, name, sizeof(p->name)); // uint8_t[100]:Process name
	memcpy(p->arguments, arguments, sizeof(p->arguments)); // uint8_t[147]:Process arguments
	p->timeout = timeout; // int32_t:Timeout (seconds)

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_WATCHDOG_PROCESS_INFO_LEN);
}

/**
 * @brief Encode a watchdog_process_info struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param watchdog_process_info C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_watchdog_process_info_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_watchdog_process_info_t* watchdog_process_info)
{
	return mavlink_msg_watchdog_process_info_pack(system_id, component_id, msg, watchdog_process_info->watchdog_id, watchdog_process_info->process_id, watchdog_process_info->name, watchdog_process_info->arguments, watchdog_process_info->timeout);
}

/**
 * @brief Send a watchdog_process_info message
 * @param chan MAVLink channel to send the message
 *
 * @param watchdog_id Watchdog ID
 * @param process_id Process ID
 * @param name Process name
 * @param arguments Process arguments
 * @param timeout Timeout (seconds)
 */


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_watchdog_process_info_send(mavlink_channel_t chan, uint16_t watchdog_id, uint16_t process_id, const uint8_t* name, const uint8_t* arguments, int32_t timeout)
{
	mavlink_header_t hdr;
	mavlink_watchdog_process_info_t payload;
	uint16_t checksum;
	mavlink_watchdog_process_info_t *p = &payload;

	p->watchdog_id = watchdog_id; // uint16_t:Watchdog ID
	p->process_id = process_id; // uint16_t:Process ID
	memcpy(p->name, name, sizeof(p->name)); // uint8_t[100]:Process name
	memcpy(p->arguments, arguments, sizeof(p->arguments)); // uint8_t[147]:Process arguments
	p->timeout = timeout; // int32_t:Timeout (seconds)

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_WATCHDOG_PROCESS_INFO_LEN;
	hdr.msgid = MAVLINK_MSG_ID_WATCHDOG_PROCESS_INFO;
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
// MESSAGE WATCHDOG_PROCESS_INFO UNPACKING

/**
 * @brief Get field watchdog_id from watchdog_process_info message
 *
 * @return Watchdog ID
 */
static inline uint16_t mavlink_msg_watchdog_process_info_get_watchdog_id(const mavlink_message_t* msg)
{
	mavlink_watchdog_process_info_t *p = (mavlink_watchdog_process_info_t *)&msg->payload[0];
	return (uint16_t)(p->watchdog_id);
}

/**
 * @brief Get field process_id from watchdog_process_info message
 *
 * @return Process ID
 */
static inline uint16_t mavlink_msg_watchdog_process_info_get_process_id(const mavlink_message_t* msg)
{
	mavlink_watchdog_process_info_t *p = (mavlink_watchdog_process_info_t *)&msg->payload[0];
	return (uint16_t)(p->process_id);
}

/**
 * @brief Get field name from watchdog_process_info message
 *
 * @return Process name
 */
static inline uint16_t mavlink_msg_watchdog_process_info_get_name(const mavlink_message_t* msg, uint8_t* name)
{
	mavlink_watchdog_process_info_t *p = (mavlink_watchdog_process_info_t *)&msg->payload[0];

	memcpy(name, p->name, sizeof(p->name));
	return sizeof(p->name);
}

/**
 * @brief Get field arguments from watchdog_process_info message
 *
 * @return Process arguments
 */
static inline uint16_t mavlink_msg_watchdog_process_info_get_arguments(const mavlink_message_t* msg, uint8_t* arguments)
{
	mavlink_watchdog_process_info_t *p = (mavlink_watchdog_process_info_t *)&msg->payload[0];

	memcpy(arguments, p->arguments, sizeof(p->arguments));
	return sizeof(p->arguments);
}

/**
 * @brief Get field timeout from watchdog_process_info message
 *
 * @return Timeout (seconds)
 */
static inline int32_t mavlink_msg_watchdog_process_info_get_timeout(const mavlink_message_t* msg)
{
	mavlink_watchdog_process_info_t *p = (mavlink_watchdog_process_info_t *)&msg->payload[0];
	return (int32_t)(p->timeout);
}

/**
 * @brief Decode a watchdog_process_info message into a struct
 *
 * @param msg The message to decode
 * @param watchdog_process_info C-struct to decode the message contents into
 */
static inline void mavlink_msg_watchdog_process_info_decode(const mavlink_message_t* msg, mavlink_watchdog_process_info_t* watchdog_process_info)
{
	memcpy( watchdog_process_info, msg->payload, sizeof(mavlink_watchdog_process_info_t));
}
