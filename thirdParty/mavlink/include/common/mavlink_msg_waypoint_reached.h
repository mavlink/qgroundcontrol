// MESSAGE WAYPOINT_REACHED PACKING

#define MAVLINK_MSG_ID_WAYPOINT_REACHED 46
#define MAVLINK_MSG_ID_WAYPOINT_REACHED_LEN 2
#define MAVLINK_MSG_46_LEN 2

typedef struct __mavlink_waypoint_reached_t 
{
	uint16_t seq; ///< Sequence

} mavlink_waypoint_reached_t;

/**
 * @brief Pack a waypoint_reached message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param seq Sequence
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_waypoint_reached_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint16_t seq)
{
	mavlink_waypoint_reached_t *p = (mavlink_waypoint_reached_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_WAYPOINT_REACHED;

	p->seq = seq; // uint16_t:Sequence

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_WAYPOINT_REACHED_LEN);
}

/**
 * @brief Pack a waypoint_reached message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param seq Sequence
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_waypoint_reached_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint16_t seq)
{
	mavlink_waypoint_reached_t *p = (mavlink_waypoint_reached_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_WAYPOINT_REACHED;

	p->seq = seq; // uint16_t:Sequence

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_WAYPOINT_REACHED_LEN);
}

/**
 * @brief Encode a waypoint_reached struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param waypoint_reached C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_waypoint_reached_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_waypoint_reached_t* waypoint_reached)
{
	return mavlink_msg_waypoint_reached_pack(system_id, component_id, msg, waypoint_reached->seq);
}

/**
 * @brief Send a waypoint_reached message
 * @param chan MAVLink channel to send the message
 *
 * @param seq Sequence
 */


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_waypoint_reached_send(mavlink_channel_t chan, uint16_t seq)
{
	mavlink_header_t hdr;
	mavlink_waypoint_reached_t payload;
	uint16_t checksum;
	mavlink_waypoint_reached_t *p = &payload;

	p->seq = seq; // uint16_t:Sequence

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_WAYPOINT_REACHED_LEN;
	hdr.msgid = MAVLINK_MSG_ID_WAYPOINT_REACHED;
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
// MESSAGE WAYPOINT_REACHED UNPACKING

/**
 * @brief Get field seq from waypoint_reached message
 *
 * @return Sequence
 */
static inline uint16_t mavlink_msg_waypoint_reached_get_seq(const mavlink_message_t* msg)
{
	mavlink_waypoint_reached_t *p = (mavlink_waypoint_reached_t *)&msg->payload[0];
	return (uint16_t)(p->seq);
}

/**
 * @brief Decode a waypoint_reached message into a struct
 *
 * @param msg The message to decode
 * @param waypoint_reached C-struct to decode the message contents into
 */
static inline void mavlink_msg_waypoint_reached_decode(const mavlink_message_t* msg, mavlink_waypoint_reached_t* waypoint_reached)
{
	memcpy( waypoint_reached, msg->payload, sizeof(mavlink_waypoint_reached_t));
}
