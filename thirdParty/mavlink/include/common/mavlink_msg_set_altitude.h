// MESSAGE SET_ALTITUDE PACKING

#define MAVLINK_MSG_ID_SET_ALTITUDE 65
#define MAVLINK_MSG_ID_SET_ALTITUDE_LEN 5
#define MAVLINK_MSG_65_LEN 5
#define MAVLINK_MSG_ID_SET_ALTITUDE_KEY 0x12
#define MAVLINK_MSG_65_KEY 0x12

typedef struct __mavlink_set_altitude_t 
{
	uint32_t mode;	///< The new altitude in meters
	uint8_t target;	///< The system setting the altitude

} mavlink_set_altitude_t;

/**
 * @brief Pack a set_altitude message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target The system setting the altitude
 * @param mode The new altitude in meters
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_set_altitude_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t target, uint32_t mode)
{
	mavlink_set_altitude_t *p = (mavlink_set_altitude_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SET_ALTITUDE;

	p->target = target;	// uint8_t:The system setting the altitude
	p->mode = mode;	// uint32_t:The new altitude in meters

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_SET_ALTITUDE_LEN);
}

/**
 * @brief Pack a set_altitude message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target The system setting the altitude
 * @param mode The new altitude in meters
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_set_altitude_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t target, uint32_t mode)
{
	mavlink_set_altitude_t *p = (mavlink_set_altitude_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SET_ALTITUDE;

	p->target = target;	// uint8_t:The system setting the altitude
	p->mode = mode;	// uint32_t:The new altitude in meters

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_SET_ALTITUDE_LEN);
}

/**
 * @brief Encode a set_altitude struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param set_altitude C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_set_altitude_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_set_altitude_t* set_altitude)
{
	return mavlink_msg_set_altitude_pack(system_id, component_id, msg, set_altitude->target, set_altitude->mode);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a set_altitude message
 * @param chan MAVLink channel to send the message
 *
 * @param target The system setting the altitude
 * @param mode The new altitude in meters
 */
static inline void mavlink_msg_set_altitude_send(mavlink_channel_t chan, uint8_t target, uint32_t mode)
{
	mavlink_header_t hdr;
	mavlink_set_altitude_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_SET_ALTITUDE_LEN )
	payload.target = target;	// uint8_t:The system setting the altitude
	payload.mode = mode;	// uint32_t:The new altitude in meters

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_SET_ALTITUDE_LEN;
	hdr.msgid = MAVLINK_MSG_ID_SET_ALTITUDE;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0x12, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE SET_ALTITUDE UNPACKING

/**
 * @brief Get field target from set_altitude message
 *
 * @return The system setting the altitude
 */
static inline uint8_t mavlink_msg_set_altitude_get_target(const mavlink_message_t* msg)
{
	mavlink_set_altitude_t *p = (mavlink_set_altitude_t *)&msg->payload[0];
	return (uint8_t)(p->target);
}

/**
 * @brief Get field mode from set_altitude message
 *
 * @return The new altitude in meters
 */
static inline uint32_t mavlink_msg_set_altitude_get_mode(const mavlink_message_t* msg)
{
	mavlink_set_altitude_t *p = (mavlink_set_altitude_t *)&msg->payload[0];
	return (uint32_t)(p->mode);
}

/**
 * @brief Decode a set_altitude message into a struct
 *
 * @param msg The message to decode
 * @param set_altitude C-struct to decode the message contents into
 */
static inline void mavlink_msg_set_altitude_decode(const mavlink_message_t* msg, mavlink_set_altitude_t* set_altitude)
{
	memcpy( set_altitude, msg->payload, sizeof(mavlink_set_altitude_t));
}
