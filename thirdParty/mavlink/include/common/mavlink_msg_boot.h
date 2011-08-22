// MESSAGE BOOT PACKING

#define MAVLINK_MSG_ID_BOOT 1
#define MAVLINK_MSG_ID_BOOT_LEN 4
#define MAVLINK_MSG_1_LEN 4
#define MAVLINK_MSG_ID_BOOT_KEY 0xF9
#define MAVLINK_MSG_1_KEY 0xF9

typedef struct __mavlink_boot_t 
{
	uint32_t version;	///< The onboard software version

} mavlink_boot_t;

/**
 * @brief Pack a boot message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param version The onboard software version
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_boot_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint32_t version)
{
	mavlink_boot_t *p = (mavlink_boot_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_BOOT;

	p->version = version;	// uint32_t:The onboard software version

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_BOOT_LEN);
}

/**
 * @brief Pack a boot message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param version The onboard software version
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_boot_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint32_t version)
{
	mavlink_boot_t *p = (mavlink_boot_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_BOOT;

	p->version = version;	// uint32_t:The onboard software version

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_BOOT_LEN);
}

/**
 * @brief Encode a boot struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param boot C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_boot_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_boot_t* boot)
{
	return mavlink_msg_boot_pack(system_id, component_id, msg, boot->version);
}


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
/**
 * @brief Send a boot message
 * @param chan MAVLink channel to send the message
 *
 * @param version The onboard software version
 */
static inline void mavlink_msg_boot_send(mavlink_channel_t chan, uint32_t version)
{
	mavlink_header_t hdr;
	mavlink_boot_t payload;

	MAVLINK_BUFFER_CHECK_START( chan, MAVLINK_MSG_ID_BOOT_LEN )
	payload.version = version;	// uint32_t:The onboard software version

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_BOOT_LEN;
	hdr.msgid = MAVLINK_MSG_ID_BOOT;
	hdr.sysid = mavlink_system.sysid;
	hdr.compid = mavlink_system.compid;
	hdr.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = hdr.seq + 1;
	mavlink_send_mem(chan, (uint8_t *)&hdr.STX, MAVLINK_NUM_HEADER_BYTES );
	mavlink_send_mem(chan, (uint8_t *)&payload, sizeof(payload) );

	crc_init(&hdr.ck);
	crc_calculate_mem((uint8_t *)&hdr.len, &hdr.ck, MAVLINK_CORE_HEADER_LEN);
	crc_calculate_mem((uint8_t *)&payload, &hdr.ck, hdr.len );
	crc_accumulate( 0xF9, &hdr.ck); /// include key in X25 checksum
	mavlink_send_mem(chan, (uint8_t *)&hdr.ck, MAVLINK_NUM_CHECKSUM_BYTES);
	MAVLINK_BUFFER_CHECK_END
}

#endif
// MESSAGE BOOT UNPACKING

/**
 * @brief Get field version from boot message
 *
 * @return The onboard software version
 */
static inline uint32_t mavlink_msg_boot_get_version(const mavlink_message_t* msg)
{
	mavlink_boot_t *p = (mavlink_boot_t *)&msg->payload[0];
	return (uint32_t)(p->version);
}

/**
 * @brief Decode a boot message into a struct
 *
 * @param msg The message to decode
 * @param boot C-struct to decode the message contents into
 */
static inline void mavlink_msg_boot_decode(const mavlink_message_t* msg, mavlink_boot_t* boot)
{
	memcpy( boot, msg->payload, sizeof(mavlink_boot_t));
}
