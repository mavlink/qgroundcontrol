// MESSAGE SAFETY_ALLOWED_AREA PACKING

#define MAVLINK_MSG_ID_SAFETY_ALLOWED_AREA 54
#define MAVLINK_MSG_ID_SAFETY_ALLOWED_AREA_LEN 25
#define MAVLINK_MSG_54_LEN 25
#define MAVLINK_MSG_ID_SAFETY_ALLOWED_AREA_KEY 0xEA
#define MAVLINK_MSG_54_KEY 0xEA

typedef struct __mavlink_safety_allowed_area_t 
{
	float p1x;	///< x position 1 / Latitude 1
	float p1y;	///< y position 1 / Longitude 1
	float p1z;	///< z position 1 / Altitude 1
	float p2x;	///< x position 2 / Latitude 2
	float p2y;	///< y position 2 / Longitude 2
	float p2z;	///< z position 2 / Altitude 2
	uint8_t frame;	///< Coordinate frame, as defined by MAV_FRAME enum in mavlink_types.h. Can be either global, GPS, right-handed with Z axis up or local, right handed, Z axis down.

} mavlink_safety_allowed_area_t;

/**
 * @brief Pack a safety_allowed_area message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param frame Coordinate frame, as defined by MAV_FRAME enum in mavlink_types.h. Can be either global, GPS, right-handed with Z axis up or local, right handed, Z axis down.
 * @param p1x x position 1 / Latitude 1
 * @param p1y y position 1 / Longitude 1
 * @param p1z z position 1 / Altitude 1
 * @param p2x x position 2 / Latitude 2
 * @param p2y y position 2 / Longitude 2
 * @param p2z z position 2 / Altitude 2
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_safety_allowed_area_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t frame, float p1x, float p1y, float p1z, float p2x, float p2y, float p2z)
{
	mavlink_safety_allowed_area_t *p = (mavlink_safety_allowed_area_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SAFETY_ALLOWED_AREA;

	p->frame = frame;	// uint8_t:Coordinate frame, as defined by MAV_FRAME enum in mavlink_types.h. Can be either global, GPS, right-handed with Z axis up or local, right handed, Z axis down.
	p->p1x = p1x;	// float:x position 1 / Latitude 1
	p->p1y = p1y;	// float:y position 1 / Longitude 1
	p->p1z = p1z;	// float:z position 1 / Altitude 1
	p->p2x = p2x;	// float:x position 2 / Latitude 2
	p->p2y = p2y;	// float:y position 2 / Longitude 2
	p->p2z = p2z;	// float:z position 2 / Altitude 2

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_SAFETY_ALLOWED_AREA_LEN);
}

/**
 * @brief Pack a safety_allowed_area message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param frame Coordinate frame, as defined by MAV_FRAME enum in mavlink_types.h. Can be either global, GPS, right-handed with Z axis up or local, right handed, Z axis down.
 * @param p1x x position 1 / Latitude 1
 * @param p1y y position 1 / Longitude 1
 * @param p1z z position 1 / Altitude 1
 * @param p2x x position 2 / Latitude 2
 * @param p2y y position 2 / Longitude 2
 * @param p2z z position 2 / Altitude 2
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_safety_allowed_area_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t frame, float p1x, float p1y, float p1z, float p2x, float p2y, float p2z)
{
	mavlink_safety_allowed_area_t *p = (mavlink_safety_allowed_area_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SAFETY_ALLOWED_AREA;

	p->frame = frame;	// uint8_t:Coordinate frame, as defined by MAV_FRAME enum in mavlink_types.h. Can be either global, GPS, right-handed with Z axis up or local, right handed, Z axis down.
	p->p1x = p1x;	// float:x position 1 / Latitude 1
	p->p1y = p1y;	// float:y position 1 / Longitude 1
	p->p1z = p1z;	// float:z position 1 / Altitude 1
	p->p2x = p2x;	// float:x position 2 / Latitude 2
	p->p2y = p2y;	// float:y position 2 / Longitude 2
	p->p2z = p2z;	// float:z position 2 / Altitude 2

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_SAFETY_ALLOWED_AREA_LEN);
}

/**
 * @brief Encode a safety_allowed_area struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param safety_allowed_area C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_safety_allowed_area_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_safety_allowed_area_t* safety_allowed_area)
{
	return mavlink_msg_safety_allowed_area_pack(system_id, component_id, msg, safety_allowed_area->frame, safety_allowed_area->p1x, safety_allowed_area->p1y, safety_allowed_area->p1z, safety_allowed_area->p2x, safety_allowed_area->p2y, safety_allowed_area->p2z);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a safety_allowed_area message
 * @param chan MAVLink channel to send the message
 *
 * @param frame Coordinate frame, as defined by MAV_FRAME enum in mavlink_types.h. Can be either global, GPS, right-handed with Z axis up or local, right handed, Z axis down.
 * @param p1x x position 1 / Latitude 1
 * @param p1y y position 1 / Longitude 1
 * @param p1z z position 1 / Altitude 1
 * @param p2x x position 2 / Latitude 2
 * @param p2y y position 2 / Longitude 2
 * @param p2z z position 2 / Altitude 2
 */
static inline void mavlink_msg_safety_allowed_area_send(mavlink_channel_t chan, uint8_t frame, float p1x, float p1y, float p1z, float p2x, float p2y, float p2z)
{
	mavlink_header_t hdr;
	mavlink_safety_allowed_area_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_SAFETY_ALLOWED_AREA_LEN )
	payload.frame = frame;	// uint8_t:Coordinate frame, as defined by MAV_FRAME enum in mavlink_types.h. Can be either global, GPS, right-handed with Z axis up or local, right handed, Z axis down.
	payload.p1x = p1x;	// float:x position 1 / Latitude 1
	payload.p1y = p1y;	// float:y position 1 / Longitude 1
	payload.p1z = p1z;	// float:z position 1 / Altitude 1
	payload.p2x = p2x;	// float:x position 2 / Latitude 2
	payload.p2y = p2y;	// float:y position 2 / Longitude 2
	payload.p2z = p2z;	// float:z position 2 / Altitude 2

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_SAFETY_ALLOWED_AREA_LEN;
	hdr.msgid = MAVLINK_MSG_ID_SAFETY_ALLOWED_AREA;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0xEA, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE SAFETY_ALLOWED_AREA UNPACKING

/**
 * @brief Get field frame from safety_allowed_area message
 *
 * @return Coordinate frame, as defined by MAV_FRAME enum in mavlink_types.h. Can be either global, GPS, right-handed with Z axis up or local, right handed, Z axis down.
 */
static inline uint8_t mavlink_msg_safety_allowed_area_get_frame(const mavlink_message_t* msg)
{
	mavlink_safety_allowed_area_t *p = (mavlink_safety_allowed_area_t *)&msg->payload[0];
	return (uint8_t)(p->frame);
}

/**
 * @brief Get field p1x from safety_allowed_area message
 *
 * @return x position 1 / Latitude 1
 */
static inline float mavlink_msg_safety_allowed_area_get_p1x(const mavlink_message_t* msg)
{
	mavlink_safety_allowed_area_t *p = (mavlink_safety_allowed_area_t *)&msg->payload[0];
	return (float)(p->p1x);
}

/**
 * @brief Get field p1y from safety_allowed_area message
 *
 * @return y position 1 / Longitude 1
 */
static inline float mavlink_msg_safety_allowed_area_get_p1y(const mavlink_message_t* msg)
{
	mavlink_safety_allowed_area_t *p = (mavlink_safety_allowed_area_t *)&msg->payload[0];
	return (float)(p->p1y);
}

/**
 * @brief Get field p1z from safety_allowed_area message
 *
 * @return z position 1 / Altitude 1
 */
static inline float mavlink_msg_safety_allowed_area_get_p1z(const mavlink_message_t* msg)
{
	mavlink_safety_allowed_area_t *p = (mavlink_safety_allowed_area_t *)&msg->payload[0];
	return (float)(p->p1z);
}

/**
 * @brief Get field p2x from safety_allowed_area message
 *
 * @return x position 2 / Latitude 2
 */
static inline float mavlink_msg_safety_allowed_area_get_p2x(const mavlink_message_t* msg)
{
	mavlink_safety_allowed_area_t *p = (mavlink_safety_allowed_area_t *)&msg->payload[0];
	return (float)(p->p2x);
}

/**
 * @brief Get field p2y from safety_allowed_area message
 *
 * @return y position 2 / Longitude 2
 */
static inline float mavlink_msg_safety_allowed_area_get_p2y(const mavlink_message_t* msg)
{
	mavlink_safety_allowed_area_t *p = (mavlink_safety_allowed_area_t *)&msg->payload[0];
	return (float)(p->p2y);
}

/**
 * @brief Get field p2z from safety_allowed_area message
 *
 * @return z position 2 / Altitude 2
 */
static inline float mavlink_msg_safety_allowed_area_get_p2z(const mavlink_message_t* msg)
{
	mavlink_safety_allowed_area_t *p = (mavlink_safety_allowed_area_t *)&msg->payload[0];
	return (float)(p->p2z);
}

/**
 * @brief Decode a safety_allowed_area message into a struct
 *
 * @param msg The message to decode
 * @param safety_allowed_area C-struct to decode the message contents into
 */
static inline void mavlink_msg_safety_allowed_area_decode(const mavlink_message_t* msg, mavlink_safety_allowed_area_t* safety_allowed_area)
{
	memcpy( safety_allowed_area, msg->payload, sizeof(mavlink_safety_allowed_area_t));
}
