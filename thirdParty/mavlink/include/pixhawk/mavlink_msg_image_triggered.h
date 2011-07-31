// MESSAGE IMAGE_TRIGGERED PACKING

#define MAVLINK_MSG_ID_IMAGE_TRIGGERED 101
#define MAVLINK_MSG_ID_IMAGE_TRIGGERED_LEN 52
#define MAVLINK_MSG_101_LEN 52

typedef struct __mavlink_image_triggered_t 
{
	uint64_t timestamp; ///< Timestamp
	uint32_t seq; ///< IMU seq
	float roll; ///< Roll angle in rad
	float pitch; ///< Pitch angle in rad
	float yaw; ///< Yaw angle in rad
	float local_z; ///< Local frame Z coordinate (height over ground)
	float lat; ///< GPS X coordinate
	float lon; ///< GPS Y coordinate
	float alt; ///< Global frame altitude
	float ground_x; ///< Ground truth X
	float ground_y; ///< Ground truth Y
	float ground_z; ///< Ground truth Z

} mavlink_image_triggered_t;

/**
 * @brief Pack a image_triggered message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param timestamp Timestamp
 * @param seq IMU seq
 * @param roll Roll angle in rad
 * @param pitch Pitch angle in rad
 * @param yaw Yaw angle in rad
 * @param local_z Local frame Z coordinate (height over ground)
 * @param lat GPS X coordinate
 * @param lon GPS Y coordinate
 * @param alt Global frame altitude
 * @param ground_x Ground truth X
 * @param ground_y Ground truth Y
 * @param ground_z Ground truth Z
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_image_triggered_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint64_t timestamp, uint32_t seq, float roll, float pitch, float yaw, float local_z, float lat, float lon, float alt, float ground_x, float ground_y, float ground_z)
{
	mavlink_image_triggered_t *p = (mavlink_image_triggered_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_IMAGE_TRIGGERED;

	p->timestamp = timestamp; // uint64_t:Timestamp
	p->seq = seq; // uint32_t:IMU seq
	p->roll = roll; // float:Roll angle in rad
	p->pitch = pitch; // float:Pitch angle in rad
	p->yaw = yaw; // float:Yaw angle in rad
	p->local_z = local_z; // float:Local frame Z coordinate (height over ground)
	p->lat = lat; // float:GPS X coordinate
	p->lon = lon; // float:GPS Y coordinate
	p->alt = alt; // float:Global frame altitude
	p->ground_x = ground_x; // float:Ground truth X
	p->ground_y = ground_y; // float:Ground truth Y
	p->ground_z = ground_z; // float:Ground truth Z

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_IMAGE_TRIGGERED_LEN);
}

/**
 * @brief Pack a image_triggered message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param timestamp Timestamp
 * @param seq IMU seq
 * @param roll Roll angle in rad
 * @param pitch Pitch angle in rad
 * @param yaw Yaw angle in rad
 * @param local_z Local frame Z coordinate (height over ground)
 * @param lat GPS X coordinate
 * @param lon GPS Y coordinate
 * @param alt Global frame altitude
 * @param ground_x Ground truth X
 * @param ground_y Ground truth Y
 * @param ground_z Ground truth Z
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_image_triggered_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint64_t timestamp, uint32_t seq, float roll, float pitch, float yaw, float local_z, float lat, float lon, float alt, float ground_x, float ground_y, float ground_z)
{
	mavlink_image_triggered_t *p = (mavlink_image_triggered_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_IMAGE_TRIGGERED;

	p->timestamp = timestamp; // uint64_t:Timestamp
	p->seq = seq; // uint32_t:IMU seq
	p->roll = roll; // float:Roll angle in rad
	p->pitch = pitch; // float:Pitch angle in rad
	p->yaw = yaw; // float:Yaw angle in rad
	p->local_z = local_z; // float:Local frame Z coordinate (height over ground)
	p->lat = lat; // float:GPS X coordinate
	p->lon = lon; // float:GPS Y coordinate
	p->alt = alt; // float:Global frame altitude
	p->ground_x = ground_x; // float:Ground truth X
	p->ground_y = ground_y; // float:Ground truth Y
	p->ground_z = ground_z; // float:Ground truth Z

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_IMAGE_TRIGGERED_LEN);
}

/**
 * @brief Encode a image_triggered struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param image_triggered C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_image_triggered_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_image_triggered_t* image_triggered)
{
	return mavlink_msg_image_triggered_pack(system_id, component_id, msg, image_triggered->timestamp, image_triggered->seq, image_triggered->roll, image_triggered->pitch, image_triggered->yaw, image_triggered->local_z, image_triggered->lat, image_triggered->lon, image_triggered->alt, image_triggered->ground_x, image_triggered->ground_y, image_triggered->ground_z);
}

/**
 * @brief Send a image_triggered message
 * @param chan MAVLink channel to send the message
 *
 * @param timestamp Timestamp
 * @param seq IMU seq
 * @param roll Roll angle in rad
 * @param pitch Pitch angle in rad
 * @param yaw Yaw angle in rad
 * @param local_z Local frame Z coordinate (height over ground)
 * @param lat GPS X coordinate
 * @param lon GPS Y coordinate
 * @param alt Global frame altitude
 * @param ground_x Ground truth X
 * @param ground_y Ground truth Y
 * @param ground_z Ground truth Z
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS
static inline void mavlink_msg_image_triggered_send(mavlink_channel_t chan, uint64_t timestamp, uint32_t seq, float roll, float pitch, float yaw, float local_z, float lat, float lon, float alt, float ground_x, float ground_y, float ground_z)
{
	mavlink_message_t msg;
	uint16_t checksum;
	mavlink_image_triggered_t *p = (mavlink_image_triggered_t *)&msg.payload[0];

	p->timestamp = timestamp; // uint64_t:Timestamp
	p->seq = seq; // uint32_t:IMU seq
	p->roll = roll; // float:Roll angle in rad
	p->pitch = pitch; // float:Pitch angle in rad
	p->yaw = yaw; // float:Yaw angle in rad
	p->local_z = local_z; // float:Local frame Z coordinate (height over ground)
	p->lat = lat; // float:GPS X coordinate
	p->lon = lon; // float:GPS Y coordinate
	p->alt = alt; // float:Global frame altitude
	p->ground_x = ground_x; // float:Ground truth X
	p->ground_y = ground_y; // float:Ground truth Y
	p->ground_z = ground_z; // float:Ground truth Z

	msg.STX = MAVLINK_STX;
	msg.len = MAVLINK_MSG_ID_IMAGE_TRIGGERED_LEN;
	msg.msgid = MAVLINK_MSG_ID_IMAGE_TRIGGERED;
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
static inline void mavlink_msg_image_triggered_send(mavlink_channel_t chan, uint64_t timestamp, uint32_t seq, float roll, float pitch, float yaw, float local_z, float lat, float lon, float alt, float ground_x, float ground_y, float ground_z)
{
	mavlink_header_t hdr;
	mavlink_image_triggered_t payload;
	uint16_t checksum;
	mavlink_image_triggered_t *p = &payload;

	p->timestamp = timestamp; // uint64_t:Timestamp
	p->seq = seq; // uint32_t:IMU seq
	p->roll = roll; // float:Roll angle in rad
	p->pitch = pitch; // float:Pitch angle in rad
	p->yaw = yaw; // float:Yaw angle in rad
	p->local_z = local_z; // float:Local frame Z coordinate (height over ground)
	p->lat = lat; // float:GPS X coordinate
	p->lon = lon; // float:GPS Y coordinate
	p->alt = alt; // float:Global frame altitude
	p->ground_x = ground_x; // float:Ground truth X
	p->ground_y = ground_y; // float:Ground truth Y
	p->ground_z = ground_z; // float:Ground truth Z

	hdr.STX = MAVLINK_STX;
	hdr.len = MAVLINK_MSG_ID_IMAGE_TRIGGERED_LEN;
	hdr.msgid = MAVLINK_MSG_ID_IMAGE_TRIGGERED;
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
// MESSAGE IMAGE_TRIGGERED UNPACKING

/**
 * @brief Get field timestamp from image_triggered message
 *
 * @return Timestamp
 */
static inline uint64_t mavlink_msg_image_triggered_get_timestamp(const mavlink_message_t* msg)
{
	mavlink_image_triggered_t *p = (mavlink_image_triggered_t *)&msg->payload[0];
	return (uint64_t)(p->timestamp);
}

/**
 * @brief Get field seq from image_triggered message
 *
 * @return IMU seq
 */
static inline uint32_t mavlink_msg_image_triggered_get_seq(const mavlink_message_t* msg)
{
	mavlink_image_triggered_t *p = (mavlink_image_triggered_t *)&msg->payload[0];
	return (uint32_t)(p->seq);
}

/**
 * @brief Get field roll from image_triggered message
 *
 * @return Roll angle in rad
 */
static inline float mavlink_msg_image_triggered_get_roll(const mavlink_message_t* msg)
{
	mavlink_image_triggered_t *p = (mavlink_image_triggered_t *)&msg->payload[0];
	return (float)(p->roll);
}

/**
 * @brief Get field pitch from image_triggered message
 *
 * @return Pitch angle in rad
 */
static inline float mavlink_msg_image_triggered_get_pitch(const mavlink_message_t* msg)
{
	mavlink_image_triggered_t *p = (mavlink_image_triggered_t *)&msg->payload[0];
	return (float)(p->pitch);
}

/**
 * @brief Get field yaw from image_triggered message
 *
 * @return Yaw angle in rad
 */
static inline float mavlink_msg_image_triggered_get_yaw(const mavlink_message_t* msg)
{
	mavlink_image_triggered_t *p = (mavlink_image_triggered_t *)&msg->payload[0];
	return (float)(p->yaw);
}

/**
 * @brief Get field local_z from image_triggered message
 *
 * @return Local frame Z coordinate (height over ground)
 */
static inline float mavlink_msg_image_triggered_get_local_z(const mavlink_message_t* msg)
{
	mavlink_image_triggered_t *p = (mavlink_image_triggered_t *)&msg->payload[0];
	return (float)(p->local_z);
}

/**
 * @brief Get field lat from image_triggered message
 *
 * @return GPS X coordinate
 */
static inline float mavlink_msg_image_triggered_get_lat(const mavlink_message_t* msg)
{
	mavlink_image_triggered_t *p = (mavlink_image_triggered_t *)&msg->payload[0];
	return (float)(p->lat);
}

/**
 * @brief Get field lon from image_triggered message
 *
 * @return GPS Y coordinate
 */
static inline float mavlink_msg_image_triggered_get_lon(const mavlink_message_t* msg)
{
	mavlink_image_triggered_t *p = (mavlink_image_triggered_t *)&msg->payload[0];
	return (float)(p->lon);
}

/**
 * @brief Get field alt from image_triggered message
 *
 * @return Global frame altitude
 */
static inline float mavlink_msg_image_triggered_get_alt(const mavlink_message_t* msg)
{
	mavlink_image_triggered_t *p = (mavlink_image_triggered_t *)&msg->payload[0];
	return (float)(p->alt);
}

/**
 * @brief Get field ground_x from image_triggered message
 *
 * @return Ground truth X
 */
static inline float mavlink_msg_image_triggered_get_ground_x(const mavlink_message_t* msg)
{
	mavlink_image_triggered_t *p = (mavlink_image_triggered_t *)&msg->payload[0];
	return (float)(p->ground_x);
}

/**
 * @brief Get field ground_y from image_triggered message
 *
 * @return Ground truth Y
 */
static inline float mavlink_msg_image_triggered_get_ground_y(const mavlink_message_t* msg)
{
	mavlink_image_triggered_t *p = (mavlink_image_triggered_t *)&msg->payload[0];
	return (float)(p->ground_y);
}

/**
 * @brief Get field ground_z from image_triggered message
 *
 * @return Ground truth Z
 */
static inline float mavlink_msg_image_triggered_get_ground_z(const mavlink_message_t* msg)
{
	mavlink_image_triggered_t *p = (mavlink_image_triggered_t *)&msg->payload[0];
	return (float)(p->ground_z);
}

/**
 * @brief Decode a image_triggered message into a struct
 *
 * @param msg The message to decode
 * @param image_triggered C-struct to decode the message contents into
 */
static inline void mavlink_msg_image_triggered_decode(const mavlink_message_t* msg, mavlink_image_triggered_t* image_triggered)
{
	memcpy( image_triggered, msg->payload, sizeof(mavlink_image_triggered_t));
}
