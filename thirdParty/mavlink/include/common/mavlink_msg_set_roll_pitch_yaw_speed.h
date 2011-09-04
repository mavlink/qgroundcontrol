// MESSAGE SET_ROLL_PITCH_YAW_SPEED PACKING

#define MAVLINK_MSG_ID_SET_ROLL_PITCH_YAW_SPEED 56

typedef struct __mavlink_set_roll_pitch_yaw_speed_t 
{
	uint8_t target_system; ///< System ID
	uint8_t target_component; ///< Component ID
	float roll_speed; ///< Desired roll angular speed in rad/s
	float pitch_speed; ///< Desired pitch angular speed in rad/s
	float yaw_speed; ///< Desired yaw angular speed in rad/s

} mavlink_set_roll_pitch_yaw_speed_t;



/**
 * @brief Pack a set_roll_pitch_yaw_speed message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param roll_speed Desired roll angular speed in rad/s
 * @param pitch_speed Desired pitch angular speed in rad/s
 * @param yaw_speed Desired yaw angular speed in rad/s
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_set_roll_pitch_yaw_speed_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component, float roll_speed, float pitch_speed, float yaw_speed)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_SET_ROLL_PITCH_YAW_SPEED;

	i += put_uint8_t_by_index(target_system, i, msg->payload); // System ID
	i += put_uint8_t_by_index(target_component, i, msg->payload); // Component ID
	i += put_float_by_index(roll_speed, i, msg->payload); // Desired roll angular speed in rad/s
	i += put_float_by_index(pitch_speed, i, msg->payload); // Desired pitch angular speed in rad/s
	i += put_float_by_index(yaw_speed, i, msg->payload); // Desired yaw angular speed in rad/s

	return mavlink_finalize_message(msg, system_id, component_id, i);
}

/**
 * @brief Pack a set_roll_pitch_yaw_speed message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target_system System ID
 * @param target_component Component ID
 * @param roll_speed Desired roll angular speed in rad/s
 * @param pitch_speed Desired pitch angular speed in rad/s
 * @param yaw_speed Desired yaw angular speed in rad/s
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_set_roll_pitch_yaw_speed_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan, mavlink_message_t* msg, uint8_t target_system, uint8_t target_component, float roll_speed, float pitch_speed, float yaw_speed)
{
	uint16_t i = 0;
	msg->msgid = MAVLINK_MSG_ID_SET_ROLL_PITCH_YAW_SPEED;

	i += put_uint8_t_by_index(target_system, i, msg->payload); // System ID
	i += put_uint8_t_by_index(target_component, i, msg->payload); // Component ID
	i += put_float_by_index(roll_speed, i, msg->payload); // Desired roll angular speed in rad/s
	i += put_float_by_index(pitch_speed, i, msg->payload); // Desired pitch angular speed in rad/s
	i += put_float_by_index(yaw_speed, i, msg->payload); // Desired yaw angular speed in rad/s

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, i);
}

/**
 * @brief Encode a set_roll_pitch_yaw_speed struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param set_roll_pitch_yaw_speed C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_set_roll_pitch_yaw_speed_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_set_roll_pitch_yaw_speed_t* set_roll_pitch_yaw_speed)
{
	return mavlink_msg_set_roll_pitch_yaw_speed_pack(system_id, component_id, msg, set_roll_pitch_yaw_speed->target_system, set_roll_pitch_yaw_speed->target_component, set_roll_pitch_yaw_speed->roll_speed, set_roll_pitch_yaw_speed->pitch_speed, set_roll_pitch_yaw_speed->yaw_speed);
}

/**
 * @brief Send a set_roll_pitch_yaw_speed message
 * @param chan MAVLink channel to send the message
 *
 * @param target_system System ID
 * @param target_component Component ID
 * @param roll_speed Desired roll angular speed in rad/s
 * @param pitch_speed Desired pitch angular speed in rad/s
 * @param yaw_speed Desired yaw angular speed in rad/s
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_set_roll_pitch_yaw_speed_send(mavlink_channel_t chan, uint8_t target_system, uint8_t target_component, float roll_speed, float pitch_speed, float yaw_speed)
{
	mavlink_message_t msg;
	mavlink_msg_set_roll_pitch_yaw_speed_pack_chan(mavlink_system.sysid, mavlink_system.compid, chan, &msg, target_system, target_component, roll_speed, pitch_speed, yaw_speed);
	mavlink_send_uart(chan, &msg);
}

#endif
// MESSAGE SET_ROLL_PITCH_YAW_SPEED UNPACKING

/**
 * @brief Get field target_system from set_roll_pitch_yaw_speed message
 *
 * @return System ID
 */
static inline uint8_t mavlink_msg_set_roll_pitch_yaw_speed_get_target_system(const mavlink_message_t* msg)
{
	return (uint8_t)(msg->payload)[0];
}

/**
 * @brief Get field target_component from set_roll_pitch_yaw_speed message
 *
 * @return Component ID
 */
static inline uint8_t mavlink_msg_set_roll_pitch_yaw_speed_get_target_component(const mavlink_message_t* msg)
{
	return (uint8_t)(msg->payload+sizeof(uint8_t))[0];
}

/**
 * @brief Get field roll_speed from set_roll_pitch_yaw_speed message
 *
 * @return Desired roll angular speed in rad/s
 */
static inline float mavlink_msg_set_roll_pitch_yaw_speed_get_roll_speed(const mavlink_message_t* msg)
{
	generic_32bit r;
	r.b[3] = (msg->payload+sizeof(uint8_t)+sizeof(uint8_t))[0];
	r.b[2] = (msg->payload+sizeof(uint8_t)+sizeof(uint8_t))[1];
	r.b[1] = (msg->payload+sizeof(uint8_t)+sizeof(uint8_t))[2];
	r.b[0] = (msg->payload+sizeof(uint8_t)+sizeof(uint8_t))[3];
	return (float)r.f;
}

/**
 * @brief Get field pitch_speed from set_roll_pitch_yaw_speed message
 *
 * @return Desired pitch angular speed in rad/s
 */
static inline float mavlink_msg_set_roll_pitch_yaw_speed_get_pitch_speed(const mavlink_message_t* msg)
{
	generic_32bit r;
	r.b[3] = (msg->payload+sizeof(uint8_t)+sizeof(uint8_t)+sizeof(float))[0];
	r.b[2] = (msg->payload+sizeof(uint8_t)+sizeof(uint8_t)+sizeof(float))[1];
	r.b[1] = (msg->payload+sizeof(uint8_t)+sizeof(uint8_t)+sizeof(float))[2];
	r.b[0] = (msg->payload+sizeof(uint8_t)+sizeof(uint8_t)+sizeof(float))[3];
	return (float)r.f;
}

/**
 * @brief Get field yaw_speed from set_roll_pitch_yaw_speed message
 *
 * @return Desired yaw angular speed in rad/s
 */
static inline float mavlink_msg_set_roll_pitch_yaw_speed_get_yaw_speed(const mavlink_message_t* msg)
{
	generic_32bit r;
	r.b[3] = (msg->payload+sizeof(uint8_t)+sizeof(uint8_t)+sizeof(float)+sizeof(float))[0];
	r.b[2] = (msg->payload+sizeof(uint8_t)+sizeof(uint8_t)+sizeof(float)+sizeof(float))[1];
	r.b[1] = (msg->payload+sizeof(uint8_t)+sizeof(uint8_t)+sizeof(float)+sizeof(float))[2];
	r.b[0] = (msg->payload+sizeof(uint8_t)+sizeof(uint8_t)+sizeof(float)+sizeof(float))[3];
	return (float)r.f;
}

/**
 * @brief Decode a set_roll_pitch_yaw_speed message into a struct
 *
 * @param msg The message to decode
 * @param set_roll_pitch_yaw_speed C-struct to decode the message contents into
 */
static inline void mavlink_msg_set_roll_pitch_yaw_speed_decode(const mavlink_message_t* msg, mavlink_set_roll_pitch_yaw_speed_t* set_roll_pitch_yaw_speed)
{
	set_roll_pitch_yaw_speed->target_system = mavlink_msg_set_roll_pitch_yaw_speed_get_target_system(msg);
	set_roll_pitch_yaw_speed->target_component = mavlink_msg_set_roll_pitch_yaw_speed_get_target_component(msg);
	set_roll_pitch_yaw_speed->roll_speed = mavlink_msg_set_roll_pitch_yaw_speed_get_roll_speed(msg);
	set_roll_pitch_yaw_speed->pitch_speed = mavlink_msg_set_roll_pitch_yaw_speed_get_pitch_speed(msg);
	set_roll_pitch_yaw_speed->yaw_speed = mavlink_msg_set_roll_pitch_yaw_speed_get_yaw_speed(msg);
}
