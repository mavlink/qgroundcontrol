// MESSAGE PING PACKING

#define MAVLINK_MSG_ID_PING 3
#define MAVLINK_MSG_ID_PING_LEN 14
#define MAVLINK_MSG_3_LEN 14

typedef struct __mavlink_ping_t 
{
	uint32_t seq; ///< PING sequence
	uint8_t target_system; ///< 0: request ping from all receiving systems, if greater than 0: message is a ping response and number is the system id of the requesting system
	uint8_t target_component; ///< 0: request ping from all receiving components, if greater than 0: message is a ping response and number is the system id of the requesting system
	uint64_t time; ///< Unix timestamp in microseconds

} mavlink_ping_t;

/**
 * @brief Pack a ping message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param seq PING sequence
 * @param target_system 0: request ping from all receiving systems, if greater than 0: message is a ping response and number is the system id of the requesting system
 * @param target_component 0: request ping from all receiving components, if greater than 0: message is a ping response and number is the system id of the requesting system
 * @param time Unix timestamp in microseconds
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_ping_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint32_t seq, uint8_t target_system, uint8_t target_component, uint64_t time)
{
	mavlink_ping_t *p = (mavlink_ping_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_PING;

	p->seq = seq; // uint32_t:PING sequence
	p->target_system = target_system; // uint8_t:0: request ping from all receiving systems, if greater than 0: message is a ping response and number is the system id of the requesting system
	p->target_component = target_component; // uint8_t:0: request ping from all receiving components, if greater than 0: message is a ping response and number is the system id of the requesting system
	p->time = time; // uint64_t:Unix timestamp in microseconds

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_PING_LEN);
}

/**
 * @brief Pack a ping message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param seq PING sequence
 * @param target_system 0: request ping from all receiving systems, if greater than 0: message is a ping response and number is the system id of the requesting system
 * @param target_component 0: request ping from all receiving components, if greater than 0: message is a ping response and number is the system id of the requesting system
 * @param time Unix timestamp in microseconds
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_ping_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint32_t seq, uint8_t target_system, uint8_t target_component, uint64_t time)
{
	mavlink_ping_t *p = (mavlink_ping_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_PING;

	p->seq = seq; // uint32_t:PING sequence
	p->target_system = target_system; // uint8_t:0: request ping from all receiving systems, if greater than 0: message is a ping response and number is the system id of the requesting system
	p->target_component = target_component; // uint8_t:0: request ping from all receiving components, if greater than 0: message is a ping response and number is the system id of the requesting system
	p->time = time; // uint64_t:Unix timestamp in microseconds

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_PING_LEN);
}

/**
 * @brief Encode a ping struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param ping C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_ping_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_ping_t* ping)
{
	return mavlink_msg_ping_pack(system_id, component_id, msg, ping->seq, ping->target_system, ping->target_component, ping->time);
}

/**
 * @brief Send a ping message
 * @param chan MAVLink channel to send the message
 *
 * @param seq PING sequence
 * @param target_system 0: request ping from all receiving systems, if greater than 0: message is a ping response and number is the system id of the requesting system
 * @param target_component 0: request ping from all receiving components, if greater than 0: message is a ping response and number is the system id of the requesting system
 * @param time Unix timestamp in microseconds
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_ping_send(mavlink_channel_t chan, uint32_t seq, uint8_t target_system, uint8_t target_component, uint64_t time)
{
	mavlink_message_t msg;
	uint16_t checksum;
	mavlink_ping_t *p = (mavlink_ping_t *)&msg.payload[0];

	p->seq = seq; // uint32_t:PING sequence
	p->target_system = target_system; // uint8_t:0: request ping from all receiving systems, if greater than 0: message is a ping response and number is the system id of the requesting system
	p->target_component = target_component; // uint8_t:0: request ping from all receiving components, if greater than 0: message is a ping response and number is the system id of the requesting system
	p->time = time; // uint64_t:Unix timestamp in microseconds

	msg.STX = MAVLINK_STX;
	msg.len = MAVLINK_MSG_ID_PING_LEN;
	msg.msgid = MAVLINK_MSG_ID_PING;
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
static inline void mavlink_msg_ping_send(mavlink_channel_t chan, uint32_t seq, uint8_t target_system, uint8_t target_component, uint64_t time)
{
	mavlink_header_t hdr;
	mavlink_ping_t payload;
	uint16_t checksum;
	mavlink_ping_t *p = &payload;

	p->seq = seq; // uint32_t:PING sequence
	p->target_system = target_system; // uint8_t:0: request ping from all receiving systems, if greater than 0: message is a ping response and number is the system id of the requesting system
	p->target_component = target_component; // uint8_t:0: request ping from all receiving components, if greater than 0: message is a ping response and number is the system id of the requesting system
	p->time = time; // uint64_t:Unix timestamp in microseconds

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_PING_LEN;
	hdr.msgid = MAVLINK_MSG_ID_PING;
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
// MESSAGE PING UNPACKING

/**
 * @brief Get field seq from ping message
 *
 * @return PING sequence
 */
static inline uint32_t mavlink_msg_ping_get_seq(const mavlink_message_t* msg)
{
	mavlink_ping_t *p = (mavlink_ping_t *)&msg->payload[0];
	return (uint32_t)(p->seq);
}

/**
 * @brief Get field target_system from ping message
 *
 * @return 0: request ping from all receiving systems, if greater than 0: message is a ping response and number is the system id of the requesting system
 */
static inline uint8_t mavlink_msg_ping_get_target_system(const mavlink_message_t* msg)
{
	mavlink_ping_t *p = (mavlink_ping_t *)&msg->payload[0];
	return (uint8_t)(p->target_system);
}

/**
 * @brief Get field target_component from ping message
 *
 * @return 0: request ping from all receiving components, if greater than 0: message is a ping response and number is the system id of the requesting system
 */
static inline uint8_t mavlink_msg_ping_get_target_component(const mavlink_message_t* msg)
{
	mavlink_ping_t *p = (mavlink_ping_t *)&msg->payload[0];
	return (uint8_t)(p->target_component);
}

/**
 * @brief Get field time from ping message
 *
 * @return Unix timestamp in microseconds
 */
static inline uint64_t mavlink_msg_ping_get_time(const mavlink_message_t* msg)
{
	mavlink_ping_t *p = (mavlink_ping_t *)&msg->payload[0];
	return (uint64_t)(p->time);
}

/**
 * @brief Decode a ping message into a struct
 *
 * @param msg The message to decode
 * @param ping C-struct to decode the message contents into
 */
static inline void mavlink_msg_ping_decode(const mavlink_message_t* msg, mavlink_ping_t* ping)
{
	memcpy( ping, msg->payload, sizeof(mavlink_ping_t));
}
