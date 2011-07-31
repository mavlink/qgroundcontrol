// MESSAGE WAYPOINT_ACK PACKING

#define MAVLINK_MSG_ID_WAYPOINT_ACK 47
#define MAVLINK_MSG_ID_WAYPOINT_ACK_LEN 3
#define MAVLINK_MSG_47_LEN 3

typedef struct __mavlink_waypoint_ack_t 
{
	uint8_t target_system; ///< System ID
	uint8_t target_component; ///< Component ID
	uint8_t type; ///< 0: OK, 1: Error

} mavlink_waypoint_ack_t;

/**
 * @brief Pack a waypoint_ack message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param type 0: OK, 1: Error
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_waypoint_ack_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component, uint8_t type)
{
	mavlink_waypoint_ack_t *p = (mavlink_waypoint_ack_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_WAYPOINT_ACK;

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	p->type = type; // uint8_t:0: OK, 1: Error

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_WAYPOINT_ACK_LEN);
}

/**
 * @brief Pack a waypoint_ack message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system System ID
 * @param target_component Component ID
 * @param type 0: OK, 1: Error
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_waypoint_ack_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component, uint8_t type)
{
	mavlink_waypoint_ack_t *p = (mavlink_waypoint_ack_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_WAYPOINT_ACK;

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	p->type = type; // uint8_t:0: OK, 1: Error

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_WAYPOINT_ACK_LEN);
}

/**
 * @brief Encode a waypoint_ack struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param waypoint_ack C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_waypoint_ack_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_waypoint_ack_t* waypoint_ack)
{
	return mavlink_msg_waypoint_ack_pack(system_id, component_id, msg, waypoint_ack->target_system, waypoint_ack->target_component, waypoint_ack->type);
}

/**
 * @brief Send a waypoint_ack message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param type 0: OK, 1: Error
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_waypoint_ack_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, uint8_t type)
{
	mavlink_message_t msg;
	uint16_t checksum;
	mavlink_waypoint_ack_t *p = (mavlink_waypoint_ack_t *)&msg.payload[0];

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	p->type = type; // uint8_t:0: OK, 1: Error

	msg.STX = MAVLINK_STX;
	msg.len = MAVLINK_MSG_ID_WAYPOINT_ACK_LEN;
	msg.msgid = MAVLINK_MSG_ID_WAYPOINT_ACK;
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
static inline void mavlink_msg_waypoint_ack_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, uint8_t type)
{
	mavlink_header_t hdr;
	mavlink_waypoint_ack_t payload;
	uint16_t checksum;
	mavlink_waypoint_ack_t *p = &payload;

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	p->type = type; // uint8_t:0: OK, 1: Error

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_WAYPOINT_ACK_LEN;
	hdr.msgid = MAVLINK_MSG_ID_WAYPOINT_ACK;
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
// MESSAGE WAYPOINT_ACK UNPACKING

/**
 * @brief Get field target_system from waypoint_ack message
 *
 * @return System ID
 */
static inline uint8_t mavlink_msg_waypoint_ack_get_target_system(const mavlink_message_t* msg)
{
	mavlink_waypoint_ack_t *p = (mavlink_waypoint_ack_t *)&msg->payload[0];
	return (uint8_t)(p->target_system);
}

/**
 * @brief Get field target_component from waypoint_ack message
 *
 * @return Component ID
 */
static inline uint8_t mavlink_msg_waypoint_ack_get_target_component(const mavlink_message_t* msg)
{
	mavlink_waypoint_ack_t *p = (mavlink_waypoint_ack_t *)&msg->payload[0];
	return (uint8_t)(p->target_component);
}

/**
 * @brief Get field type from waypoint_ack message
 *
 * @return 0: OK, 1: Error
 */
static inline uint8_t mavlink_msg_waypoint_ack_get_type(const mavlink_message_t* msg)
{
	mavlink_waypoint_ack_t *p = (mavlink_waypoint_ack_t *)&msg->payload[0];
	return (uint8_t)(p->type);
}

/**
 * @brief Decode a waypoint_ack message into a struct
 *
 * @param msg The message to decode
 * @param waypoint_ack C-struct to decode the message contents into
 */
static inline void mavlink_msg_waypoint_ack_decode(const mavlink_message_t* msg, mavlink_waypoint_ack_t* waypoint_ack)
{
	memcpy( waypoint_ack, msg->payload, sizeof(mavlink_waypoint_ack_t));
}
