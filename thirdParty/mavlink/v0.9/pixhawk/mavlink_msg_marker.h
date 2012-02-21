// MESSAGE MARKER PACKING

#define MAVLINK_MSG_ID_MARKER 171

typedef struct __mavlink_marker_t
{
 uint16_t id; ///< ID
 float x; ///< x position
 float y; ///< y position
 float z; ///< z position
 float roll; ///< roll orientation
 float pitch; ///< pitch orientation
 float yaw; ///< yaw orientation
} mavlink_marker_t;

#define MAVLINK_MSG_ID_MARKER_LEN 26
#define MAVLINK_MSG_ID_171_LEN 26



#define MAVLINK_MESSAGE_INFO_MARKER { \
	"MARKER", \
	7, \
	{  { "id", NULL, MAVLINK_TYPE_UINT16_T, 0, 0, offsetof(mavlink_marker_t, id) }, \
         { "x", NULL, MAVLINK_TYPE_FLOAT, 0, 2, offsetof(mavlink_marker_t, x) }, \
         { "y", NULL, MAVLINK_TYPE_FLOAT, 0, 6, offsetof(mavlink_marker_t, y) }, \
         { "z", NULL, MAVLINK_TYPE_FLOAT, 0, 10, offsetof(mavlink_marker_t, z) }, \
         { "roll", NULL, MAVLINK_TYPE_FLOAT, 0, 14, offsetof(mavlink_marker_t, roll) }, \
         { "pitch", NULL, MAVLINK_TYPE_FLOAT, 0, 18, offsetof(mavlink_marker_t, pitch) }, \
         { "yaw", NULL, MAVLINK_TYPE_FLOAT, 0, 22, offsetof(mavlink_marker_t, yaw) }, \
         } \
}


/**
 * @brief Pack a marker message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param id ID
 * @param x x position
 * @param y y position
 * @param z z position
 * @param roll roll orientation
 * @param pitch pitch orientation
 * @param yaw yaw orientation
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_marker_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint16_t id, float x, float y, float z, float roll, float pitch, float yaw)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[26];
	_mav_put_uint16_t(buf, 0, id);
	_mav_put_float(buf, 2, x);
	_mav_put_float(buf, 6, y);
	_mav_put_float(buf, 10, z);
	_mav_put_float(buf, 14, roll);
	_mav_put_float(buf, 18, pitch);
	_mav_put_float(buf, 22, yaw);

        memcpy(_MAV_PAYLOAD(msg), buf, 26);
#else
	mavlink_marker_t packet;
	packet.id = id;
	packet.x = x;
	packet.y = y;
	packet.z = z;
	packet.roll = roll;
	packet.pitch = pitch;
	packet.yaw = yaw;

        memcpy(_MAV_PAYLOAD(msg), &packet, 26);
#endif

	msg->msgid = MAVLINK_MSG_ID_MARKER;
	return mavlink_finalize_message(msg, system_id, component_id, 26);
}

/**
 * @brief Pack a marker message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param id ID
 * @param x x position
 * @param y y position
 * @param z z position
 * @param roll roll orientation
 * @param pitch pitch orientation
 * @param yaw yaw orientation
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_marker_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint16_t id,float x,float y,float z,float roll,float pitch,float yaw)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[26];
	_mav_put_uint16_t(buf, 0, id);
	_mav_put_float(buf, 2, x);
	_mav_put_float(buf, 6, y);
	_mav_put_float(buf, 10, z);
	_mav_put_float(buf, 14, roll);
	_mav_put_float(buf, 18, pitch);
	_mav_put_float(buf, 22, yaw);

        memcpy(_MAV_PAYLOAD(msg), buf, 26);
#else
	mavlink_marker_t packet;
	packet.id = id;
	packet.x = x;
	packet.y = y;
	packet.z = z;
	packet.roll = roll;
	packet.pitch = pitch;
	packet.yaw = yaw;

        memcpy(_MAV_PAYLOAD(msg), &packet, 26);
#endif

	msg->msgid = MAVLINK_MSG_ID_MARKER;
	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, 26);
}

/**
 * @brief Encode a marker struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param marker C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_marker_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_marker_t* marker)
{
	return mavlink_msg_marker_pack(system_id, component_id, msg, marker->id, marker->x, marker->y, marker->z, marker->roll, marker->pitch, marker->yaw);
}

/**
 * @brief Send a marker message
 * @param chan MAVLink channel to send the message
 *
 * @param id ID
 * @param x x position
 * @param y y position
 * @param z z position
 * @param roll roll orientation
 * @param pitch pitch orientation
 * @param yaw yaw orientation
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_marker_send(mavlink_channel_t chan, uint16_t id, float x, float y, float z, float roll, float pitch, float yaw)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[26];
	_mav_put_uint16_t(buf, 0, id);
	_mav_put_float(buf, 2, x);
	_mav_put_float(buf, 6, y);
	_mav_put_float(buf, 10, z);
	_mav_put_float(buf, 14, roll);
	_mav_put_float(buf, 18, pitch);
	_mav_put_float(buf, 22, yaw);

	_mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MARKER, buf, 26);
#else
	mavlink_marker_t packet;
	packet.id = id;
	packet.x = x;
	packet.y = y;
	packet.z = z;
	packet.roll = roll;
	packet.pitch = pitch;
	packet.yaw = yaw;

	_mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_MARKER, (const char *)&packet, 26);
#endif
}

#endif

// MESSAGE MARKER UNPACKING


/**
 * @brief Get field id from marker message
 *
 * @return ID
 */
static inline uint16_t mavlink_msg_marker_get_id(const mavlink_message_t* msg)
{
	return _MAV_RETURN_uint16_t(msg,  0);
}

/**
 * @brief Get field x from marker message
 *
 * @return x position
 */
static inline float mavlink_msg_marker_get_x(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  2);
}

/**
 * @brief Get field y from marker message
 *
 * @return y position
 */
static inline float mavlink_msg_marker_get_y(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  6);
}

/**
 * @brief Get field z from marker message
 *
 * @return z position
 */
static inline float mavlink_msg_marker_get_z(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  10);
}

/**
 * @brief Get field roll from marker message
 *
 * @return roll orientation
 */
static inline float mavlink_msg_marker_get_roll(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  14);
}

/**
 * @brief Get field pitch from marker message
 *
 * @return pitch orientation
 */
static inline float mavlink_msg_marker_get_pitch(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  18);
}

/**
 * @brief Get field yaw from marker message
 *
 * @return yaw orientation
 */
static inline float mavlink_msg_marker_get_yaw(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  22);
}

/**
 * @brief Decode a marker message into a struct
 *
 * @param msg The message to decode
 * @param marker C-struct to decode the message contents into
 */
static inline void mavlink_msg_marker_decode(const mavlink_message_t* msg, mavlink_marker_t* marker)
{
#if MAVLINK_NEED_BYTE_SWAP
	marker->id = mavlink_msg_marker_get_id(msg);
	marker->x = mavlink_msg_marker_get_x(msg);
	marker->y = mavlink_msg_marker_get_y(msg);
	marker->z = mavlink_msg_marker_get_z(msg);
	marker->roll = mavlink_msg_marker_get_roll(msg);
	marker->pitch = mavlink_msg_marker_get_pitch(msg);
	marker->yaw = mavlink_msg_marker_get_yaw(msg);
#else
	memcpy(marker, _MAV_PAYLOAD(msg), 26);
#endif
}
