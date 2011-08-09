// MESSAGE POSITION_CONTROLLER_OUTPUT PACKING

#define MAVLINK_MSG_ID_POSITION_CONTROLLER_OUTPUT 61
#define MAVLINK_MSG_ID_POSITION_CONTROLLER_OUTPUT_LEN 5
#define MAVLINK_MSG_61_LEN 5

typedef struct __mavlink_position_controller_output_t 
{
	uint8_t enabled; ///< 1: enabled, 0: disabled
	int8_t x; ///< Position x: -128: -100%, 127: +100%
	int8_t y; ///< Position y: -128: -100%, 127: +100%
	int8_t z; ///< Position z: -128: -100%, 127: +100%
	int8_t yaw; ///< Position yaw: -128: -100%, 127: +100%

} mavlink_position_controller_output_t;

/**
 * @brief Pack a position_controller_output message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param enabled 1: enabled, 0: disabled
 * @param x Position x: -128: -100%, 127: +100%
 * @param y Position y: -128: -100%, 127: +100%
 * @param z Position z: -128: -100%, 127: +100%
 * @param yaw Position yaw: -128: -100%, 127: +100%
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_position_controller_output_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t enabled, int8_t x, int8_t y, int8_t z, int8_t yaw)
{
	mavlink_position_controller_output_t *p = (mavlink_position_controller_output_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_POSITION_CONTROLLER_OUTPUT;

	p->enabled = enabled; // uint8_t:1: enabled, 0: disabled
	p->x = x; // int8_t:Position x: -128: -100%, 127: +100%
	p->y = y; // int8_t:Position y: -128: -100%, 127: +100%
	p->z = z; // int8_t:Position z: -128: -100%, 127: +100%
	p->yaw = yaw; // int8_t:Position yaw: -128: -100%, 127: +100%

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_POSITION_CONTROLLER_OUTPUT_LEN);
}

/**
 * @brief Pack a position_controller_output message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param enabled 1: enabled, 0: disabled
 * @param x Position x: -128: -100%, 127: +100%
 * @param y Position y: -128: -100%, 127: +100%
 * @param z Position z: -128: -100%, 127: +100%
 * @param yaw Position yaw: -128: -100%, 127: +100%
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_position_controller_output_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t enabled, int8_t x, int8_t y, int8_t z, int8_t yaw)
{
	mavlink_position_controller_output_t *p = (mavlink_position_controller_output_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_POSITION_CONTROLLER_OUTPUT;

	p->enabled = enabled; // uint8_t:1: enabled, 0: disabled
	p->x = x; // int8_t:Position x: -128: -100%, 127: +100%
	p->y = y; // int8_t:Position y: -128: -100%, 127: +100%
	p->z = z; // int8_t:Position z: -128: -100%, 127: +100%
	p->yaw = yaw; // int8_t:Position yaw: -128: -100%, 127: +100%

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_POSITION_CONTROLLER_OUTPUT_LEN);
}

/**
 * @brief Encode a position_controller_output struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param position_controller_output C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_position_controller_output_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_position_controller_output_t* position_controller_output)
{
	return mavlink_msg_position_controller_output_pack(system_id, component_id, msg, position_controller_output->enabled, position_controller_output->x, position_controller_output->y, position_controller_output->z, position_controller_output->yaw);
}

/**
 * @brief Send a position_controller_output message
 * @param chan MAVLink channel to send the message
 *
 * @param enabled 1: enabled, 0: disabled
 * @param x Position x: -128: -100%, 127: +100%
 * @param y Position y: -128: -100%, 127: +100%
 * @param z Position z: -128: -100%, 127: +100%
 * @param yaw Position yaw: -128: -100%, 127: +100%
 */


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_position_controller_output_send(mavlink_channel_t chan, uint8_t enabled, int8_t x, int8_t y, int8_t z, int8_t yaw)
{
	mavlink_header_t hdr;
	mavlink_position_controller_output_t payload;
	uint16_t checksum;
	mavlink_position_controller_output_t *p = &payload;

	p->enabled = enabled; // uint8_t:1: enabled, 0: disabled
	p->x = x; // int8_t:Position x: -128: -100%, 127: +100%
	p->y = y; // int8_t:Position y: -128: -100%, 127: +100%
	p->z = z; // int8_t:Position z: -128: -100%, 127: +100%
	p->yaw = yaw; // int8_t:Position yaw: -128: -100%, 127: +100%

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_POSITION_CONTROLLER_OUTPUT_LEN;
	hdr.msgid = MAVLINK_MSG_ID_POSITION_CONTROLLER_OUTPUT;
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
// MESSAGE POSITION_CONTROLLER_OUTPUT UNPACKING

/**
 * @brief Get field enabled from position_controller_output message
 *
 * @return 1: enabled, 0: disabled
 */
static inline uint8_t mavlink_msg_position_controller_output_get_enabled(const mavlink_message_t* msg)
{
	mavlink_position_controller_output_t *p = (mavlink_position_controller_output_t *)&msg->payload[0];
	return (uint8_t)(p->enabled);
}

/**
 * @brief Get field x from position_controller_output message
 *
 * @return Position x: -128: -100%, 127: +100%
 */
static inline int8_t mavlink_msg_position_controller_output_get_x(const mavlink_message_t* msg)
{
	mavlink_position_controller_output_t *p = (mavlink_position_controller_output_t *)&msg->payload[0];
	return (int8_t)(p->x);
}

/**
 * @brief Get field y from position_controller_output message
 *
 * @return Position y: -128: -100%, 127: +100%
 */
static inline int8_t mavlink_msg_position_controller_output_get_y(const mavlink_message_t* msg)
{
	mavlink_position_controller_output_t *p = (mavlink_position_controller_output_t *)&msg->payload[0];
	return (int8_t)(p->y);
}

/**
 * @brief Get field z from position_controller_output message
 *
 * @return Position z: -128: -100%, 127: +100%
 */
static inline int8_t mavlink_msg_position_controller_output_get_z(const mavlink_message_t* msg)
{
	mavlink_position_controller_output_t *p = (mavlink_position_controller_output_t *)&msg->payload[0];
	return (int8_t)(p->z);
}

/**
 * @brief Get field yaw from position_controller_output message
 *
 * @return Position yaw: -128: -100%, 127: +100%
 */
static inline int8_t mavlink_msg_position_controller_output_get_yaw(const mavlink_message_t* msg)
{
	mavlink_position_controller_output_t *p = (mavlink_position_controller_output_t *)&msg->payload[0];
	return (int8_t)(p->yaw);
}

/**
 * @brief Decode a position_controller_output message into a struct
 *
 * @param msg The message to decode
 * @param position_controller_output C-struct to decode the message contents into
 */
static inline void mavlink_msg_position_controller_output_decode(const mavlink_message_t* msg, mavlink_position_controller_output_t* position_controller_output)
{
	memcpy( position_controller_output, msg->payload, sizeof(mavlink_position_controller_output_t));
}
