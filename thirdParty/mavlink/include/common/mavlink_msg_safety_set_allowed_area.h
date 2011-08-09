// MESSAGE SAFETY_SET_ALLOWED_AREA PACKING

#define MAVLINK_MSG_ID_SAFETY_SET_ALLOWED_AREA 53
#define MAVLINK_MSG_ID_SAFETY_SET_ALLOWED_AREA_LEN 27
#define MAVLINK_MSG_53_LEN 27

typedef struct __mavlink_safety_set_allowed_area_t 
{
	float p1x; ///< x position 1 / Latitude 1
	float p1y; ///< y position 1 / Longitude 1
	float p1z; ///< z position 1 / Altitude 1
	float p2x; ///< x position 2 / Latitude 2
	float p2y; ///< y position 2 / Longitude 2
	float p2z; ///< z position 2 / Altitude 2
	uint8_t target_system; ///< System ID
	uint8_t target_component; ///< Component ID
	uint8_t frame; ///< Coordinate frame, as defined by MAV_FRAME enum in mavlink_types.h. Can be either global, GPS, right-handed with Z axis up or local, right handed, Z axis down.

} mavlink_safety_set_allowed_area_t;

/**
 * @brief Pack a safety_set_allowed_area message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param frame Coordinate frame, as defined by MAV_FRAME enum in mavlink_types.h. Can be either global, GPS, right-handed with Z axis up or local, right handed, Z axis down.
 * @param p1x x position 1 / Latitude 1
 * @param p1y y position 1 / Longitude 1
 * @param p1z z position 1 / Altitude 1
 * @param p2x x position 2 / Latitude 2
 * @param p2y y position 2 / Longitude 2
 * @param p2z z position 2 / Altitude 2
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_safety_set_allowed_area_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component, uint8_t frame, float p1x, float p1y, float p1z, float p2x, float p2y, float p2z)
{
	mavlink_safety_set_allowed_area_t *p = (mavlink_safety_set_allowed_area_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SAFETY_SET_ALLOWED_AREA;

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	p->frame = frame; // uint8_t:Coordinate frame, as defined by MAV_FRAME enum in mavlink_types.h. Can be either global, GPS, right-handed with Z axis up or local, right handed, Z axis down.
	p->p1x = p1x; // float:x position 1 / Latitude 1
	p->p1y = p1y; // float:y position 1 / Longitude 1
	p->p1z = p1z; // float:z position 1 / Altitude 1
	p->p2x = p2x; // float:x position 2 / Latitude 2
	p->p2y = p2y; // float:y position 2 / Longitude 2
	p->p2z = p2z; // float:z position 2 / Altitude 2

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_SAFETY_SET_ALLOWED_AREA_LEN);
}

/**
 * @brief Pack a safety_set_allowed_area message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system System ID
 * @param target_component Component ID
 * @param frame Coordinate frame, as defined by MAV_FRAME enum in mavlink_types.h. Can be either global, GPS, right-handed with Z axis up or local, right handed, Z axis down.
 * @param p1x x position 1 / Latitude 1
 * @param p1y y position 1 / Longitude 1
 * @param p1z z position 1 / Altitude 1
 * @param p2x x position 2 / Latitude 2
 * @param p2y y position 2 / Longitude 2
 * @param p2z z position 2 / Altitude 2
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_safety_set_allowed_area_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component, uint8_t frame, float p1x, float p1y, float p1z, float p2x, float p2y, float p2z)
{
	mavlink_safety_set_allowed_area_t *p = (mavlink_safety_set_allowed_area_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SAFETY_SET_ALLOWED_AREA;

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	p->frame = frame; // uint8_t:Coordinate frame, as defined by MAV_FRAME enum in mavlink_types.h. Can be either global, GPS, right-handed with Z axis up or local, right handed, Z axis down.
	p->p1x = p1x; // float:x position 1 / Latitude 1
	p->p1y = p1y; // float:y position 1 / Longitude 1
	p->p1z = p1z; // float:z position 1 / Altitude 1
	p->p2x = p2x; // float:x position 2 / Latitude 2
	p->p2y = p2y; // float:y position 2 / Longitude 2
	p->p2z = p2z; // float:z position 2 / Altitude 2

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_SAFETY_SET_ALLOWED_AREA_LEN);
}

/**
 * @brief Encode a safety_set_allowed_area struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param safety_set_allowed_area C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_safety_set_allowed_area_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_safety_set_allowed_area_t* safety_set_allowed_area)
{
	return mavlink_msg_safety_set_allowed_area_pack(system_id, component_id, msg, safety_set_allowed_area->target_system, safety_set_allowed_area->target_component, safety_set_allowed_area->frame, safety_set_allowed_area->p1x, safety_set_allowed_area->p1y, safety_set_allowed_area->p1z, safety_set_allowed_area->p2x, safety_set_allowed_area->p2y, safety_set_allowed_area->p2z);
}

/**
 * @brief Send a safety_set_allowed_area message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param frame Coordinate frame, as defined by MAV_FRAME enum in mavlink_types.h. Can be either global, GPS, right-handed with Z axis up or local, right handed, Z axis down.
 * @param p1x x position 1 / Latitude 1
 * @param p1y y position 1 / Longitude 1
 * @param p1z z position 1 / Altitude 1
 * @param p2x x position 2 / Latitude 2
 * @param p2y y position 2 / Longitude 2
 * @param p2z z position 2 / Altitude 2
 */


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_safety_set_allowed_area_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, uint8_t frame, float p1x, float p1y, float p1z, float p2x, float p2y, float p2z)
{
	mavlink_header_t hdr;
	mavlink_safety_set_allowed_area_t payload;
	uint16_t checksum;
	mavlink_safety_set_allowed_area_t *p = &payload;

	p->target_system = target_system; // uint8_t:System ID
	p->target_component = target_component; // uint8_t:Component ID
	p->frame = frame; // uint8_t:Coordinate frame, as defined by MAV_FRAME enum in mavlink_types.h. Can be either global, GPS, right-handed with Z axis up or local, right handed, Z axis down.
	p->p1x = p1x; // float:x position 1 / Latitude 1
	p->p1y = p1y; // float:y position 1 / Longitude 1
	p->p1z = p1z; // float:z position 1 / Altitude 1
	p->p2x = p2x; // float:x position 2 / Latitude 2
	p->p2y = p2y; // float:y position 2 / Longitude 2
	p->p2z = p2z; // float:z position 2 / Altitude 2

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_SAFETY_SET_ALLOWED_AREA_LEN;
	hdr.msgid = MAVLINK_MSG_ID_SAFETY_SET_ALLOWED_AREA;
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
// MESSAGE SAFETY_SET_ALLOWED_AREA UNPACKING

/**
 * @brief Get field target_system from safety_set_allowed_area message
 *
 * @return System ID
 */
static inline uint8_t mavlink_msg_safety_set_allowed_area_get_target_system(const mavlink_message_t* msg)
{
	mavlink_safety_set_allowed_area_t *p = (mavlink_safety_set_allowed_area_t *)&msg->payload[0];
	return (uint8_t)(p->target_system);
}

/**
 * @brief Get field target_component from safety_set_allowed_area message
 *
 * @return Component ID
 */
static inline uint8_t mavlink_msg_safety_set_allowed_area_get_target_component(const mavlink_message_t* msg)
{
	mavlink_safety_set_allowed_area_t *p = (mavlink_safety_set_allowed_area_t *)&msg->payload[0];
	return (uint8_t)(p->target_component);
}

/**
 * @brief Get field frame from safety_set_allowed_area message
 *
 * @return Coordinate frame, as defined by MAV_FRAME enum in mavlink_types.h. Can be either global, GPS, right-handed with Z axis up or local, right handed, Z axis down.
 */
static inline uint8_t mavlink_msg_safety_set_allowed_area_get_frame(const mavlink_message_t* msg)
{
	mavlink_safety_set_allowed_area_t *p = (mavlink_safety_set_allowed_area_t *)&msg->payload[0];
	return (uint8_t)(p->frame);
}

/**
 * @brief Get field p1x from safety_set_allowed_area message
 *
 * @return x position 1 / Latitude 1
 */
static inline float mavlink_msg_safety_set_allowed_area_get_p1x(const mavlink_message_t* msg)
{
	mavlink_safety_set_allowed_area_t *p = (mavlink_safety_set_allowed_area_t *)&msg->payload[0];
	return (float)(p->p1x);
}

/**
 * @brief Get field p1y from safety_set_allowed_area message
 *
 * @return y position 1 / Longitude 1
 */
static inline float mavlink_msg_safety_set_allowed_area_get_p1y(const mavlink_message_t* msg)
{
	mavlink_safety_set_allowed_area_t *p = (mavlink_safety_set_allowed_area_t *)&msg->payload[0];
	return (float)(p->p1y);
}

/**
 * @brief Get field p1z from safety_set_allowed_area message
 *
 * @return z position 1 / Altitude 1
 */
static inline float mavlink_msg_safety_set_allowed_area_get_p1z(const mavlink_message_t* msg)
{
	mavlink_safety_set_allowed_area_t *p = (mavlink_safety_set_allowed_area_t *)&msg->payload[0];
	return (float)(p->p1z);
}

/**
 * @brief Get field p2x from safety_set_allowed_area message
 *
 * @return x position 2 / Latitude 2
 */
static inline float mavlink_msg_safety_set_allowed_area_get_p2x(const mavlink_message_t* msg)
{
	mavlink_safety_set_allowed_area_t *p = (mavlink_safety_set_allowed_area_t *)&msg->payload[0];
	return (float)(p->p2x);
}

/**
 * @brief Get field p2y from safety_set_allowed_area message
 *
 * @return y position 2 / Longitude 2
 */
static inline float mavlink_msg_safety_set_allowed_area_get_p2y(const mavlink_message_t* msg)
{
	mavlink_safety_set_allowed_area_t *p = (mavlink_safety_set_allowed_area_t *)&msg->payload[0];
	return (float)(p->p2y);
}

/**
 * @brief Get field p2z from safety_set_allowed_area message
 *
 * @return z position 2 / Altitude 2
 */
static inline float mavlink_msg_safety_set_allowed_area_get_p2z(const mavlink_message_t* msg)
{
	mavlink_safety_set_allowed_area_t *p = (mavlink_safety_set_allowed_area_t *)&msg->payload[0];
	return (float)(p->p2z);
}

/**
 * @brief Decode a safety_set_allowed_area message into a struct
 *
 * @param msg The message to decode
 * @param safety_set_allowed_area C-struct to decode the message contents into
 */
static inline void mavlink_msg_safety_set_allowed_area_decode(const mavlink_message_t* msg, mavlink_safety_set_allowed_area_t* safety_set_allowed_area)
{
	memcpy( safety_set_allowed_area, msg->payload, sizeof(mavlink_safety_set_allowed_area_t));
}
