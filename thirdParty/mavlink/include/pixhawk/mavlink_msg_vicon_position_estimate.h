// MESSAGE VICON_POSITION_ESTIMATE PACKING

#define MAVLINK_MSG_ID_VICON_POSITION_ESTIMATE 112

typedef struct __mavlink_vicon_position_estimate_t 
{
	uint64_t usec; ///< Timestamp (milliseconds)
	float x; ///< Global X position
	float y; ///< Global Y position
	float z; ///< Global Z position
	float roll; ///< Roll angle in rad
	float pitch; ///< Pitch angle in rad
	float yaw; ///< Yaw angle in rad

} mavlink_vicon_position_estimate_t;



/**
 * @brief Pack a vicon_position_estimate message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param usec Timestamp (milliseconds)
 * @param x Global X position
 * @param y Global Y position
 * @param z Global Z position
 * @param roll Roll angle in rad
 * @param pitch Pitch angle in rad
 * @param yaw Yaw angle in rad
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_vicon_position_estimate_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint64_t usec, float x, float y, float z, float roll, float pitch, float yaw)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_VICON_POSITION_ESTIMATE;

	i += put_uint64_t_by_index(usec, i, msg->payload); // Timestamp (milliseconds)
	i += put_float_by_index(x, i, msg->payload); // Global X position
	i += put_float_by_index(y, i, msg->payload); // Global Y position
	i += put_float_by_index(z, i, msg->payload); // Global Z position
	i += put_float_by_index(roll, i, msg->payload); // Roll angle in rad
	i += put_float_by_index(pitch, i, msg->payload); // Pitch angle in rad
	i += put_float_by_index(yaw, i, msg->payload); // Yaw angle in rad

	return mavlink_finalize_message(msg, system_id, component_id, i);
}

/**
 * @brief Pack a vicon_position_estimate message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param usec Timestamp (milliseconds)
 * @param x Global X position
 * @param y Global Y position
 * @param z Global Z position
 * @param roll Roll angle in rad
 * @param pitch Pitch angle in rad
 * @param yaw Yaw angle in rad
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_vicon_position_estimate_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint64_t usec, float x, float y, float z, float roll, float pitch, float yaw)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_VICON_POSITION_ESTIMATE;

	i += put_uint64_t_by_index(usec, i, msg->payload); // Timestamp (milliseconds)
	i += put_float_by_index(x, i, msg->payload); // Global X position
	i += put_float_by_index(y, i, msg->payload); // Global Y position
	i += put_float_by_index(z, i, msg->payload); // Global Z position
	i += put_float_by_index(roll, i, msg->payload); // Roll angle in rad
	i += put_float_by_index(pitch, i, msg->payload); // Pitch angle in rad
	i += put_float_by_index(yaw, i, msg->payload); // Yaw angle in rad

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, i);
}

/**
 * @brief Encode a vicon_position_estimate struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param vicon_position_estimate C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_vicon_position_estimate_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_vicon_position_estimate_t* vicon_position_estimate)
{
	return mavlink_msg_vicon_position_estimate_pack(system_id, component_id, msg, vicon_position_estimate->usec, vicon_position_estimate->x, vicon_position_estimate->y, vicon_position_estimate->z, vicon_position_estimate->roll, vicon_position_estimate->pitch, vicon_position_estimate->yaw);
}

/**
 * @brief Send a vicon_position_estimate message
 * @param chan MAVLink channel to send the message
 *
 * @param usec Timestamp (milliseconds)
 * @param x Global X position
 * @param y Global Y position
 * @param z Global Z position
 * @param roll Roll angle in rad
 * @param pitch Pitch angle in rad
 * @param yaw Yaw angle in rad
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_vicon_position_estimate_send(mavlink_channel_t chan, uint64_t usec, float x, float y, float z, float roll, float pitch, float yaw)
{
	mavlink_message_t msg;
	mavlink_msg_vicon_position_estimate_pack_chan(mavlink_system.sysid, mavlink_system.compid, chan, &msg, usec, x, y, z, roll, pitch, yaw);
	mavlink_send_uart(chan, &msg);
}

#endif
// MESSAGE VICON_POSITION_ESTIMATE UNPACKING

/**
 * @brief Get field usec from vicon_position_estimate message
 *
 * @return Timestamp (milliseconds)
 */
static inline uint64_t mavlink_msg_vicon_position_estimate_get_usec(const mavlink_message_t* msg)
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
 * @brief Get field x from vicon_position_estimate message
 *
 * @return Global X position
 */
static inline float mavlink_msg_vicon_position_estimate_get_x(const mavlink_message_t* msg)
{
	generic_32bit r;
	r.b[3] = (msg->payload+sizeof(uint64_t))[0];
	r.b[2] = (msg->payload+sizeof(uint64_t))[1];
	r.b[1] = (msg->payload+sizeof(uint64_t))[2];
	r.b[0] = (msg->payload+sizeof(uint64_t))[3];
	return (float)r.f;
}

/**
 * @brief Get field y from vicon_position_estimate message
 *
 * @return Global Y position
 */
static inline float mavlink_msg_vicon_position_estimate_get_y(const mavlink_message_t* msg)
{
	generic_32bit r;
	r.b[3] = (msg->payload+sizeof(uint64_t)+sizeof(float))[0];
	r.b[2] = (msg->payload+sizeof(uint64_t)+sizeof(float))[1];
	r.b[1] = (msg->payload+sizeof(uint64_t)+sizeof(float))[2];
	r.b[0] = (msg->payload+sizeof(uint64_t)+sizeof(float))[3];
	return (float)r.f;
}

/**
 * @brief Get field z from vicon_position_estimate message
 *
 * @return Global Z position
 */
static inline float mavlink_msg_vicon_position_estimate_get_z(const mavlink_message_t* msg)
{
	generic_32bit r;
	r.b[3] = (msg->payload+sizeof(uint64_t)+sizeof(float)+sizeof(float))[0];
	r.b[2] = (msg->payload+sizeof(uint64_t)+sizeof(float)+sizeof(float))[1];
	r.b[1] = (msg->payload+sizeof(uint64_t)+sizeof(float)+sizeof(float))[2];
	r.b[0] = (msg->payload+sizeof(uint64_t)+sizeof(float)+sizeof(float))[3];
	return (float)r.f;
}

/**
 * @brief Get field roll from vicon_position_estimate message
 *
 * @return Roll angle in rad
 */
static inline float mavlink_msg_vicon_position_estimate_get_roll(const mavlink_message_t* msg)
{
	generic_32bit r;
	r.b[3] = (msg->payload+sizeof(uint64_t)+sizeof(float)+sizeof(float)+sizeof(float))[0];
	r.b[2] = (msg->payload+sizeof(uint64_t)+sizeof(float)+sizeof(float)+sizeof(float))[1];
	r.b[1] = (msg->payload+sizeof(uint64_t)+sizeof(float)+sizeof(float)+sizeof(float))[2];
	r.b[0] = (msg->payload+sizeof(uint64_t)+sizeof(float)+sizeof(float)+sizeof(float))[3];
	return (float)r.f;
}

/**
 * @brief Get field pitch from vicon_position_estimate message
 *
 * @return Pitch angle in rad
 */
static inline float mavlink_msg_vicon_position_estimate_get_pitch(const mavlink_message_t* msg)
{
	generic_32bit r;
	r.b[3] = (msg->payload+sizeof(uint64_t)+sizeof(float)+sizeof(float)+sizeof(float)+sizeof(float))[0];
	r.b[2] = (msg->payload+sizeof(uint64_t)+sizeof(float)+sizeof(float)+sizeof(float)+sizeof(float))[1];
	r.b[1] = (msg->payload+sizeof(uint64_t)+sizeof(float)+sizeof(float)+sizeof(float)+sizeof(float))[2];
	r.b[0] = (msg->payload+sizeof(uint64_t)+sizeof(float)+sizeof(float)+sizeof(float)+sizeof(float))[3];
	return (float)r.f;
}

/**
 * @brief Get field yaw from vicon_position_estimate message
 *
 * @return Yaw angle in rad
 */
static inline float mavlink_msg_vicon_position_estimate_get_yaw(const mavlink_message_t* msg)
{
	generic_32bit r;
	r.b[3] = (msg->payload+sizeof(uint64_t)+sizeof(float)+sizeof(float)+sizeof(float)+sizeof(float)+sizeof(float))[0];
	r.b[2] = (msg->payload+sizeof(uint64_t)+sizeof(float)+sizeof(float)+sizeof(float)+sizeof(float)+sizeof(float))[1];
	r.b[1] = (msg->payload+sizeof(uint64_t)+sizeof(float)+sizeof(float)+sizeof(float)+sizeof(float)+sizeof(float))[2];
	r.b[0] = (msg->payload+sizeof(uint64_t)+sizeof(float)+sizeof(float)+sizeof(float)+sizeof(float)+sizeof(float))[3];
	return (float)r.f;
}

/**
 * @brief Decode a vicon_position_estimate message into a struct
 *
 * @param msg The message to decode
 * @param vicon_position_estimate C-struct to decode the message contents into
 */
static inline void mavlink_msg_vicon_position_estimate_decode(const mavlink_message_t* msg, mavlink_vicon_position_estimate_t* vicon_position_estimate)
{
	vicon_position_estimate->usec = mavlink_msg_vicon_position_estimate_get_usec(msg);
	vicon_position_estimate->x = mavlink_msg_vicon_position_estimate_get_x(msg);
	vicon_position_estimate->y = mavlink_msg_vicon_position_estimate_get_y(msg);
	vicon_position_estimate->z = mavlink_msg_vicon_position_estimate_get_z(msg);
	vicon_position_estimate->roll = mavlink_msg_vicon_position_estimate_get_roll(msg);
	vicon_position_estimate->pitch = mavlink_msg_vicon_position_estimate_get_pitch(msg);
	vicon_position_estimate->yaw = mavlink_msg_vicon_position_estimate_get_yaw(msg);
}
