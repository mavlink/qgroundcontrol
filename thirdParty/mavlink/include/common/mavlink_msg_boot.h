// MESSAGE BOOT PACKING

#define MAVLINK_MSG_ID_BOOT 1
#define MAVLINK_MSG_ID_BOOT_LEN 4
#define MAVLINK_MSG_1_LEN 4

typedef struct __mavlink_boot_t 
{
	uint32_t version; ///< The onboard software version

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

	p->version = version; // uint32_t:The onboard software version

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

	p->version = version; // uint32_t:The onboard software version

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

/**
 * @brief Send a boot message
 * @param chan MAVLink channel to send the message
 *
 * @param version The onboard software version
 */


#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_boot_send(mavlink_channel_t chan, uint32_t version)
{
	mavlink_header_t hdr;
	mavlink_boot_t payload;
	uint16_t checksum;
	mavlink_boot_t *p = &payload;

	p->version = version; // uint32_t:The onboard software version

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_BOOT_LEN;
	hdr.msgid = MAVLINK_MSG_ID_BOOT;
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
