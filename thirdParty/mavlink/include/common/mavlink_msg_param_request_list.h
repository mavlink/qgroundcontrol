// MESSAGE PARAM_REQUEST_LIST PACKING

#define MAVLINK_MSG_ID_PARAM_REQUEST_LIST 21
#define MAVLINK_MSG_ID_PARAM_REQUEST_LIST_LEN 2
#define MAVLINK_MSG_21_LEN 2
#define MAVLINK_MSG_ID_PARAM_REQUEST_LIST_KEY 0x22
#define MAVLINK_MSG_21_KEY 0x22

typedef struct __mavlink_param_request_list_t 
{
	uint8_t target_system;	///< System ID
	uint8_t target_component;	///< Component ID

} mavlink_param_request_list_t;

/**
 * @brief Pack a param_request_list message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_param_request_list_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component)
{
	mavlink_param_request_list_t *p = (mavlink_param_request_list_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_PARAM_REQUEST_LIST;

	p->target_system = target_system;	// uint8_t:System ID
	p->target_component = target_component;	// uint8_t:Component ID

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_PARAM_REQUEST_LIST_LEN);
}

/**
 * @brief Pack a param_request_list message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system System ID
 * @param target_component Component ID
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_param_request_list_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component)
{
	mavlink_param_request_list_t *p = (mavlink_param_request_list_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_PARAM_REQUEST_LIST;

	p->target_system = target_system;	// uint8_t:System ID
	p->target_component = target_component;	// uint8_t:Component ID

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_PARAM_REQUEST_LIST_LEN);
}

/**
 * @brief Encode a param_request_list struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param param_request_list C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_param_request_list_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_param_request_list_t* param_request_list)
{
	return mavlink_msg_param_request_list_pack(system_id, component_id, msg, param_request_list->target_system, param_request_list->target_component);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a param_request_list message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system System ID
 * @param target_component Component ID
 */
static inline void mavlink_msg_param_request_list_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component)
{
	mavlink_header_t hdr;
	mavlink_param_request_list_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_PARAM_REQUEST_LIST_LEN )
	payload.target_system = target_system;	// uint8_t:System ID
	payload.target_component = target_component;	// uint8_t:Component ID

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_PARAM_REQUEST_LIST_LEN;
	hdr.msgid = MAVLINK_MSG_ID_PARAM_REQUEST_LIST;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0x22, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE PARAM_REQUEST_LIST UNPACKING

/**
 * @brief Get field target_system from param_request_list message
 *
 * @return System ID
 */
static inline uint8_t mavlink_msg_param_request_list_get_target_system(const mavlink_message_t* msg)
{
	mavlink_param_request_list_t *p = (mavlink_param_request_list_t *)&msg->payload[0];
	return (uint8_t)(p->target_system);
}

/**
 * @brief Get field target_component from param_request_list message
 *
 * @return Component ID
 */
static inline uint8_t mavlink_msg_param_request_list_get_target_component(const mavlink_message_t* msg)
{
	mavlink_param_request_list_t *p = (mavlink_param_request_list_t *)&msg->payload[0];
	return (uint8_t)(p->target_component);
}

/**
 * @brief Decode a param_request_list message into a struct
 *
 * @param msg The message to decode
 * @param param_request_list C-struct to decode the message contents into
 */
static inline void mavlink_msg_param_request_list_decode(const mavlink_message_t* msg, mavlink_param_request_list_t* param_request_list)
{
	memcpy( param_request_list, msg->payload, sizeof(mavlink_param_request_list_t));
}
