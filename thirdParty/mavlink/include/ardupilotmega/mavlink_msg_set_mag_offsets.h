// MESSAGE SET_MAG_OFFSETS PACKING

#define MAVLINK_MSG_ID_SET_MAG_OFFSETS 151
#define MAVLINK_MSG_ID_SET_MAG_OFFSETS_LEN 8
#define MAVLINK_MSG_151_LEN 8
#define MAVLINK_MSG_ID_SET_MAG_OFFSETS_KEY 0x41
#define MAVLINK_MSG_151_KEY 0x41

typedef struct __mavlink_set_mag_offsets_t 
{
	int16_t mag_ofs_x;	///< magnetometer X offset
	int16_t mag_ofs_y;	///< magnetometer Y offset
	int16_t mag_ofs_z;	///< magnetometer Z offset
	uint8_t target_system;	///< System ID
	uint8_t target_component;	///< Component ID

} mavlink_set_mag_offsets_t;

/**
 * @brief Pack a set_mag_offsets message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param mag_ofs_x magnetometer X offset
 * @param mag_ofs_y magnetometer Y offset
 * @param mag_ofs_z magnetometer Z offset
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_set_mag_offsets_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component, int16_t mag_ofs_x, int16_t mag_ofs_y, int16_t mag_ofs_z)
{
	mavlink_set_mag_offsets_t *p = (mavlink_set_mag_offsets_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SET_MAG_OFFSETS;

	p->target_system = target_system;	// uint8_t:System ID
	p->target_component = target_component;	// uint8_t:Component ID
	p->mag_ofs_x = mag_ofs_x;	// int16_t:magnetometer X offset
	p->mag_ofs_y = mag_ofs_y;	// int16_t:magnetometer Y offset
	p->mag_ofs_z = mag_ofs_z;	// int16_t:magnetometer Z offset

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_SET_MAG_OFFSETS_LEN);
}

/**
 * @brief Pack a set_mag_offsets message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system System ID
 * @param target_component Component ID
 * @param mag_ofs_x magnetometer X offset
 * @param mag_ofs_y magnetometer Y offset
 * @param mag_ofs_z magnetometer Z offset
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_set_mag_offsets_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component, int16_t mag_ofs_x, int16_t mag_ofs_y, int16_t mag_ofs_z)
{
	mavlink_set_mag_offsets_t *p = (mavlink_set_mag_offsets_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_SET_MAG_OFFSETS;

	p->target_system = target_system;	// uint8_t:System ID
	p->target_component = target_component;	// uint8_t:Component ID
	p->mag_ofs_x = mag_ofs_x;	// int16_t:magnetometer X offset
	p->mag_ofs_y = mag_ofs_y;	// int16_t:magnetometer Y offset
	p->mag_ofs_z = mag_ofs_z;	// int16_t:magnetometer Z offset

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_SET_MAG_OFFSETS_LEN);
}

/**
 * @brief Encode a set_mag_offsets struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param set_mag_offsets C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_set_mag_offsets_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_set_mag_offsets_t* set_mag_offsets)
{
	return mavlink_msg_set_mag_offsets_pack(system_id, component_id, msg, set_mag_offsets->target_system, set_mag_offsets->target_component, set_mag_offsets->mag_ofs_x, set_mag_offsets->mag_ofs_y, set_mag_offsets->mag_ofs_z);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a set_mag_offsets message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param mag_ofs_x magnetometer X offset
 * @param mag_ofs_y magnetometer Y offset
 * @param mag_ofs_z magnetometer Z offset
 */
static inline void mavlink_msg_set_mag_offsets_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, int16_t mag_ofs_x, int16_t mag_ofs_y, int16_t mag_ofs_z)
{
	mavlink_header_t hdr;
	mavlink_set_mag_offsets_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_SET_MAG_OFFSETS_LEN )
	payload.target_system = target_system;	// uint8_t:System ID
	payload.target_component = target_component;	// uint8_t:Component ID
	payload.mag_ofs_x = mag_ofs_x;	// int16_t:magnetometer X offset
	payload.mag_ofs_y = mag_ofs_y;	// int16_t:magnetometer Y offset
	payload.mag_ofs_z = mag_ofs_z;	// int16_t:magnetometer Z offset

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_SET_MAG_OFFSETS_LEN;
	hdr.msgid = MAVLINK_MSG_ID_SET_MAG_OFFSETS;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0x41, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE SET_MAG_OFFSETS UNPACKING

/**
 * @brief Get field target_system from set_mag_offsets message
 *
 * @return System ID
 */
static inline uint8_t mavlink_msg_set_mag_offsets_get_target_system(const mavlink_message_t* msg)
{
	mavlink_set_mag_offsets_t *p = (mavlink_set_mag_offsets_t *)&msg->payload[0];
	return (uint8_t)(p->target_system);
}

/**
 * @brief Get field target_component from set_mag_offsets message
 *
 * @return Component ID
 */
static inline uint8_t mavlink_msg_set_mag_offsets_get_target_component(const mavlink_message_t* msg)
{
	mavlink_set_mag_offsets_t *p = (mavlink_set_mag_offsets_t *)&msg->payload[0];
	return (uint8_t)(p->target_component);
}

/**
 * @brief Get field mag_ofs_x from set_mag_offsets message
 *
 * @return magnetometer X offset
 */
static inline int16_t mavlink_msg_set_mag_offsets_get_mag_ofs_x(const mavlink_message_t* msg)
{
	mavlink_set_mag_offsets_t *p = (mavlink_set_mag_offsets_t *)&msg->payload[0];
	return (int16_t)(p->mag_ofs_x);
}

/**
 * @brief Get field mag_ofs_y from set_mag_offsets message
 *
 * @return magnetometer Y offset
 */
static inline int16_t mavlink_msg_set_mag_offsets_get_mag_ofs_y(const mavlink_message_t* msg)
{
	mavlink_set_mag_offsets_t *p = (mavlink_set_mag_offsets_t *)&msg->payload[0];
	return (int16_t)(p->mag_ofs_y);
}

/**
 * @brief Get field mag_ofs_z from set_mag_offsets message
 *
 * @return magnetometer Z offset
 */
static inline int16_t mavlink_msg_set_mag_offsets_get_mag_ofs_z(const mavlink_message_t* msg)
{
	mavlink_set_mag_offsets_t *p = (mavlink_set_mag_offsets_t *)&msg->payload[0];
	return (int16_t)(p->mag_ofs_z);
}

/**
 * @brief Decode a set_mag_offsets message into a struct
 *
 * @param msg The message to decode
 * @param set_mag_offsets C-struct to decode the message contents into
 */
static inline void mavlink_msg_set_mag_offsets_decode(const mavlink_message_t* msg, mavlink_set_mag_offsets_t* set_mag_offsets)
{
	memcpy( set_mag_offsets, msg->payload, sizeof(mavlink_set_mag_offsets_t));
}
