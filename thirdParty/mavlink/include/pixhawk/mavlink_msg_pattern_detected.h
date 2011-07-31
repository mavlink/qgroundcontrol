// MESSAGE PATTERN_DETECTED PACKING

#define MAVLINK_MSG_ID_PATTERN_DETECTED 160
#define MAVLINK_MSG_ID_PATTERN_DETECTED_LEN 106
#define MAVLINK_MSG_160_LEN 106

typedef struct __mavlink_pattern_detected_t 
{
	uint8_t type; ///< 0: Pattern, 1: Letter
	float confidence; ///< Confidence of detection
	char file[100]; ///< Pattern file name
	uint8_t detected; ///< Accepted as true detection, 0 no, 1 yes

} mavlink_pattern_detected_t;
#define MAVLINK_MSG_PATTERN_DETECTED_FIELD_FILE_LEN 100

/**
 * @brief Pack a pattern_detected message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param type 0: Pattern, 1: Letter
 * @param confidence Confidence of detection
 * @param file Pattern file name
 * @param detected Accepted as true detection, 0 no, 1 yes
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_pattern_detected_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t type, float confidence, const char* file, uint8_t detected)
{
	mavlink_pattern_detected_t *p = (mavlink_pattern_detected_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_PATTERN_DETECTED;

	p->type = type; // uint8_t:0: Pattern, 1: Letter
	p->confidence = confidence; // float:Confidence of detection
	memcpy(p->file, file, sizeof(p->file)); // char[100]:Pattern file name
	p->detected = detected; // uint8_t:Accepted as true detection, 0 no, 1 yes

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_PATTERN_DETECTED_LEN);
}

/**
 * @brief Pack a pattern_detected message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param type 0: Pattern, 1: Letter
 * @param confidence Confidence of detection
 * @param file Pattern file name
 * @param detected Accepted as true detection, 0 no, 1 yes
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_pattern_detected_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t type, float confidence, const char* file, uint8_t detected)
{
	mavlink_pattern_detected_t *p = (mavlink_pattern_detected_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_PATTERN_DETECTED;

	p->type = type; // uint8_t:0: Pattern, 1: Letter
	p->confidence = confidence; // float:Confidence of detection
	memcpy(p->file, file, sizeof(p->file)); // char[100]:Pattern file name
	p->detected = detected; // uint8_t:Accepted as true detection, 0 no, 1 yes

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_PATTERN_DETECTED_LEN);
}

/**
 * @brief Encode a pattern_detected struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param pattern_detected C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_pattern_detected_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_pattern_detected_t* pattern_detected)
{
	return mavlink_msg_pattern_detected_pack(system_id, component_id, msg, pattern_detected->type, pattern_detected->confidence, pattern_detected->file, pattern_detected->detected);
}

/**
 * @brief Send a pattern_detected message
 * @param chan MAVLink channel to send the message
 *
 * @param type 0: Pattern, 1: Letter
 * @param confidence Confidence of detection
 * @param file Pattern file name
 * @param detected Accepted as true detection, 0 no, 1 yes
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_pattern_detected_send(mavlink_channel_t chan, uint8_t type, float confidence, const char* file, uint8_t detected)
{
	mavlink_message_t msg;
	uint16_t checksum;
	mavlink_pattern_detected_t *p = (mavlink_pattern_detected_t *)&msg.payload[0];

	p->type = type; // uint8_t:0: Pattern, 1: Letter
	p->confidence = confidence; // float:Confidence of detection
	memcpy(p->file, file, sizeof(p->file)); // char[100]:Pattern file name
	p->detected = detected; // uint8_t:Accepted as true detection, 0 no, 1 yes

	msg.STX = MAVLINK_STX;
	msg.len = MAVLINK_MSG_ID_PATTERN_DETECTED_LEN;
	msg.msgid = MAVLINK_MSG_ID_PATTERN_DETECTED;
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
static inline void mavlink_msg_pattern_detected_send(mavlink_channel_t chan, uint8_t type, float confidence, const char* file, uint8_t detected)
{
	mavlink_header_t hdr;
	mavlink_pattern_detected_t payload;
	uint16_t checksum;
	mavlink_pattern_detected_t *p = &payload;

	p->type = type; // uint8_t:0: Pattern, 1: Letter
	p->confidence = confidence; // float:Confidence of detection
	memcpy(p->file, file, sizeof(p->file)); // char[100]:Pattern file name
	p->detected = detected; // uint8_t:Accepted as true detection, 0 no, 1 yes

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_PATTERN_DETECTED_LEN;
	hdr.msgid = MAVLINK_MSG_ID_PATTERN_DETECTED;
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
// MESSAGE PATTERN_DETECTED UNPACKING

/**
 * @brief Get field type from pattern_detected message
 *
 * @return 0: Pattern, 1: Letter
 */
static inline uint8_t mavlink_msg_pattern_detected_get_type(const mavlink_message_t* msg)
{
	mavlink_pattern_detected_t *p = (mavlink_pattern_detected_t *)&msg->payload[0];
	return (uint8_t)(p->type);
}

/**
 * @brief Get field confidence from pattern_detected message
 *
 * @return Confidence of detection
 */
static inline float mavlink_msg_pattern_detected_get_confidence(const mavlink_message_t* msg)
{
	mavlink_pattern_detected_t *p = (mavlink_pattern_detected_t *)&msg->payload[0];
	return (float)(p->confidence);
}

/**
 * @brief Get field file from pattern_detected message
 *
 * @return Pattern file name
 */
static inline uint16_t mavlink_msg_pattern_detected_get_file(const mavlink_message_t* msg, char* file)
{
	mavlink_pattern_detected_t *p = (mavlink_pattern_detected_t *)&msg->payload[0];

	memcpy(file, p->file, sizeof(p->file));
	return sizeof(p->file);
}

/**
 * @brief Get field detected from pattern_detected message
 *
 * @return Accepted as true detection, 0 no, 1 yes
 */
static inline uint8_t mavlink_msg_pattern_detected_get_detected(const mavlink_message_t* msg)
{
	mavlink_pattern_detected_t *p = (mavlink_pattern_detected_t *)&msg->payload[0];
	return (uint8_t)(p->detected);
}

/**
 * @brief Decode a pattern_detected message into a struct
 *
 * @param msg The message to decode
 * @param pattern_detected C-struct to decode the message contents into
 */
static inline void mavlink_msg_pattern_detected_decode(const mavlink_message_t* msg, mavlink_pattern_detected_t* pattern_detected)
{
	memcpy( pattern_detected, msg->payload, sizeof(mavlink_pattern_detected_t));
}
