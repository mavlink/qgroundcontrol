// MESSAGE SET_NAV_MODE PACKING

#define MAVLINK_MSG_ID_SET_NAV_MODE 12
#define MAVLINK_MSG_ID_SET_NAV_MODE_LEN 2
#define MAVLINK_MSG_12_LEN 2

typedef struct __mavlink_set_nav_mode_t 
{
	uint8_t target; ///< The system setting the mode
	uint8_t nav_mode; ///< The new navigation mode

} mavlink_set_nav_mode_t;

/**
 * @brief Pack a set_nav_mode message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target The system setting the mode
 * @param nav_mode The new navigation mode
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_set_nav_mode_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t target, uint8_t nav_mode)
{
	mavlink_set_nav_mode_t *p = (mavlink_set_nav_mode_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SET_NAV_MODE;

	p->target = target; // uint8_t:The system setting the mode
	p->nav_mode = nav_mode; // uint8_t:The new navigation mode

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_SET_NAV_MODE_LEN);
}

/**
 * @brief Pack a set_nav_mode message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target The system setting the mode
 * @param nav_mode The new navigation mode
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_set_nav_mode_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t target, uint8_t nav_mode)
{
	mavlink_set_nav_mode_t *p = (mavlink_set_nav_mode_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SET_NAV_MODE;

	p->target = target; // uint8_t:The system setting the mode
	p->nav_mode = nav_mode; // uint8_t:The new navigation mode

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_SET_NAV_MODE_LEN);
}

/**
 * @brief Encode a set_nav_mode struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param set_nav_mode C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_set_nav_mode_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_set_nav_mode_t* set_nav_mode)
{
	return mavlink_msg_set_nav_mode_pack(system_id, component_id, msg, set_nav_mode->target, set_nav_mode->nav_mode);
}

/**
 * @brief Send a set_nav_mode message
 * @param chan MAVLink channel to send the message
 *
 * @param target The system setting the mode
 * @param nav_mode The new navigation mode
 */


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_set_nav_mode_send(mavlink_channel_t chan, uint8_t target, uint8_t nav_mode)
{
	mavlink_header_t hdr;
	mavlink_set_nav_mode_t payload;
	uint16_t checksum;
	mavlink_set_nav_mode_t *p = &payload;

	p->target = target; // uint8_t:The system setting the mode
	p->nav_mode = nav_mode; // uint8_t:The new navigation mode

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_SET_NAV_MODE_LEN;
	hdr.msgid = MAVLINK_MSG_ID_SET_NAV_MODE;
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
// MESSAGE SET_NAV_MODE UNPACKING

/**
 * @brief Get field target from set_nav_mode message
 *
 * @return The system setting the mode
 */
static inline uint8_t mavlink_msg_set_nav_mode_get_target(const mavlink_message_t* msg)
{
	mavlink_set_nav_mode_t *p = (mavlink_set_nav_mode_t *)&msg->payload[0];
	return (uint8_t)(p->target);
}

/**
 * @brief Get field nav_mode from set_nav_mode message
 *
 * @return The new navigation mode
 */
static inline uint8_t mavlink_msg_set_nav_mode_get_nav_mode(const mavlink_message_t* msg)
{
	mavlink_set_nav_mode_t *p = (mavlink_set_nav_mode_t *)&msg->payload[0];
	return (uint8_t)(p->nav_mode);
}

/**
 * @brief Decode a set_nav_mode message into a struct
 *
 * @param msg The message to decode
 * @param set_nav_mode C-struct to decode the message contents into
 */
static inline void mavlink_msg_set_nav_mode_decode(const mavlink_message_t* msg, mavlink_set_nav_mode_t* set_nav_mode)
{
	memcpy( set_nav_mode, msg->payload, sizeof(mavlink_set_nav_mode_t));
}
