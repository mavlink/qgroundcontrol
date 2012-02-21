// MESSAGE VISION_SPEED_ESTIMATE PACKING

#define MAVLINK_MSG_ID_VISION_SPEED_ESTIMATE 158

typedef struct __mavlink_vision_speed_estimate_t
{
 uint64_t usec; ///< Timestamp (milliseconds)
 float x; ///< Global X speed
 float y; ///< Global Y speed
 float z; ///< Global Z speed
} mavlink_vision_speed_estimate_t;

#define MAVLINK_MSG_ID_VISION_SPEED_ESTIMATE_LEN 20
#define MAVLINK_MSG_ID_158_LEN 20



#define MAVLINK_MESSAGE_INFO_VISION_SPEED_ESTIMATE { \
	"VISION_SPEED_ESTIMATE", \
	4, \
	{  { "usec", NULL, MAVLINK_TYPE_UINT64_T, 0, 0, offsetof(mavlink_vision_speed_estimate_t, usec) }, \
         { "x", NULL, MAVLINK_TYPE_FLOAT, 0, 8, offsetof(mavlink_vision_speed_estimate_t, x) }, \
         { "y", NULL, MAVLINK_TYPE_FLOAT, 0, 12, offsetof(mavlink_vision_speed_estimate_t, y) }, \
         { "z", NULL, MAVLINK_TYPE_FLOAT, 0, 16, offsetof(mavlink_vision_speed_estimate_t, z) }, \
         } \
}


/**
 * @brief Pack a vision_speed_estimate message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param usec Timestamp (milliseconds)
 * @param x Global X speed
 * @param y Global Y speed
 * @param z Global Z speed
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_vision_speed_estimate_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint64_t usec, float x, float y, float z)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[20];
	_mav_put_uint64_t(buf, 0, usec);
	_mav_put_float(buf, 8, x);
	_mav_put_float(buf, 12, y);
	_mav_put_float(buf, 16, z);

        memcpy(_MAV_PAYLOAD(msg), buf, 20);
#else
	mavlink_vision_speed_estimate_t packet;
	packet.usec = usec;
	packet.x = x;
	packet.y = y;
	packet.z = z;

        memcpy(_MAV_PAYLOAD(msg), &packet, 20);
#endif

	msg->msgid = MAVLINK_MSG_ID_VISION_SPEED_ESTIMATE;
	return mavlink_finalize_message(msg, system_id, component_id, 20);
}

/**
 * @brief Pack a vision_speed_estimate message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param usec Timestamp (milliseconds)
 * @param x Global X speed
 * @param y Global Y speed
 * @param z Global Z speed
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_vision_speed_estimate_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint64_t usec,float x,float y,float z)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[20];
	_mav_put_uint64_t(buf, 0, usec);
	_mav_put_float(buf, 8, x);
	_mav_put_float(buf, 12, y);
	_mav_put_float(buf, 16, z);

        memcpy(_MAV_PAYLOAD(msg), buf, 20);
#else
	mavlink_vision_speed_estimate_t packet;
	packet.usec = usec;
	packet.x = x;
	packet.y = y;
	packet.z = z;

        memcpy(_MAV_PAYLOAD(msg), &packet, 20);
#endif

	msg->msgid = MAVLINK_MSG_ID_VISION_SPEED_ESTIMATE;
	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, 20);
}

/**
 * @brief Encode a vision_speed_estimate struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param vision_speed_estimate C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_vision_speed_estimate_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_vision_speed_estimate_t* vision_speed_estimate)
{
	return mavlink_msg_vision_speed_estimate_pack(system_id, component_id, msg, vision_speed_estimate->usec, vision_speed_estimate->x, vision_speed_estimate->y, vision_speed_estimate->z);
}

/**
 * @brief Send a vision_speed_estimate message
 * @param chan MAVLink channel to send the message
 *
 * @param usec Timestamp (milliseconds)
 * @param x Global X speed
 * @param y Global Y speed
 * @param z Global Z speed
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_vision_speed_estimate_send(mavlink_channel_t chan, uint64_t usec, float x, float y, float z)
{
#if MAVLINK_NEED_BYTE_SWAP || !MAVLINK_ALIGNED_FIELDS
	char buf[20];
	_mav_put_uint64_t(buf, 0, usec);
	_mav_put_float(buf, 8, x);
	_mav_put_float(buf, 12, y);
	_mav_put_float(buf, 16, z);

	_mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_VISION_SPEED_ESTIMATE, buf, 20);
#else
	mavlink_vision_speed_estimate_t packet;
	packet.usec = usec;
	packet.x = x;
	packet.y = y;
	packet.z = z;

	_mav_finalize_message_chan_send(chan, MAVLINK_MSG_ID_VISION_SPEED_ESTIMATE, (const char *)&packet, 20);
#endif
}

#endif

// MESSAGE VISION_SPEED_ESTIMATE UNPACKING


/**
 * @brief Get field usec from vision_speed_estimate message
 *
 * @return Timestamp (milliseconds)
 */
static inline uint64_t mavlink_msg_vision_speed_estimate_get_usec(const mavlink_message_t* msg)
{
	return _MAV_RETURN_uint64_t(msg,  0);
}

/**
 * @brief Get field x from vision_speed_estimate message
 *
 * @return Global X speed
 */
static inline float mavlink_msg_vision_speed_estimate_get_x(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  8);
}

/**
 * @brief Get field y from vision_speed_estimate message
 *
 * @return Global Y speed
 */
static inline float mavlink_msg_vision_speed_estimate_get_y(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  12);
}

/**
 * @brief Get field z from vision_speed_estimate message
 *
 * @return Global Z speed
 */
static inline float mavlink_msg_vision_speed_estimate_get_z(const mavlink_message_t* msg)
{
	return _MAV_RETURN_float(msg,  16);
}

/**
 * @brief Decode a vision_speed_estimate message into a struct
 *
 * @param msg The message to decode
 * @param vision_speed_estimate C-struct to decode the message contents into
 */
static inline void mavlink_msg_vision_speed_estimate_decode(const mavlink_message_t* msg, mavlink_vision_speed_estimate_t* vision_speed_estimate)
{
#if MAVLINK_NEED_BYTE_SWAP
	vision_speed_estimate->usec = mavlink_msg_vision_speed_estimate_get_usec(msg);
	vision_speed_estimate->x = mavlink_msg_vision_speed_estimate_get_x(msg);
	vision_speed_estimate->y = mavlink_msg_vision_speed_estimate_get_y(msg);
	vision_speed_estimate->z = mavlink_msg_vision_speed_estimate_get_z(msg);
#else
	memcpy(vision_speed_estimate, _MAV_PAYLOAD(msg), 20);
#endif
}
