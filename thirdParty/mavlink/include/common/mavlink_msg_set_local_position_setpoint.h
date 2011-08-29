// MESSAGE SET_LOCAL_POSITION_SETPOINT PACKING

#define MAVLINK_MSG_ID_SET_LOCAL_POSITION_SETPOINT 50

typedef struct __mavlink_set_local_position_setpoint_t
{
 float x; ///< x position
 float y; ///< y position
 float z; ///< z position
 float yaw; ///< Desired yaw angle
 uint8_t target_system; ///< System ID
 uint8_t target_component; ///< Component ID
} mavlink_set_local_position_setpoint_t;

#define MAVLINK_MSG_ID_SET_LOCAL_POSITION_SETPOINT_LEN 18
#define MAVLINK_MSG_ID_50_LEN 18



#define MAVLINK_MESSAGE_INFO_SET_LOCAL_POSITION_SETPOINT { \
	"SET_LOCAL_POSITION_SETPOINT", \
	6, \
	{  { "x", MAVLINK_TYPE_FLOAT, 0, 0, offsetof(mavlink_set_local_position_setpoint_t, x) }, \
         { "y", MAVLINK_TYPE_FLOAT, 0, 4, offsetof(mavlink_set_local_position_setpoint_t, y) }, \
         { "z", MAVLINK_TYPE_FLOAT, 0, 8, offsetof(mavlink_set_local_position_setpoint_t, z) }, \
         { "yaw", MAVLINK_TYPE_FLOAT, 0, 12, offsetof(mavlink_set_local_position_setpoint_t, yaw) }, \
         { "target_system", MAVLINK_TYPE_UINT8_T, 0, 16, offsetof(mavlink_set_local_position_setpoint_t, target_system) }, \
         { "target_component", MAVLINK_TYPE_UINT8_T, 0, 17, offsetof(mavlink_set_local_position_setpoint_t, target_component) }, \
         } \
}


/**
 * @brief Pack a set_local_position_setpoint message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param x x position
 * @param y y position
 * @param z z position
 * @param yaw Desired yaw angle
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_set_local_position_setpoint_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint8_t target_system, uint8_t target_component, float x, float y, float z, float yaw)
{
	msg->msgid = MAVLINK_MSG_ID_SET_LOCAL_POSITION_SETPOINT;

	put_float_by_index(msg, 0, x); // x position
	put_float_by_index(msg, 4, y); // y position
	put_float_by_index(msg, 8, z); // z position
	put_float_by_index(msg, 12, yaw); // Desired yaw angle
	put_uint8_t_by_index(msg, 16, target_system); // System ID
	put_uint8_t_by_index(msg, 17, target_component); // Component ID

	return mavlink_finalize_message(msg, system_id, component_id, 18, 131);
}

/**
 * @brief Pack a set_local_position_setpoint message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system System ID
 * @param target_component Component ID
 * @param x x position
 * @param y y position
 * @param z z position
 * @param yaw Desired yaw angle
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_set_local_position_setpoint_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint8_t target_system,uint8_t target_component,float x,float y,float z,float yaw)
{
	msg->msgid = MAVLINK_MSG_ID_SET_LOCAL_POSITION_SETPOINT;

	put_float_by_index(msg, 0, x); // x position
	put_float_by_index(msg, 4, y); // y position
	put_float_by_index(msg, 8, z); // z position
	put_float_by_index(msg, 12, yaw); // Desired yaw angle
	put_uint8_t_by_index(msg, 16, target_system); // System ID
	put_uint8_t_by_index(msg, 17, target_component); // Component ID

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, 18, 131);
}

/**
 * @brief Encode a set_local_position_setpoint struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param set_local_position_setpoint C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_set_local_position_setpoint_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_set_local_position_setpoint_t* set_local_position_setpoint)
{
	return mavlink_msg_set_local_position_setpoint_pack(system_id, component_id, msg, set_local_position_setpoint->target_system, set_local_position_setpoint->target_component, set_local_position_setpoint->x, set_local_position_setpoint->y, set_local_position_setpoint->z, set_local_position_setpoint->yaw);
}

/**
 * @brief Send a set_local_position_setpoint message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param x x position
 * @param y y position
 * @param z z position
 * @param yaw Desired yaw angle
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_set_local_position_setpoint_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, float x, float y, float z, float yaw)
{
	MAVLINK_ALIGNED_MESSAGE(msg, 18);
	msg->msgid = MAVLINK_MSG_ID_SET_LOCAL_POSITION_SETPOINT;

	put_float_by_index(msg, 0, x); // x position
	put_float_by_index(msg, 4, y); // y position
	put_float_by_index(msg, 8, z); // z position
	put_float_by_index(msg, 12, yaw); // Desired yaw angle
	put_uint8_t_by_index(msg, 16, target_system); // System ID
	put_uint8_t_by_index(msg, 17, target_component); // Component ID

	mavlink_finalize_message_chan_send(msg, chan, 18, 131);
}

#endif

// MESSAGE SET_LOCAL_POSITION_SETPOINT UNPACKING


/**
 * @brief Get field target_system from set_local_position_setpoint message
 *
 * @return System ID
 */
static inline uint8_t mavlink_msg_set_local_position_setpoint_get_target_system(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  16);
}

/**
 * @brief Get field target_component from set_local_position_setpoint message
 *
 * @return Component ID
 */
static inline uint8_t mavlink_msg_set_local_position_setpoint_get_target_component(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  17);
}

/**
 * @brief Get field x from set_local_position_setpoint message
 *
 * @return x position
 */
static inline float mavlink_msg_set_local_position_setpoint_get_x(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_float(msg,  0);
}

/**
 * @brief Get field y from set_local_position_setpoint message
 *
 * @return y position
 */
static inline float mavlink_msg_set_local_position_setpoint_get_y(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_float(msg,  4);
}

/**
 * @brief Get field z from set_local_position_setpoint message
 *
 * @return z position
 */
static inline float mavlink_msg_set_local_position_setpoint_get_z(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_float(msg,  8);
}

/**
 * @brief Get field yaw from set_local_position_setpoint message
 *
 * @return Desired yaw angle
 */
static inline float mavlink_msg_set_local_position_setpoint_get_yaw(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_float(msg,  12);
}

/**
 * @brief Decode a set_local_position_setpoint message into a struct
 *
 * @param msg The message to decode
 * @param set_local_position_setpoint C-struct to decode the message contents into
 */
static inline void mavlink_msg_set_local_position_setpoint_decode(const mavlink_message_t* msg, mavlink_set_local_position_setpoint_t* set_local_position_setpoint)
{
#if MAVLINK_NEED_BYTE_SWAP
	set_local_position_setpoint->x = mavlink_msg_set_local_position_setpoint_get_x(msg);
	set_local_position_setpoint->y = mavlink_msg_set_local_position_setpoint_get_y(msg);
	set_local_position_setpoint->z = mavlink_msg_set_local_position_setpoint_get_z(msg);
	set_local_position_setpoint->yaw = mavlink_msg_set_local_position_setpoint_get_yaw(msg);
	set_local_position_setpoint->target_system = mavlink_msg_set_local_position_setpoint_get_target_system(msg);
	set_local_position_setpoint->target_component = mavlink_msg_set_local_position_setpoint_get_target_component(msg);
#else
	memcpy(set_local_position_setpoint, MAVLINK_PAYLOAD(msg), 18);
#endif
}
