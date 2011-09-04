// MESSAGE OPTICAL_FLOW PACKING

#define MAVLINK_MSG_ID_OPTICAL_FLOW 100

typedef struct __mavlink_optical_flow_t 
{
	uint64_t time; ///< Timestamp (UNIX)
	uint8_t sensor_id; ///< Sensor ID
	int16_t flow_x; ///< Flow in pixels in x-sensor direction
	int16_t flow_y; ///< Flow in pixels in y-sensor direction
	uint8_t quality; ///< Optical flow quality / confidence. 0: bad, 255: maximum quality
	float ground_distance; ///< Ground distance in meters

} mavlink_optical_flow_t;



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
static inline uint16_t mavlink_msg_optical_flow_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint64_t time, uint8_t sensor_id, int16_t flow_x, int16_t flow_y, uint8_t quality, float ground_distance)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_OPTICAL_FLOW;

	i += put_uint64_t_by_index(time, i, msg->payload); // Timestamp (UNIX)
	i += put_uint8_t_by_index(sensor_id, i, msg->payload); // Sensor ID
	i += put_int16_t_by_index(flow_x, i, msg->payload); // Flow in pixels in x-sensor direction
	i += put_int16_t_by_index(flow_y, i, msg->payload); // Flow in pixels in y-sensor direction
	i += put_uint8_t_by_index(quality, i, msg->payload); // Optical flow quality / confidence. 0: bad, 255: maximum quality
	i += put_float_by_index(ground_distance, i, msg->payload); // Ground distance in meters

	return mavlink_finalize_message(msg, system_id, component_id, i);
}

/**
 * @brief Pack a optical_flow message
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
static inline uint16_t mavlink_msg_optical_flow_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint64_t time, uint8_t sensor_id, int16_t flow_x, int16_t flow_y, uint8_t quality, float ground_distance)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_OPTICAL_FLOW;

	i += put_uint64_t_by_index(time, i, msg->payload); // Timestamp (UNIX)
	i += put_uint8_t_by_index(sensor_id, i, msg->payload); // Sensor ID
	i += put_int16_t_by_index(flow_x, i, msg->payload); // Flow in pixels in x-sensor direction
	i += put_int16_t_by_index(flow_y, i, msg->payload); // Flow in pixels in y-sensor direction
	i += put_uint8_t_by_index(quality, i, msg->payload); // Optical flow quality / confidence. 0: bad, 255: maximum quality
	i += put_float_by_index(ground_distance, i, msg->payload); // Ground distance in meters

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, i);
}

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
	mavlink_message_t msg;
	mavlink_msg_optical_flow_pack_chan(mavlink_system.sysid, mavlink_system.compid, chan, &msg, time, sensor_id, flow_x, flow_y, quality, ground_distance);
	mavlink_send_uart(chan, &msg);
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
	generic_64bit r;
	r.b[7] = (msg->payload)[0];
	r.b[6] = (msg->payload)[1];
	r.b[5] = (msg->payload)[2];
	r.b[4] = (msg->payload)[3];
	r.b[3] = (msg->payload)[4];
	r.b[2] = (msg->payload)[5];
	r.b[1] = (msg->payload)[6];
	r.b[0] = (msg->payload)[7];
	return (uint64_t)r.ll;
}

/**
 * @brief Get field sensor_id from optical_flow message
 *
 * @return Sensor ID
 */
static inline uint8_t mavlink_msg_optical_flow_get_sensor_id(const mavlink_message_t* msg)
{
	return (uint8_t)(msg->payload+sizeof(uint64_t))[0];
}

/**
 * @brief Get field flow_x from optical_flow message
 *
 * @return Flow in pixels in x-sensor direction
 */
static inline int16_t mavlink_msg_optical_flow_get_flow_x(const mavlink_message_t* msg)
{
	generic_16bit r;
	r.b[1] = (msg->payload+sizeof(uint64_t)+sizeof(uint8_t))[0];
	r.b[0] = (msg->payload+sizeof(uint64_t)+sizeof(uint8_t))[1];
	return (int16_t)r.s;
}

/**
 * @brief Get field flow_y from optical_flow message
 *
 * @return Flow in pixels in y-sensor direction
 */
static inline int16_t mavlink_msg_optical_flow_get_flow_y(const mavlink_message_t* msg)
{
	generic_16bit r;
	r.b[1] = (msg->payload+sizeof(uint64_t)+sizeof(uint8_t)+sizeof(int16_t))[0];
	r.b[0] = (msg->payload+sizeof(uint64_t)+sizeof(uint8_t)+sizeof(int16_t))[1];
	return (int16_t)r.s;
}

/**
 * @brief Get field quality from optical_flow message
 *
 * @return Optical flow quality / confidence. 0: bad, 255: maximum quality
 */
static inline uint8_t mavlink_msg_optical_flow_get_quality(const mavlink_message_t* msg)
{
	return (uint8_t)(msg->payload+sizeof(uint64_t)+sizeof(uint8_t)+sizeof(int16_t)+sizeof(int16_t))[0];
}

/**
 * @brief Get field ground_distance from optical_flow message
 *
 * @return Ground distance in meters
 */
static inline float mavlink_msg_optical_flow_get_ground_distance(const mavlink_message_t* msg)
{
	generic_32bit r;
	r.b[3] = (msg->payload+sizeof(uint64_t)+sizeof(uint8_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(uint8_t))[0];
	r.b[2] = (msg->payload+sizeof(uint64_t)+sizeof(uint8_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(uint8_t))[1];
	r.b[1] = (msg->payload+sizeof(uint64_t)+sizeof(uint8_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(uint8_t))[2];
	r.b[0] = (msg->payload+sizeof(uint64_t)+sizeof(uint8_t)+sizeof(int16_t)+sizeof(int16_t)+sizeof(uint8_t))[3];
	return (float)r.f;
}

/**
 * @brief Decode a optical_flow message into a struct
 *
 * @param msg The message to decode
 * @param optical_flow C-struct to decode the message contents into
 */
static inline void mavlink_msg_optical_flow_decode(const mavlink_message_t* msg, mavlink_optical_flow_t* optical_flow)
{
	optical_flow->time = mavlink_msg_optical_flow_get_time(msg);
	optical_flow->sensor_id = mavlink_msg_optical_flow_get_sensor_id(msg);
	optical_flow->flow_x = mavlink_msg_optical_flow_get_flow_x(msg);
	optical_flow->flow_y = mavlink_msg_optical_flow_get_flow_y(msg);
	optical_flow->quality = mavlink_msg_optical_flow_get_quality(msg);
	optical_flow->ground_distance = mavlink_msg_optical_flow_get_ground_distance(msg);
}
