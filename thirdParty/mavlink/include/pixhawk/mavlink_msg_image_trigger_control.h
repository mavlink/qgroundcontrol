// MESSAGE IMAGE_TRIGGER_CONTROL PACKING

#define MAVLINK_MSG_ID_IMAGE_TRIGGER_CONTROL 102
#define MAVLINK_MSG_ID_IMAGE_TRIGGER_CONTROL_LEN 1
#define MAVLINK_MSG_102_LEN 1
#define MAVLINK_MSG_ID_IMAGE_TRIGGER_CONTROL_KEY 0xEE
#define MAVLINK_MSG_102_KEY 0xEE

typedef struct __mavlink_image_trigger_control_t 
{
	uint8_t enable;	///< 0 to disable, 1 to enable

} mavlink_image_trigger_control_t;

/**
 * @brief Pack a image_trigger_control message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param enable 0 to disable, 1 to enable
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_image_trigger_control_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t enable)
{
	mavlink_image_trigger_control_t *p = (mavlink_image_trigger_control_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_IMAGE_TRIGGER_CONTROL;

	p->enable = enable;	// uint8_t:0 to disable, 1 to enable

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_IMAGE_TRIGGER_CONTROL_LEN);
}

/**
 * @brief Pack a image_trigger_control message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param enable 0 to disable, 1 to enable
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_image_trigger_control_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t enable)
{
	mavlink_image_trigger_control_t *p = (mavlink_image_trigger_control_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_IMAGE_TRIGGER_CONTROL;

	p->enable = enable;	// uint8_t:0 to disable, 1 to enable

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_IMAGE_TRIGGER_CONTROL_LEN);
}

/**
 * @brief Encode a image_trigger_control struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param image_trigger_control C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_image_trigger_control_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_image_trigger_control_t* image_trigger_control)
{
	return mavlink_msg_image_trigger_control_pack(system_id, component_id, msg, image_trigger_control->enable);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a image_trigger_control message
 * @param chan MAVLink channel to send the message
 *
 * @param enable 0 to disable, 1 to enable
 */
static inline void mavlink_msg_image_trigger_control_send(mavlink_channel_t chan, uint8_t enable)
{
	mavlink_header_t hdr;
	mavlink_image_trigger_control_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_IMAGE_TRIGGER_CONTROL_LEN )
	payload.enable = enable;	// uint8_t:0 to disable, 1 to enable

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_IMAGE_TRIGGER_CONTROL_LEN;
	hdr.msgid = MAVLINK_MSG_ID_IMAGE_TRIGGER_CONTROL;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0xEE, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE IMAGE_TRIGGER_CONTROL UNPACKING

/**
 * @brief Get field enable from image_trigger_control message
 *
 * @return 0 to disable, 1 to enable
 */
static inline uint8_t mavlink_msg_image_trigger_control_get_enable(const mavlink_message_t* msg)
{
	mavlink_image_trigger_control_t *p = (mavlink_image_trigger_control_t *)&msg->payload[0];
	return (uint8_t)(p->enable);
}

/**
 * @brief Decode a image_trigger_control message into a struct
 *
 * @param msg The message to decode
 * @param image_trigger_control C-struct to decode the message contents into
 */
static inline void mavlink_msg_image_trigger_control_decode(const mavlink_message_t* msg, mavlink_image_trigger_control_t* image_trigger_control)
{
	memcpy( image_trigger_control, msg->payload, sizeof(mavlink_image_trigger_control_t));
}
