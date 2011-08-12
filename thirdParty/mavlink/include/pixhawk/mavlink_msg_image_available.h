// MESSAGE IMAGE_AVAILABLE PACKING

#define MAVLINK_MSG_ID_IMAGE_AVAILABLE 103
#define MAVLINK_MSG_ID_IMAGE_AVAILABLE_LEN 92
#define MAVLINK_MSG_103_LEN 92

typedef struct __mavlink_image_available_t 
{
	uint64_t cam_id; ///< Camera id
	uint64_t timestamp; ///< Timestamp
	uint64_t valid_until; ///< Until which timestamp this buffer will stay valid
	uint32_t img_seq; ///< The image sequence number
	uint32_t img_buf_index; ///< Position of the image in the buffer, starts with 0
	uint32_t key; ///< Shared memory area key
	uint32_t exposure; ///< Exposure time, in microseconds
	float gain; ///< Camera gain
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
	uint16_t width; ///< Image width
	uint16_t height; ///< Image height
	uint16_t depth; ///< Image depth
	uint8_t cam_no; ///< Camera # (starts with 0)
	uint8_t channels; ///< Image channels

} mavlink_image_available_t;

/**
 * @brief Pack a image_available message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param cam_id Camera id
 * @param cam_no Camera # (starts with 0)
 * @param timestamp Timestamp
 * @param valid_until Until which timestamp this buffer will stay valid
 * @param img_seq The image sequence number
 * @param img_buf_index Position of the image in the buffer, starts with 0
 * @param width Image width
 * @param height Image height
 * @param depth Image depth
 * @param channels Image channels
 * @param key Shared memory area key
 * @param exposure Exposure time, in microseconds
 * @param gain Camera gain
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
static inline uint16_t mavlink_msg_image_available_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint64_t cam_id, uint8_t cam_no, uint64_t timestamp, uint64_t valid_until, uint32_t img_seq, uint32_t img_buf_index, uint16_t width, uint16_t height, uint16_t depth, uint8_t channels, uint32_t key, uint32_t exposure, float gain, float roll, float pitch, float yaw, float local_z, float lat, float lon, float alt, float ground_x, float ground_y, float ground_z)
{
	mavlink_image_available_t *p = (mavlink_image_available_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_IMAGE_AVAILABLE;

	p->cam_id = cam_id; // uint64_t:Camera id
	p->cam_no = cam_no; // uint8_t:Camera # (starts with 0)
	p->timestamp = timestamp; // uint64_t:Timestamp
	p->valid_until = valid_until; // uint64_t:Until which timestamp this buffer will stay valid
	p->img_seq = img_seq; // uint32_t:The image sequence number
	p->img_buf_index = img_buf_index; // uint32_t:Position of the image in the buffer, starts with 0
	p->width = width; // uint16_t:Image width
	p->height = height; // uint16_t:Image height
	p->depth = depth; // uint16_t:Image depth
	p->channels = channels; // uint8_t:Image channels
	p->key = key; // uint32_t:Shared memory area key
	p->exposure = exposure; // uint32_t:Exposure time, in microseconds
	p->gain = gain; // float:Camera gain
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

	return mavlink_finalize_message(msg, system_id, component_id, MAVLINK_MSG_ID_IMAGE_AVAILABLE_LEN);
}

/**
 * @brief Pack a image_available message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param cam_id Camera id
 * @param cam_no Camera # (starts with 0)
 * @param timestamp Timestamp
 * @param valid_until Until which timestamp this buffer will stay valid
 * @param img_seq The image sequence number
 * @param img_buf_index Position of the image in the buffer, starts with 0
 * @param width Image width
 * @param height Image height
 * @param depth Image depth
 * @param channels Image channels
 * @param key Shared memory area key
 * @param exposure Exposure time, in microseconds
 * @param gain Camera gain
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
static inline uint16_t mavlink_msg_image_available_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint64_t cam_id, uint8_t cam_no, uint64_t timestamp, uint64_t valid_until, uint32_t img_seq, uint32_t img_buf_index, uint16_t width, uint16_t height, uint16_t depth, uint8_t channels, uint32_t key, uint32_t exposure, float gain, float roll, float pitch, float yaw, float local_z, float lat, float lon, float alt, float ground_x, float ground_y, float ground_z)
{
	mavlink_image_available_t *p = (mavlink_image_available_t *)&msg->payload[0];
	msg->msgid = MAVLINK_MSG_ID_IMAGE_AVAILABLE;

	p->cam_id = cam_id; // uint64_t:Camera id
	p->cam_no = cam_no; // uint8_t:Camera # (starts with 0)
	p->timestamp = timestamp; // uint64_t:Timestamp
	p->valid_until = valid_until; // uint64_t:Until which timestamp this buffer will stay valid
	p->img_seq = img_seq; // uint32_t:The image sequence number
	p->img_buf_index = img_buf_index; // uint32_t:Position of the image in the buffer, starts with 0
	p->width = width; // uint16_t:Image width
	p->height = height; // uint16_t:Image height
	p->depth = depth; // uint16_t:Image depth
	p->channels = channels; // uint8_t:Image channels
	p->key = key; // uint32_t:Shared memory area key
	p->exposure = exposure; // uint32_t:Exposure time, in microseconds
	p->gain = gain; // float:Camera gain
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

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, MAVLINK_MSG_ID_IMAGE_AVAILABLE_LEN);
}

/**
 * @brief Encode a image_available struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param image_available C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_image_available_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_image_available_t* image_available)
{
	return mavlink_msg_image_available_pack(system_id, component_id, msg, image_available->cam_id, image_available->cam_no, image_available->timestamp, image_available->valid_until, image_available->img_seq, image_available->img_buf_index, image_available->width, image_available->height, image_available->depth, image_available->channels, image_available->key, image_available->exposure, image_available->gain, image_available->roll, image_available->pitch, image_available->yaw, image_available->local_z, image_available->lat, image_available->lon, image_available->alt, image_available->ground_x, image_available->ground_y, image_available->ground_z);
}

/**
 * @brief Send a image_available message
 * @param chan MAVLink channel to send the message
 *
 * @param cam_id Camera id
 * @param cam_no Camera # (starts with 0)
 * @param timestamp Timestamp
 * @param valid_until Until which timestamp this buffer will stay valid
 * @param img_seq The image sequence number
 * @param img_buf_index Position of the image in the buffer, starts with 0
 * @param width Image width
 * @param height Image height
 * @param depth Image depth
 * @param channels Image channels
 * @param key Shared memory area key
 * @param exposure Exposure time, in microseconds
 * @param gain Camera gain
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
static inline void mavlink_msg_image_available_send(mavlink_channel_t chan, uint64_t cam_id, uint8_t cam_no, uint64_t timestamp, uint64_t valid_until, uint32_t img_seq, uint32_t img_buf_index, uint16_t width, uint16_t height, uint16_t depth, uint8_t channels, uint32_t key, uint32_t exposure, float gain, float roll, float pitch, float yaw, float local_z, float lat, float lon, float alt, float ground_x, float ground_y, float ground_z)
{
	mavlink_header_t hdr;
	mavlink_image_available_t payload;
	uint16_t checksum;
	mavlink_image_available_t *p = &payload;

	p->cam_id = cam_id; // uint64_t:Camera id
	p->cam_no = cam_no; // uint8_t:Camera # (starts with 0)
	p->timestamp = timestamp; // uint64_t:Timestamp
	p->valid_until = valid_until; // uint64_t:Until which timestamp this buffer will stay valid
	p->img_seq = img_seq; // uint32_t:The image sequence number
	p->img_buf_index = img_buf_index; // uint32_t:Position of the image in the buffer, starts with 0
	p->width = width; // uint16_t:Image width
	p->height = height; // uint16_t:Image height
	p->depth = depth; // uint16_t:Image depth
	p->channels = channels; // uint8_t:Image channels
	p->key = key; // uint32_t:Shared memory area key
	p->exposure = exposure; // uint32_t:Exposure time, in microseconds
	p->gain = gain; // float:Camera gain
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
	hdr.len = MAVLINK_MSG_ID_IMAGE_AVAILABLE_LEN;
	hdr.msgid = MAVLINK_MSG_ID_IMAGE_AVAILABLE;
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
// MESSAGE IMAGE_AVAILABLE UNPACKING

/**
 * @brief Get field cam_id from image_available message
 *
 * @return Camera id
 */
static inline uint64_t mavlink_msg_image_available_get_cam_id(const mavlink_message_t* msg)
{
	mavlink_image_available_t *p = (mavlink_image_available_t *)&msg->payload[0];
	return (uint64_t)(p->cam_id);
}

/**
 * @brief Get field cam_no from image_available message
 *
 * @return Camera # (starts with 0)
 */
static inline uint8_t mavlink_msg_image_available_get_cam_no(const mavlink_message_t* msg)
{
	mavlink_image_available_t *p = (mavlink_image_available_t *)&msg->payload[0];
	return (uint8_t)(p->cam_no);
}

/**
 * @brief Get field timestamp from image_available message
 *
 * @return Timestamp
 */
static inline uint64_t mavlink_msg_image_available_get_timestamp(const mavlink_message_t* msg)
{
	mavlink_image_available_t *p = (mavlink_image_available_t *)&msg->payload[0];
	return (uint64_t)(p->timestamp);
}

/**
 * @brief Get field valid_until from image_available message
 *
 * @return Until which timestamp this buffer will stay valid
 */
static inline uint64_t mavlink_msg_image_available_get_valid_until(const mavlink_message_t* msg)
{
	mavlink_image_available_t *p = (mavlink_image_available_t *)&msg->payload[0];
	return (uint64_t)(p->valid_until);
}

/**
 * @brief Get field img_seq from image_available message
 *
 * @return The image sequence number
 */
static inline uint32_t mavlink_msg_image_available_get_img_seq(const mavlink_message_t* msg)
{
	mavlink_image_available_t *p = (mavlink_image_available_t *)&msg->payload[0];
	return (uint32_t)(p->img_seq);
}

/**
 * @brief Get field img_buf_index from image_available message
 *
 * @return Position of the image in the buffer, starts with 0
 */
static inline uint32_t mavlink_msg_image_available_get_img_buf_index(const mavlink_message_t* msg)
{
	mavlink_image_available_t *p = (mavlink_image_available_t *)&msg->payload[0];
	return (uint32_t)(p->img_buf_index);
}

/**
 * @brief Get field width from image_available message
 *
 * @return Image width
 */
static inline uint16_t mavlink_msg_image_available_get_width(const mavlink_message_t* msg)
{
	mavlink_image_available_t *p = (mavlink_image_available_t *)&msg->payload[0];
	return (uint16_t)(p->width);
}

/**
 * @brief Get field height from image_available message
 *
 * @return Image height
 */
static inline uint16_t mavlink_msg_image_available_get_height(const mavlink_message_t* msg)
{
	mavlink_image_available_t *p = (mavlink_image_available_t *)&msg->payload[0];
	return (uint16_t)(p->height);
}

/**
 * @brief Get field depth from image_available message
 *
 * @return Image depth
 */
static inline uint16_t mavlink_msg_image_available_get_depth(const mavlink_message_t* msg)
{
	mavlink_image_available_t *p = (mavlink_image_available_t *)&msg->payload[0];
	return (uint16_t)(p->depth);
}

/**
 * @brief Get field channels from image_available message
 *
 * @return Image channels
 */
static inline uint8_t mavlink_msg_image_available_get_channels(const mavlink_message_t* msg)
{
	mavlink_image_available_t *p = (mavlink_image_available_t *)&msg->payload[0];
	return (uint8_t)(p->channels);
}

/**
 * @brief Get field key from image_available message
 *
 * @return Shared memory area key
 */
static inline uint32_t mavlink_msg_image_available_get_key(const mavlink_message_t* msg)
{
	mavlink_image_available_t *p = (mavlink_image_available_t *)&msg->payload[0];
	return (uint32_t)(p->key);
}

/**
 * @brief Get field exposure from image_available message
 *
 * @return Exposure time, in microseconds
 */
static inline uint32_t mavlink_msg_image_available_get_exposure(const mavlink_message_t* msg)
{
	mavlink_image_available_t *p = (mavlink_image_available_t *)&msg->payload[0];
	return (uint32_t)(p->exposure);
}

/**
 * @brief Get field gain from image_available message
 *
 * @return Camera gain
 */
static inline float mavlink_msg_image_available_get_gain(const mavlink_message_t* msg)
{
	mavlink_image_available_t *p = (mavlink_image_available_t *)&msg->payload[0];
	return (float)(p->gain);
}

/**
 * @brief Get field roll from image_available message
 *
 * @return Roll angle in rad
 */
static inline float mavlink_msg_image_available_get_roll(const mavlink_message_t* msg)
{
	mavlink_image_available_t *p = (mavlink_image_available_t *)&msg->payload[0];
	return (float)(p->roll);
}

/**
 * @brief Get field pitch from image_available message
 *
 * @return Pitch angle in rad
 */
static inline float mavlink_msg_image_available_get_pitch(const mavlink_message_t* msg)
{
	mavlink_image_available_t *p = (mavlink_image_available_t *)&msg->payload[0];
	return (float)(p->pitch);
}

/**
 * @brief Get field yaw from image_available message
 *
 * @return Yaw angle in rad
 */
static inline float mavlink_msg_image_available_get_yaw(const mavlink_message_t* msg)
{
	mavlink_image_available_t *p = (mavlink_image_available_t *)&msg->payload[0];
	return (float)(p->yaw);
}

/**
 * @brief Get field local_z from image_available message
 *
 * @return Local frame Z coordinate (height over ground)
 */
static inline float mavlink_msg_image_available_get_local_z(const mavlink_message_t* msg)
{
	mavlink_image_available_t *p = (mavlink_image_available_t *)&msg->payload[0];
	return (float)(p->local_z);
}

/**
 * @brief Get field lat from image_available message
 *
 * @return GPS X coordinate
 */
static inline float mavlink_msg_image_available_get_lat(const mavlink_message_t* msg)
{
	mavlink_image_available_t *p = (mavlink_image_available_t *)&msg->payload[0];
	return (float)(p->lat);
}

/**
 * @brief Get field lon from image_available message
 *
 * @return GPS Y coordinate
 */
static inline float mavlink_msg_image_available_get_lon(const mavlink_message_t* msg)
{
	mavlink_image_available_t *p = (mavlink_image_available_t *)&msg->payload[0];
	return (float)(p->lon);
}

/**
 * @brief Get field alt from image_available message
 *
 * @return Global frame altitude
 */
static inline float mavlink_msg_image_available_get_alt(const mavlink_message_t* msg)
{
	mavlink_image_available_t *p = (mavlink_image_available_t *)&msg->payload[0];
	return (float)(p->alt);
}

/**
 * @brief Get field ground_x from image_available message
 *
 * @return Ground truth X
 */
static inline float mavlink_msg_image_available_get_ground_x(const mavlink_message_t* msg)
{
	mavlink_image_available_t *p = (mavlink_image_available_t *)&msg->payload[0];
	return (float)(p->ground_x);
}

/**
 * @brief Get field ground_y from image_available message
 *
 * @return Ground truth Y
 */
static inline float mavlink_msg_image_available_get_ground_y(const mavlink_message_t* msg)
{
	mavlink_image_available_t *p = (mavlink_image_available_t *)&msg->payload[0];
	return (float)(p->ground_y);
}

/**
 * @brief Get field ground_z from image_available message
 *
 * @return Ground truth Z
 */
static inline float mavlink_msg_image_available_get_ground_z(const mavlink_message_t* msg)
{
	mavlink_image_available_t *p = (mavlink_image_available_t *)&msg->payload[0];
	return (float)(p->ground_z);
}

/**
 * @brief Decode a image_available message into a struct
 *
 * @param msg The message to decode
 * @param image_available C-struct to decode the message contents into
 */
static inline void mavlink_msg_image_available_decode(const mavlink_message_t* msg, mavlink_image_available_t* image_available)
{
	memcpy( image_available, msg->payload, sizeof(mavlink_image_available_t));
}
