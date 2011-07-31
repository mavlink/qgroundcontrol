// MESSAGE STATUSTEXT PACKING

#define MAVLINK_MSG_ID_STATUSTEXT 254
#define MAVLINK_MSG_ID_STATUSTEXT_LEN 51
#define MAVLINK_MSG_254_LEN 51

typedef struct __mavlink_statustext_t 
{
	uint8_t severity; ///< Severity of status, 0 = info message, 255 = critical fault
	char text[50]; ///< Status text message, without null termination character

} mavlink_statustext_t;
#define MAVLINK_MSG_STATUSTEXT_FIELD_TEXT_LEN 50

/**
 * @brief Pack a statustext message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param severity Severity of status, 0 = info message, 255 = critical fault
 * @param text Status text message, without null termination character
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_statustext_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t severity, const char* text)
{
	mavlink_statustext_t *p = (mavlink_statustext_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_STATUSTEXT;

	p->severity = severity; // uint8_t:Severity of status, 0 = info message, 255 = critical fault
	memcpy(p->text, text, sizeof(p->text)); // char[50]:Status text message, without null termination character

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_STATUSTEXT_LEN);
}

/**
 * @brief Pack a statustext message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param severity Severity of status, 0 = info message, 255 = critical fault
 * @param text Status text message, without null termination character
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_statustext_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t severity, const char* text)
{
	mavlink_statustext_t *p = (mavlink_statustext_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_STATUSTEXT;

	p->severity = severity; // uint8_t:Severity of status, 0 = info message, 255 = critical fault
	memcpy(p->text, text, sizeof(p->text)); // char[50]:Status text message, without null termination character

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_STATUSTEXT_LEN);
}

/**
 * @brief Encode a statustext struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param statustext C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_statustext_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_statustext_t* statustext)
{
	return mavlink_msg_statustext_pack(system_id, component_id, msg, statustext->severity, statustext->text);
}

/**
 * @brief Send a statustext message
 * @param chan MAVLink channel to send the message
 *
 * @param severity Severity of status, 0 = info message, 255 = critical fault
 * @param text Status text message, without null termination character
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_statustext_send(mavlink_channel_t chan, uint8_t severity, const char* text)
{
	mavlink_message_t msg;
	uint16_t checksum;
	mavlink_statustext_t *p = (mavlink_statustext_t *)&msg.payload[0];

	p->severity = severity; // uint8_t:Severity of status, 0 = info message, 255 = critical fault
	memcpy(p->text, text, sizeof(p->text)); // char[50]:Status text message, without null termination character

	msg.STX = MAVLINK_STX;
	msg.len = MAVLINK_MSG_ID_STATUSTEXT_LEN;
	msg.msgid = MAVLINK_MSG_ID_STATUSTEXT;
	msg.sysid = mavlink_system.sysid;
	msg.compid = mavlink_system.compid;
	msg.seq = mavlink_get_channel_status(chan)->current_tx_seq;
	mavlink_get_channel_status(chan)->current_tx_seq = msg.seq + 1;
	checksum = crc_calculate_msg(&msg, msg.len + MAVLINK_CORE_HEADER_LEN);
	msg.ck_a = (uint8_t)(checksum & 0xFF); ///< Low byte
	msg.ck_b = (uint8_t)(checksum >> 8); ///< High byte

	mavlink_send_msg(chan, &msg);
}

#endif

#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS_SMALL
static inline void mavlink_msg_statustext_send(mavlink_channel_t chan, uint8_t severity, const char* text)
{
	mavlink_header_t hdr;
	mavlink_statustext_t payload;
	uint16_t checksum;
	mavlink_statustext_t *p = &payload;

	p->severity = severity; // uint8_t:Severity of status, 0 = info message, 255 = critical fault
	memcpy(p->text, text, sizeof(p->text)); // char[50]:Status text message, without null termination character

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_STATUSTEXT_LEN;
	hdr.msgid = MAVLINK_MSG_ID_STATUSTEXT;
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
// MESSAGE STATUSTEXT UNPACKING

/**
 * @brief Get field severity from statustext message
 *
 * @return Severity of status, 0 = info message, 255 = critical fault
 */
static inline uint8_t mavlink_msg_statustext_get_severity(const mavlink_message_t* msg)
{
	mavlink_statustext_t *p = (mavlink_statustext_t *)&msg->payload[0];
	return (uint8_t)(p->severity);
}

/**
 * @brief Get field text from statustext message
 *
 * @return Status text message, without null termination character
 */
static inline uint16_t mavlink_msg_statustext_get_text(const mavlink_message_t* msg, char* text)
{
	mavlink_statustext_t *p = (mavlink_statustext_t *)&msg->payload[0];

	memcpy(text, p->text, sizeof(p->text));
	return sizeof(p->text);
}

/**
 * @brief Decode a statustext message into a struct
 *
 * @param msg The message to decode
 * @param statustext C-struct to decode the message contents into
 */
static inline void mavlink_msg_statustext_decode(const mavlink_message_t* msg, mavlink_statustext_t* statustext)
{
	memcpy( statustext, msg->payload, sizeof(mavlink_statustext_t));
}
