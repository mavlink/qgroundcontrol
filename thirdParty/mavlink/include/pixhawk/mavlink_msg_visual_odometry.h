// MESSAGE VISUAL_ODOMETRY PACKING

#define MAVLINK_MSG_ID_VISUAL_ODOMETRY 180

typedef struct __mavlink_visual_odometry_t 
{
	uint64_t frame1_time_us; ///< Time at which frame 1 was captured (in microseconds since unix epoch)
	uint64_t frame2_time_us; ///< Time at which frame 2 was captured (in microseconds since unix epoch)
	float x; ///< Relative X position
	float y; ///< Relative Y position
	float z; ///< Relative Z position
	float roll; ///< Relative roll angle in rad
	float pitch; ///< Relative pitch angle in rad
	float yaw; ///< Relative yaw angle in rad

} mavlink_visual_odometry_t;



/**
 * @brief Pack a visual_odometry message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param frame1_time_us Time at which frame 1 was captured (in microseconds since unix epoch)
 * @param frame2_time_us Time at which frame 2 was captured (in microseconds since unix epoch)
 * @param x Relative X position
 * @param y Relative Y position
 * @param z Relative Z position
 * @param roll Relative roll angle in rad
 * @param pitch Relative pitch angle in rad
 * @param yaw Relative yaw angle in rad
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_visual_odometry_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint64_t frame1_time_us, uint64_t frame2_time_us, float x, float y, float z, float roll, float pitch, float yaw)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_VISUAL_ODOMETRY;

	i += put_uint64_t_by_index(frame1_time_us, i, msg->payload); // Time at which frame 1 was captured (in microseconds since unix epoch)
	i += put_uint64_t_by_index(frame2_time_us, i, msg->payload); // Time at which frame 2 was captured (in microseconds since unix epoch)
	i += put_float_by_index(x, i, msg->payload); // Relative X position
	i += put_float_by_index(y, i, msg->payload); // Relative Y position
	i += put_float_by_index(z, i, msg->payload); // Relative Z position
	i += put_float_by_index(roll, i, msg->payload); // Relative roll angle in rad
	i += put_float_by_index(pitch, i, msg->payload); // Relative pitch angle in rad
	i += put_float_by_index(yaw, i, msg->payload); // Relative yaw angle in rad

	return mavlink_finalize_message(msg, system_id, component_id, i);
}

/**
 * @brief Pack a visual_odometry message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param frame1_time_us Time at which frame 1 was captured (in microseconds since unix epoch)
 * @param frame2_time_us Time at which frame 2 was captured (in microseconds since unix epoch)
 * @param x Relative X position
 * @param y Relative Y position
 * @param z Relative Z position
 * @param roll Relative roll angle in rad
 * @param pitch Relative pitch angle in rad
 * @param yaw Relative yaw angle in rad
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_visual_odometry_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint64_t frame1_time_us, uint64_t frame2_time_us, float x, float y, float z, float roll, float pitch, float yaw)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_VISUAL_ODOMETRY;

	i += put_uint64_t_by_index(frame1_time_us, i, msg->payload); // Time at which frame 1 was captured (in microseconds since unix epoch)
	i += put_uint64_t_by_index(frame2_time_us, i, msg->payload); // Time at which frame 2 was captured (in microseconds since unix epoch)
	i += put_float_by_index(x, i, msg->payload); // Relative X position
	i += put_float_by_index(y, i, msg->payload); // Relative Y position
	i += put_float_by_index(z, i, msg->payload); // Relative Z position
	i += put_float_by_index(roll, i, msg->payload); // Relative roll angle in rad
	i += put_float_by_index(pitch, i, msg->payload); // Relative pitch angle in rad
	i += put_float_by_index(yaw, i, msg->payload); // Relative yaw angle in rad

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, i);
}

/**
 * @brief Encode a visual_odometry struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param visual_odometry C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_visual_odometry_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_visual_odometry_t* visual_odometry)
{
	return mavlink_msg_visual_odometry_pack(system_id, component_id, msg, visual_odometry->frame1_time_us, visual_odometry->frame2_time_us, visual_odometry->x, visual_odometry->y, visual_odometry->z, visual_odometry->roll, visual_odometry->pitch, visual_odometry->yaw);
}

/**
 * @brief Send a visual_odometry message
 * @param chan MAVLink channel to send the message
 *
 * @param frame1_time_us Time at which frame 1 was captured (in microseconds since unix epoch)
 * @param frame2_time_us Time at which frame 2 was captured (in microseconds since unix epoch)
 * @param x Relative X position
 * @param y Relative Y position
 * @param z Relative Z position
 * @param roll Relative roll angle in rad
 * @param pitch Relative pitch angle in rad
 * @param yaw Relative yaw angle in rad
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_visual_odometry_send(mavlink_channel_t chan, uint64_t frame1_time_us, uint64_t frame2_time_us, float x, float y, float z, float roll, float pitch, float yaw)
{
	mavlink_message_t msg;
	mavlink_msg_visual_odometry_pack_chan(mavlink_system.sysid, mavlink_system.compid, chan, &msg, frame1_time_us, frame2_time_us, x, y, z, roll, pitch, yaw);
	mavlink_send_uart(chan, &msg);
}

#endif
// MESSAGE VISUAL_ODOMETRY UNPACKING

/**
 * @brief Get field frame1_time_us from visual_odometry message
 *
 * @return Time at which frame 1 was captured (in microseconds since unix epoch)
 */
static inline uint64_t mavlink_msg_visual_odometry_get_frame1_time_us(const mavlink_message_t* msg)
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
 * @brief Get field frame2_time_us from visual_odometry message
 *
 * @return Time at which frame 2 was captured (in microseconds since unix epoch)
 */
static inline uint64_t mavlink_msg_visual_odometry_get_frame2_time_us(const mavlink_message_t* msg)
{
	generic_64bit r;
	r.b[7] = (msg->payload+sizeof(uint64_t))[0];
	r.b[6] = (msg->payload+sizeof(uint64_t))[1];
	r.b[5] = (msg->payload+sizeof(uint64_t))[2];
	r.b[4] = (msg->payload+sizeof(uint64_t))[3];
	r.b[3] = (msg->payload+sizeof(uint64_t))[4];
	r.b[2] = (msg->payload+sizeof(uint64_t))[5];
	r.b[1] = (msg->payload+sizeof(uint64_t))[6];
	r.b[0] = (msg->payload+sizeof(uint64_t))[7];
	return (uint64_t)r.ll;
}

/**
 * @brief Get field x from visual_odometry message
 *
 * @return Relative X position
 */
static inline float mavlink_msg_visual_odometry_get_x(const mavlink_message_t* msg)
{
	generic_32bit r;
	r.b[3] = (msg->payload+sizeof(uint64_t)+sizeof(uint64_t))[0];
	r.b[2] = (msg->payload+sizeof(uint64_t)+sizeof(uint64_t))[1];
	r.b[1] = (msg->payload+sizeof(uint64_t)+sizeof(uint64_t))[2];
	r.b[0] = (msg->payload+sizeof(uint64_t)+sizeof(uint64_t))[3];
	return (float)r.f;
}

/**
 * @brief Get field y from visual_odometry message
 *
 * @return Relative Y position
 */
static inline float mavlink_msg_visual_odometry_get_y(const mavlink_message_t* msg)
{
	generic_32bit r;
	r.b[3] = (msg->payload+sizeof(uint64_t)+sizeof(uint64_t)+sizeof(float))[0];
	r.b[2] = (msg->payload+sizeof(uint64_t)+sizeof(uint64_t)+sizeof(float))[1];
	r.b[1] = (msg->payload+sizeof(uint64_t)+sizeof(uint64_t)+sizeof(float))[2];
	r.b[0] = (msg->payload+sizeof(uint64_t)+sizeof(uint64_t)+sizeof(float))[3];
	return (float)r.f;
}

/**
 * @brief Get field z from visual_odometry message
 *
 * @return Relative Z position
 */
static inline float mavlink_msg_visual_odometry_get_z(const mavlink_message_t* msg)
{
	generic_32bit r;
	r.b[3] = (msg->payload+sizeof(uint64_t)+sizeof(uint64_t)+sizeof(float)+sizeof(float))[0];
	r.b[2] = (msg->payload+sizeof(uint64_t)+sizeof(uint64_t)+sizeof(float)+sizeof(float))[1];
	r.b[1] = (msg->payload+sizeof(uint64_t)+sizeof(uint64_t)+sizeof(float)+sizeof(float))[2];
	r.b[0] = (msg->payload+sizeof(uint64_t)+sizeof(uint64_t)+sizeof(float)+sizeof(float))[3];
	return (float)r.f;
}

/**
 * @brief Get field roll from visual_odometry message
 *
 * @return Relative roll angle in rad
 */
static inline float mavlink_msg_visual_odometry_get_roll(const mavlink_message_t* msg)
{
	generic_32bit r;
	r.b[3] = (msg->payload+sizeof(uint64_t)+sizeof(uint64_t)+sizeof(float)+sizeof(float)+sizeof(float))[0];
	r.b[2] = (msg->payload+sizeof(uint64_t)+sizeof(uint64_t)+sizeof(float)+sizeof(float)+sizeof(float))[1];
	r.b[1] = (msg->payload+sizeof(uint64_t)+sizeof(uint64_t)+sizeof(float)+sizeof(float)+sizeof(float))[2];
	r.b[0] = (msg->payload+sizeof(uint64_t)+sizeof(uint64_t)+sizeof(float)+sizeof(float)+sizeof(float))[3];
	return (float)r.f;
}

/**
 * @brief Get field pitch from visual_odometry message
 *
 * @return Relative pitch angle in rad
 */
static inline float mavlink_msg_visual_odometry_get_pitch(const mavlink_message_t* msg)
{
	generic_32bit r;
	r.b[3] = (msg->payload+sizeof(uint64_t)+sizeof(uint64_t)+sizeof(float)+sizeof(float)+sizeof(float)+sizeof(float))[0];
	r.b[2] = (msg->payload+sizeof(uint64_t)+sizeof(uint64_t)+sizeof(float)+sizeof(float)+sizeof(float)+sizeof(float))[1];
	r.b[1] = (msg->payload+sizeof(uint64_t)+sizeof(uint64_t)+sizeof(float)+sizeof(float)+sizeof(float)+sizeof(float))[2];
	r.b[0] = (msg->payload+sizeof(uint64_t)+sizeof(uint64_t)+sizeof(float)+sizeof(float)+sizeof(float)+sizeof(float))[3];
	return (float)r.f;
}

/**
 * @brief Get field yaw from visual_odometry message
 *
 * @return Relative yaw angle in rad
 */
static inline float mavlink_msg_visual_odometry_get_yaw(const mavlink_message_t* msg)
{
	generic_32bit r;
	r.b[3] = (msg->payload+sizeof(uint64_t)+sizeof(uint64_t)+sizeof(float)+sizeof(float)+sizeof(float)+sizeof(float)+sizeof(float))[0];
	r.b[2] = (msg->payload+sizeof(uint64_t)+sizeof(uint64_t)+sizeof(float)+sizeof(float)+sizeof(float)+sizeof(float)+sizeof(float))[1];
	r.b[1] = (msg->payload+sizeof(uint64_t)+sizeof(uint64_t)+sizeof(float)+sizeof(float)+sizeof(float)+sizeof(float)+sizeof(float))[2];
	r.b[0] = (msg->payload+sizeof(uint64_t)+sizeof(uint64_t)+sizeof(float)+sizeof(float)+sizeof(float)+sizeof(float)+sizeof(float))[3];
	return (float)r.f;
}

/**
 * @brief Decode a visual_odometry message into a struct
 *
 * @param msg The message to decode
 * @param visual_odometry C-struct to decode the message contents into
 */
static inline void mavlink_msg_visual_odometry_decode(const mavlink_message_t* msg, mavlink_visual_odometry_t* visual_odometry)
{
	visual_odometry->frame1_time_us = mavlink_msg_visual_odometry_get_frame1_time_us(msg);
	visual_odometry->frame2_time_us = mavlink_msg_visual_odometry_get_frame2_time_us(msg);
	visual_odometry->x = mavlink_msg_visual_odometry_get_x(msg);
	visual_odometry->y = mavlink_msg_visual_odometry_get_y(msg);
	visual_odometry->z = mavlink_msg_visual_odometry_get_z(msg);
	visual_odometry->roll = mavlink_msg_visual_odometry_get_roll(msg);
	visual_odometry->pitch = mavlink_msg_visual_odometry_get_pitch(msg);
	visual_odometry->yaw = mavlink_msg_visual_odometry_get_yaw(msg);
}
