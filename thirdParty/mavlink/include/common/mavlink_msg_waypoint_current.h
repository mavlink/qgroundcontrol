// MESSAGE WAYPOINT_CURRENT PACKING

#define MAVLINK_MSG_ID_WAYPOINT_CURRENT 42
#define MAVLINK_MSG_ID_WAYPOINT_CURRENT_LEN 2
#define MAVLINK_MSG_42_LEN 2
#define MAVLINK_MSG_ID_WAYPOINT_CURRENT_KEY 0xA6
#define MAVLINK_MSG_42_KEY 0xA6

typedef struct __mavlink_waypoint_current_t 
{
	uint16_t seq;	///< Sequence

} mavlink_waypoint_current_t;

/**
 * @brief Pack a waypoint_current message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param seq Sequence
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_waypoint_current_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint16_t seq)
{
	mavlink_waypoint_current_t *p = (mavlink_waypoint_current_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_WAYPOINT_CURRENT;

	p->seq = seq;	// uint16_t:Sequence

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_WAYPOINT_CURRENT_LEN);
}

/**
 * @brief Pack a waypoint_current message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param seq Sequence
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_waypoint_current_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint16_t seq)
{
	mavlink_waypoint_current_t *p = (mavlink_waypoint_current_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_WAYPOINT_CURRENT;

	p->seq = seq;	// uint16_t:Sequence

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_WAYPOINT_CURRENT_LEN);
}

/**
 * @brief Encode a waypoint_current struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param waypoint_current C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_waypoint_current_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_waypoint_current_t* waypoint_current)
{
	return mavlink_msg_waypoint_current_pack(system_id, component_id, msg, waypoint_current->seq);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a waypoint_current message
 * @param chan MAVLink channel to send the message
 *
 * @param seq Sequence
 */
static inline void mavlink_msg_waypoint_current_send(mavlink_channel_t chan, uint16_t seq)
{
	mavlink_header_t hdr;
	mavlink_waypoint_current_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_WAYPOINT_CURRENT_LEN )
	payload.seq = seq;	// uint16_t:Sequence

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_WAYPOINT_CURRENT_LEN;
	hdr.msgid = MAVLINK_MSG_ID_WAYPOINT_CURRENT;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0xA6, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE WAYPOINT_CURRENT UNPACKING

/**
 * @brief Get field seq from waypoint_current message
 *
 * @return Sequence
 */
static inline uint16_t mavlink_msg_waypoint_current_get_seq(const mavlink_message_t* msg)
{
	mavlink_waypoint_current_t *p = (mavlink_waypoint_current_t *)&msg->payload[0];
	return (uint16_t)(p->seq);
}

/**
 * @brief Decode a waypoint_current message into a struct
 *
 * @param msg The message to decode
 * @param waypoint_current C-struct to decode the message contents into
 */
static inline void mavlink_msg_waypoint_current_decode(const mavlink_message_t* msg, mavlink_waypoint_current_t* waypoint_current)
{
	memcpy( waypoint_current, msg->payload, sizeof(mavlink_waypoint_current_t));
}
