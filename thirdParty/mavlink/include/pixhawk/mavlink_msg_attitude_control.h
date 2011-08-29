// MESSAGE ATTITUDE_CONTROL PACKING

#define MAVLINK_MSG_ID_ATTITUDE_CONTROL 200

typedef struct __mavlink_attitude_control_t
{
 float roll; ///< roll
 float pitch; ///< pitch
 float yaw; ///< yaw
 float thrust; ///< thrust
 uint8_t target; ///< The system to be controlled
 uint8_t roll_manual; ///< roll control enabled auto:0, manual:1
 uint8_t pitch_manual; ///< pitch auto:0, manual:1
 uint8_t yaw_manual; ///< yaw auto:0, manual:1
 uint8_t thrust_manual; ///< thrust auto:0, manual:1
} mavlink_attitude_control_t;

#define MAVLINK_MSG_ID_ATTITUDE_CONTROL_LEN 21
#define MAVLINK_MSG_ID_200_LEN 21



#define MAVLINK_MESSAGE_INFO_ATTITUDE_CONTROL { \
	"ATTITUDE_CONTROL", \
	9, \
	{  { "roll", MAVLINK_TYPE_FLOAT, 0, 0, offsetof(mavlink_attitude_control_t, roll) }, \
         { "pitch", MAVLINK_TYPE_FLOAT, 0, 4, offsetof(mavlink_attitude_control_t, pitch) }, \
         { "yaw", MAVLINK_TYPE_FLOAT, 0, 8, offsetof(mavlink_attitude_control_t, yaw) }, \
         { "thrust", MAVLINK_TYPE_FLOAT, 0, 12, offsetof(mavlink_attitude_control_t, thrust) }, \
         { "target", MAVLINK_TYPE_UINT8_T, 0, 16, offsetof(mavlink_attitude_control_t, target) }, \
         { "roll_manual", MAVLINK_TYPE_UINT8_T, 0, 17, offsetof(mavlink_attitude_control_t, roll_manual) }, \
         { "pitch_manual", MAVLINK_TYPE_UINT8_T, 0, 18, offsetof(mavlink_attitude_control_t, pitch_manual) }, \
         { "yaw_manual", MAVLINK_TYPE_UINT8_T, 0, 19, offsetof(mavlink_attitude_control_t, yaw_manual) }, \
         { "thrust_manual", MAVLINK_TYPE_UINT8_T, 0, 20, offsetof(mavlink_attitude_control_t, thrust_manual) }, \
         } \
}


/**
 * @brief Pack a attitude_control message
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 *
 * @param target The system to be controlled
 * @param roll roll
 * @param pitch pitch
 * @param yaw yaw
 * @param thrust thrust
 * @param roll_manual roll control enabled auto:0, manual:1
 * @param pitch_manual pitch auto:0, manual:1
 * @param yaw_manual yaw auto:0, manual:1
 * @param thrust_manual thrust auto:0, manual:1
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_attitude_control_pack(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg,
						       uint8_t target, float roll, float pitch, float yaw, float thrust, uint8_t roll_manual, uint8_t pitch_manual, uint8_t yaw_manual, uint8_t thrust_manual)
{
	msg->msgid = MAVLINK_MSG_ID_ATTITUDE_CONTROL;

	put_float_by_index(msg, 0, roll); // roll
	put_float_by_index(msg, 4, pitch); // pitch
	put_float_by_index(msg, 8, yaw); // yaw
	put_float_by_index(msg, 12, thrust); // thrust
	put_uint8_t_by_index(msg, 16, target); // The system to be controlled
	put_uint8_t_by_index(msg, 17, roll_manual); // roll control enabled auto:0, manual:1
	put_uint8_t_by_index(msg, 18, pitch_manual); // pitch auto:0, manual:1
	put_uint8_t_by_index(msg, 19, yaw_manual); // yaw auto:0, manual:1
	put_uint8_t_by_index(msg, 20, thrust_manual); // thrust auto:0, manual:1

	return mavlink_finalize_message(msg, system_id, component_id, 21, 254);
}

/**
 * @brief Pack a attitude_control message on a channel
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param chan The MAVLink channel this message was sent over
 * @param msg The MAVLink message to compress the data into
 * @param target The system to be controlled
 * @param roll roll
 * @param pitch pitch
 * @param yaw yaw
 * @param thrust thrust
 * @param roll_manual roll control enabled auto:0, manual:1
 * @param pitch_manual pitch auto:0, manual:1
 * @param yaw_manual yaw auto:0, manual:1
 * @param thrust_manual thrust auto:0, manual:1
 * @return length of the message in bytes (excluding serial stream start sign)
 */
static inline uint16_t mavlink_msg_attitude_control_pack_chan(uint8_t system_id, uint8_t component_id, uint8_t chan,
							   mavlink_message_t* msg,
						           uint8_t target,float roll,float pitch,float yaw,float thrust,uint8_t roll_manual,uint8_t pitch_manual,uint8_t yaw_manual,uint8_t thrust_manual)
{
	msg->msgid = MAVLINK_MSG_ID_ATTITUDE_CONTROL;

	put_float_by_index(msg, 0, roll); // roll
	put_float_by_index(msg, 4, pitch); // pitch
	put_float_by_index(msg, 8, yaw); // yaw
	put_float_by_index(msg, 12, thrust); // thrust
	put_uint8_t_by_index(msg, 16, target); // The system to be controlled
	put_uint8_t_by_index(msg, 17, roll_manual); // roll control enabled auto:0, manual:1
	put_uint8_t_by_index(msg, 18, pitch_manual); // pitch auto:0, manual:1
	put_uint8_t_by_index(msg, 19, yaw_manual); // yaw auto:0, manual:1
	put_uint8_t_by_index(msg, 20, thrust_manual); // thrust auto:0, manual:1

	return mavlink_finalize_message_chan(msg, system_id, component_id, chan, 21, 254);
}

/**
 * @brief Encode a attitude_control struct into a message
 *
 * @param system_id ID of this system
 * @param component_id ID of this component (e.g. 200 for IMU)
 * @param msg The MAVLink message to compress the data into
 * @param attitude_control C-struct to read the message contents from
 */
static inline uint16_t mavlink_msg_attitude_control_encode(uint8_t system_id, uint8_t component_id, mavlink_message_t* msg, const mavlink_attitude_control_t* attitude_control)
{
	return mavlink_msg_attitude_control_pack(system_id, component_id, msg, attitude_control->target, attitude_control->roll, attitude_control->pitch, attitude_control->yaw, attitude_control->thrust, attitude_control->roll_manual, attitude_control->pitch_manual, attitude_control->yaw_manual, attitude_control->thrust_manual);
}

/**
 * @brief Send a attitude_control message
 * @param chan MAVLink channel to send the message
 *
 * @param target The system to be controlled
 * @param roll roll
 * @param pitch pitch
 * @param yaw yaw
 * @param thrust thrust
 * @param roll_manual roll control enabled auto:0, manual:1
 * @param pitch_manual pitch auto:0, manual:1
 * @param yaw_manual yaw auto:0, manual:1
 * @param thrust_manual thrust auto:0, manual:1
 */
#ifdef MAVLINK_USE_CONVENIENCE_FUNCTIONS

static inline void mavlink_msg_attitude_control_send(mavlink_channel_t chan, uint8_t target, float roll, float pitch, float yaw, float thrust, uint8_t roll_manual, uint8_t pitch_manual, uint8_t yaw_manual, uint8_t thrust_manual)
{
	MAVLINK_ALIGNED_MESSAGE(msg, 21);
	msg->msgid = MAVLINK_MSG_ID_ATTITUDE_CONTROL;

	put_float_by_index(msg, 0, roll); // roll
	put_float_by_index(msg, 4, pitch); // pitch
	put_float_by_index(msg, 8, yaw); // yaw
	put_float_by_index(msg, 12, thrust); // thrust
	put_uint8_t_by_index(msg, 16, target); // The system to be controlled
	put_uint8_t_by_index(msg, 17, roll_manual); // roll control enabled auto:0, manual:1
	put_uint8_t_by_index(msg, 18, pitch_manual); // pitch auto:0, manual:1
	put_uint8_t_by_index(msg, 19, yaw_manual); // yaw auto:0, manual:1
	put_uint8_t_by_index(msg, 20, thrust_manual); // thrust auto:0, manual:1

	mavlink_finalize_message_chan_send(msg, chan, 21, 254);
}

#endif

// MESSAGE ATTITUDE_CONTROL UNPACKING


/**
 * @brief Get field target from attitude_control message
 *
 * @return The system to be controlled
 */
static inline uint8_t mavlink_msg_attitude_control_get_target(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  16);
}

/**
 * @brief Get field roll from attitude_control message
 *
 * @return roll
 */
static inline float mavlink_msg_attitude_control_get_roll(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_float(msg,  0);
}

/**
 * @brief Get field pitch from attitude_control message
 *
 * @return pitch
 */
static inline float mavlink_msg_attitude_control_get_pitch(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_float(msg,  4);
}

/**
 * @brief Get field yaw from attitude_control message
 *
 * @return yaw
 */
static inline float mavlink_msg_attitude_control_get_yaw(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_float(msg,  8);
}

/**
 * @brief Get field thrust from attitude_control message
 *
 * @return thrust
 */
static inline float mavlink_msg_attitude_control_get_thrust(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_float(msg,  12);
}

/**
 * @brief Get field roll_manual from attitude_control message
 *
 * @return roll control enabled auto:0, manual:1
 */
static inline uint8_t mavlink_msg_attitude_control_get_roll_manual(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  17);
}

/**
 * @brief Get field pitch_manual from attitude_control message
 *
 * @return pitch auto:0, manual:1
 */
static inline uint8_t mavlink_msg_attitude_control_get_pitch_manual(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  18);
}

/**
 * @brief Get field yaw_manual from attitude_control message
 *
 * @return yaw auto:0, manual:1
 */
static inline uint8_t mavlink_msg_attitude_control_get_yaw_manual(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  19);
}

/**
 * @brief Get field thrust_manual from attitude_control message
 *
 * @return thrust auto:0, manual:1
 */
static inline uint8_t mavlink_msg_attitude_control_get_thrust_manual(const mavlink_message_t* msg)
{
	return MAVLINK_MSG_RETURN_uint8_t(msg,  20);
}

/**
 * @brief Decode a attitude_control message into a struct
 *
 * @param msg The message to decode
 * @param attitude_control C-struct to decode the message contents into
 */
static inline void mavlink_msg_attitude_control_decode(const mavlink_message_t* msg, mavlink_attitude_control_t* attitude_control)
{
#if MAVLINK_NEED_BYTE_SWAP
	attitude_control->roll = mavlink_msg_attitude_control_get_roll(msg);
	attitude_control->pitch = mavlink_msg_attitude_control_get_pitch(msg);
	attitude_control->yaw = mavlink_msg_attitude_control_get_yaw(msg);
	attitude_control->thrust = mavlink_msg_attitude_control_get_thrust(msg);
	attitude_control->target = mavlink_msg_attitude_control_get_target(msg);
	attitude_control->roll_manual = mavlink_msg_attitude_control_get_roll_manual(msg);
	attitude_control->pitch_manual = mavlink_msg_attitude_control_get_pitch_manual(msg);
	attitude_control->yaw_manual = mavlink_msg_attitude_control_get_yaw_manual(msg);
	attitude_control->thrust_manual = mavlink_msg_attitude_control_get_thrust_manual(msg);
#else
	memcpy(attitude_control, MAVLINK_PAYLOAD(msg), 21);
#endif
}
