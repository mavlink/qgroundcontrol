// MESSAGE OPTICAL_FLOW PACKING

#define MAVLINK_MSG_ID_OPTICAL_FLOW 100

typedef struct __mavlink_optical_flow_t
{
 uint64_t time; ///< Timestamp (UNIX)
 float ground_distance; ///< Ground distance in meters
 int16_t flow_x; ///< Flow in pixels in x-sensor direction
 int16_t flow_y; ///< Flow in pixels in y-sensor direction
 uint8_t sensor_id; ///< Sensor ID
 uint8_t quality; ///< Optical flow quality / confidence. 0: bad, 255: maximum quality
} mavlink_optical_flow_t;

#define MAVLINK_MSG_ID_OPTICAL_FLOW_LEN 18
#define MAVLINK_MSG_ID_100_LEN 18



#define MAVLINK_MESSAGE_INFO_OPTICAL_FLOW { \
	"OPTICAL_FLOW", \
	6, \
	{  { "time", MAVLINK_TYPE_UINT64_T, 0, 0, offsetof(mavlink_optical_flow_t, time) }, \
         { "ground_distance", MAVLINK_TYPE_FLOAT, 0, 8, offsetof(mavlink_optical_flow_t, ground_distance) }, \
         { "flow_x", MAVLINK_TYPE_INT16_T, 0, 12, offsetof(mavlink_optical_flow_t, flow_x) }, \
         { "flow_y", MAVLINK_TYPE_INT16_T, 0, 14, offsetof(mavlink_optical_flow_t, flow_y) }, \
         { "sensor_id", MAVLINK_TYPE_UINT8_T, 0, 16, offsetof(mavlink_optical_flow_t, sensor_id) }, \
         { "quality", MAVLINK_TYPE_UINT8_T, 0, 17, offsetof(mavlink_optical_flow_t, quality) }, \
         } \
}


/**
 * @brief Pack a optical_flow message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param time Timestamp (UNIX)
 * @param sensor_id Sensor ID
 * @param flow_x Flow in pixels in x-sensor direction
 * @param flow_y Flow in pixels in y-sensor direction
 * @param quality Optical flow quality / confidence. 0: bad, 255: maximum quality
 * @param ground_distance Ground distance in meters
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_optical_flow_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint64_t time, uint8_t sensor_id, int16_t flow_x, int16_t flow_y, uint8_t quality, float ground_distance)
{
	msg->msgid = MAVLINK_MSG_ID_OPTICAL_FLOW;

	put_uint64_t_by_index(msg, 0, time); // Timestamp (UNIX)
	put_float_by_index(msg, 8, ground_distance); // Ground distance in meters
	put_int16_t_by_index(msg, 12, flow_x); // Flow in pixels in x-sensor direction
	put_int16_t_by_index(msg, 14, flow_y); // Flow in pixels in y-sensor direction
	put_uint8_t_by_index(msg, 16, sensor_id); // Sensor ID
	put_uint8_t_by_index(msg, 17, quality); // Optical flow quality / confidence. 0: bad, 255: maximum quality

	return mavlink_finalize_message(msg, system_id, component_id, 18, 146);
}

/**
 * @brief Pack a optical_flow message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param time Timestamp (UNIX)
 * @param sensor_id Sensor ID
 * @param flow_x Flow in pixels in x-sensor direction
 * @param flow_y Flow in pixels in y-sensor direction
 * @param quality Optical flow quality / confidence. 0: bad, 255: maximum quality
 * @param ground_distance Ground distance in meters
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_optical_flow_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint64_t time,uint8_t sensor_id,int16_t flow_x,int16_t flow_y,uint8_t quality,float ground_distance)
{
	msg->msgid = MAVLINK_MSG_ID_OPTICAL_FLOW;

	put_uint64_t_by_index(msg, 0, time); // Timestamp (UNIX)
	put_float_by_index(msg, 8, ground_distance); // Ground distance in meters
	put_int16_t_by_index(msg, 12, flow_x); // Flow in pixels in x-sensor direction
	put_int16_t_by_index(msg, 14, flow_y); // Flow in pixels in y-sensor direction
	put_uint8_t_by_index(msg, 16, sensor_id); // Sensor ID
	put_uint8_t_by_index(msg, 17, quality); // Optical flow quality / confidence. 0: bad, 255: maximum quality

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, 18, 146);
}

#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

/**
 * @brief Pack a optical_flow message on a channel and send
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param time Timestamp (UNIX)
 * @param sensor_id Sensor ID
 * @param flow_x Flow in pixels in x-sensor direction
 * @param flow_y Flow in pixels in y-sensor direction
 * @param quality Optical flow quality / confidence. 0: bad, 255: maximum quality
 * @param ground_distance Ground distance in meters
 */
static inline void mavlink_msg_optical_flow_pack_chan_send(mavlink_channel_t chan,
							   mavlink_message_t* msg,
						           uint64_t time,uint8_t sensor_id,int16_t flow_x,int16_t flow_y,uint8_t quality,float ground_distance)
{
	msg->msgid = MAVLINK_MSG_ID_OPTICAL_FLOW;

	put_uint64_t_by_index(msg, 0, time); // Timestamp (UNIX)
	put_float_by_index(msg, 8, ground_distance); // Ground distance in meters
	put_int16_t_by_index(msg, 12, flow_x); // Flow in pixels in x-sensor direction
	put_int16_t_by_index(msg, 14, flow_y); // Flow in pixels in y-sensor direction
	put_uint8_t_by_index(msg, 16, sensor_id); // Sensor ID
	put_uint8_t_by_index(msg, 17, quality); // Optical flow quality / confidence. 0: bad, 255: maximum quality

	mavlink_finalize_message_chan_send(msg, chan, 18, 146);
}
#endif // MAVLINK_USE_CONVENIENCE_FUNCTIONS


/**
 * @brief Encode a optical_flow struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param optical_flow C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_optical_flow_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_optical_flow_t* optical_flow)
{
	return mavlink_msg_optical_flow_pack(system_id, component_id, msg, optical_flow->time, optical_flow->sensor_id, optical_flow->flow_x, optical_flow->flow_y, optical_flow->quality, optical_flow->ground_distance);
}

/**
 * @brief Send a optical_flow message
 * @param chan MAVLink channel to send the message
 *
 * @param time Timestamp (UNIX)
 * @param sensor_id Sensor ID
 * @param flow_x Flow in pixels in x-sensor direction
 * @param flow_y Flow in pixels in y-sensor direction
 * @param quality Optical flow quality / confidence. 0: bad, 255: maximum quality
 * @param ground_distance Ground distance in meters
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_optical_flow_send(mavlink_channel_t chan, uint64_t time, uint8_t sensor_id, int16_t flow_x, int16_t flow_y, uint8_t quality, float ground_distance)
{
	MAVLINK_ALIGNED_MESSAGE(msg, 18);
	mavlink_msg_optical_flow_pack_chan_send(chan, msg, time, sensor_id, flow_x, flow_y, quality, ground_distance);
}

#endif

// MESSAGE OPTICAL_FLOW UNPACKING


/**
 * @brief Get field time from optical_flow message
 *
 * @return Timestamp (UNIX)
 */
static inline uint64_t mavlink_msg_optical_flow_get_time(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint64_t(msg,  0);
}

/**
 * @brief Get field sensor_id from optical_flow message
 *
 * @return Sensor ID
 */
static inline uint8_t mavlink_msg_optical_flow_get_sensor_id(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  16);
}

/**
 * @brief Get field flow_x from optical_flow message
 *
 * @return Flow in pixels in x-sensor direction
 */
static inline int16_t mavlink_msg_optical_flow_get_flow_x(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_int16_t(msg,  12);
}

/**
 * @brief Get field flow_y from optical_flow message
 *
 * @return Flow in pixels in y-sensor direction
 */
static inline int16_t mavlink_msg_optical_flow_get_flow_y(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_int16_t(msg,  14);
}

/**
 * @brief Get field quality from optical_flow message
 *
 * @return Optical flow quality / confidence. 0: bad, 255: maximum quality
 */
static inline uint8_t mavlink_msg_optical_flow_get_quality(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  17);
}

/**
 * @brief Get field ground_distance from optical_flow message
 *
 * @return Ground distance in meters
 */
static inline float mavlink_msg_optical_flow_get_ground_distance(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_float(msg,  8);
}

/**
 * @brief Decode a optical_flow message into a struct
 *
 * @param msg The message to decode
 * @param optical_flow C-struct to decode the message contents into
 */
static inline void mavlink_msg_optical_flow_decode(const mavlink_message_t* msg, mavlink_optical_flow_t* optical_flow)
{
#if MAVLINK_NEED_BYTE_SWAP
	optical_flow->time = mavlink_msg_optical_flow_get_time(msg);
	optical_flow->ground_distance = mavlink_msg_optical_flow_get_ground_distance(msg);
	optical_flow->flow_x = mavlink_msg_optical_flow_get_flow_x(msg);
	optical_flow->flow_y = mavlink_msg_optical_flow_get_flow_y(msg);
	optical_flow->sensor_id = mavlink_msg_optical_flow_get_sensor_id(msg);
	optical_flow->quality = mavlink_msg_optical_flow_get_quality(msg);
#else
	memcpy(optical_flow, MAVLINK_PAYLOAD(msg), 18);
#endif
}
